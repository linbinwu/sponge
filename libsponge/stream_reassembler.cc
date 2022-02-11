#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , _unassembled_bytes(std::vector<char>(capacity))
    , _unassembled_exist(std::vector<bool>(capacity))
    , _unassembled_bytes_count(0)
    , _eof_index(-1) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    uint64_t read_len = _output.bytes_read();
    uint64_t write_len = _output.bytes_written();
    uint64_t data_len = data.length();
    uint64_t eof_index = index + data_len;
    for (uint64_t idx = 0; idx < data_len; idx++) {
        uint64_t cur_index = index + idx;
        // legal in [read_len, read_len + _capacity)
        if (cur_index >= read_len + _capacity)
            break;
        if (cur_index < write_len)
            continue;
        if (!_unassembled_exist[cur_index % _capacity]) {
            _unassembled_bytes_count++;
            _unassembled_bytes[cur_index % _capacity] = data.at(idx);
            _unassembled_exist[cur_index % _capacity] = true;
        }
    }
    if (eof)
        _eof_index = eof_index;

    string assembled_bytes = "";
    uint64_t end_index = write_len;
    for (uint64_t idx = write_len; idx < read_len + _capacity; idx++) {
        if (!_unassembled_exist[idx % _capacity])
            break;
        assembled_bytes += _unassembled_bytes[idx % _capacity];
        _unassembled_exist[idx % _capacity] = false;
        _unassembled_bytes_count--;
        end_index = idx + 1;
    }
    if (!assembled_bytes.empty())
        _output.write(assembled_bytes);
    if (end_index == _eof_index)
        _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes_count; }

bool StreamReassembler::empty() const { return _output.buffer_empty(); }
