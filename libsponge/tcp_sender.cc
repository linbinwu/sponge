#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const {
    uint64_t count = 0;
    for (auto &pair : _waiting_ack_segments) {
        count += pair.second.seg.length_in_sequence_space();
    }
    return count;
}

void TCPSender::fill_window() {
    if (stream_in().eof() && next_seqno_absolute() == stream_in().bytes_written() + 2 && bytes_in_flight() == 0)
        return;
    bool syn = !send_syn && next_seqno_absolute() == 0;
    bool fin = !send_fin && stream_in().eof();
    if (_send_size < _window_size && (syn || fin)) {
        TCPSegment seg;
        seg.header().syn = syn;
        seg.header().fin = fin;
        seg.header().seqno = next_seqno();
        if (seg.length_in_sequence_space() == 0)
            return;
        _segments_out.push(seg);
        _waiting_ack_segments[_next_seqno] = WaitingAck{_cur_time, seg};
        _next_seqno += seg.length_in_sequence_space();
        send_syn |= syn;
        send_fin |= fin;
        _send_size += seg.length_in_sequence_space();
    }
    while (_send_size < _window_size) {
        TCPSegment seg;
        seg.header().seqno = next_seqno();
        seg.payload() =
            Buffer(_stream.read(min(TCPConfig::MAX_PAYLOAD_SIZE, static_cast<size_t>(_window_size - _send_size))));
        if (_stream.eof() && !send_fin && seg.length_in_sequence_space() + _send_size < _window_size) {
            seg.header().fin = true;
            send_fin |= true;
        }
        if (seg.length_in_sequence_space() == 0)
            break;
        _send_size += seg.length_in_sequence_space();
        _segments_out.push(seg);
        _waiting_ack_segments[_next_seqno] = WaitingAck{_cur_time, seg};
        _next_seqno += seg.length_in_sequence_space();
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);
    if (abs_ackno > _next_seqno)
        return;
    if (abs_ackno > _last_ackno) {
        _last_ackno = abs_ackno;
        _retransmission_timeout = _initial_retransmission_timeout;
        _consecutive_retransmissions = 0;
        vector<uint64_t> need_delete;
        for (auto &pair : _waiting_ack_segments) {
            TCPSegment cur_seg = pair.second.seg;
            uint64_t cur_abs_seqno = pair.first;
            uint64_t cur_abs_ackno = cur_abs_seqno + cur_seg.length_in_sequence_space();
            pair.second.send_time = _cur_time;
            if (cur_abs_ackno <= abs_ackno)
                need_delete.push_back(cur_abs_seqno);
        }
        for (auto &abs_seqno : need_delete) {
            _waiting_ack_segments.erase(abs_seqno);
        }
    }
    // treat window_size = 0 as 1
    int zero_specific = (window_size == 0 ? 1 : 0);
    _window_size = window_size + zero_specific;
    _send_size = _next_seqno - abs_ackno;
    fill_window();
    // recover window_size
    _window_size -= zero_specific;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _cur_time += ms_since_last_tick;
    TCPSegment earliest_seg;
    uint64_t earliest_abs_seqno = 0;
    bool found = false;
    for (auto &pair : _waiting_ack_segments) {
        uint64_t cur_abs_seqno = pair.first;
        TCPSegment seg = pair.second.seg;
        size_t pre_time = pair.second.send_time;
        if (_cur_time - pre_time >= _retransmission_timeout) {
            if (!found || (found && cur_abs_seqno <= earliest_abs_seqno)) {
                earliest_abs_seqno = cur_abs_seqno;
                earliest_seg = seg;
            }
            found = true;
        }
    }
    if (found) {
        if (_window_size != 0) {
            _retransmission_timeout <<= 1;
            _consecutive_retransmissions++;
        }
        _segments_out.push(earliest_seg);
        _waiting_ack_segments[earliest_abs_seqno] = WaitingAck{_cur_time, earliest_seg};
        for (auto &pair : _waiting_ack_segments) {
            pair.second.send_time = _cur_time;
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}
