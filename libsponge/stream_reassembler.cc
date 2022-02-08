#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
	StreamReassembler::SubStream sub_stream = StreamReassembler::SubStream{index, index + data.length(), eof, data};
	if (sub_stream._end <= _output.bytes_written()) return;
	if (!_end_to_sub_stream.count(sub_stream._end)) {
		_unassembled_bytes += data.length();
		_end_to_sub_stream[sub_stream._end] = sub_stream;
	}
	while (_end_to_sub_stream.count(_output.bytes_written())) {
		auto cur_sub_stream = _end_to_sub_stream[_output.bytes_written()];
		_end_to_sub_stream.erase(_output.bytes_written());
		_unassembled_bytes -= cur_sub_stream._data.length();

		_output.write(cur_sub_stream._data);
		if (cur_sub_stream._eof) _output.end_input();
	}

}

size_t StreamReassembler::unassembled_bytes() const {
	return _unassembled_bytes;
}

bool StreamReassembler::empty() const {
	return _output.buffer_empty();
}
