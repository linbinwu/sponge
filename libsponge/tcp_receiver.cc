#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
	if (seg.header().syn) {
		std::srand(std::time(0));
		_receiver_isn = WrappingInt32(std::rand());
		_sender_isn = seg.header().seqno;
		_syned = true;
		_fined = false;
		_abs_seqno = 0;
	}
	if (!_syned) return;
	if (seg.header().fin)
		_fined = true;
	uint64_t abs_seqno = unwrap(seg.header().seqno, _sender_isn, _abs_seqno);
	if (!seg.header().syn && abs_seqno == 0) return;
	_reassembler.push_substring(seg.payload().copy(), abs_seqno - (abs_seqno > 0), seg.header().fin);
	_abs_seqno = _reassembler.stream_out().bytes_written();
}

optional<WrappingInt32> TCPReceiver::ackno() const {
	if (!_syned) return {};
	return wrap(_abs_seqno + 1 + _reassembler.stream_out().input_ended(), _sender_isn);
}

size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity(); }
