#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) { capacity_ = capacity; }

size_t ByteStream::write(const string &data) {
    size_t write_len = min(data.length(), remaining_capacity());
    buffer_ = buffer_ + data.substr(0, write_len);
    bytes_written_ += write_len;
    return write_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
std::string ByteStream::peek_output(const size_t len) const {
    std::string output = buffer_.substr(0, min(len, buffer_.length()));
    return output;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t read_len = min(len, buffer_.length());
    buffer_ = buffer_.substr(read_len, buffer_.length());
    bytes_read_ += read_len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string res = peek_output(len);
    pop_output(len);
    return res;
}

void ByteStream::end_input() { end_input_ = true; }

bool ByteStream::input_ended() const { return end_input_; }

size_t ByteStream::buffer_size() const { return buffer_.length(); }

bool ByteStream::buffer_empty() const { return buffer_.empty(); }

bool ByteStream::eof() const { return end_input_ && buffer_empty(); }

size_t ByteStream::bytes_written() const { return bytes_written_; }

size_t ByteStream::bytes_read() const { return bytes_read_; }

size_t ByteStream::remaining_capacity() const { return capacity_ - buffer_.length(); }
