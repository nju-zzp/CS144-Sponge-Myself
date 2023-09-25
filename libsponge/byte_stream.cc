#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) :  _capacity(capacity){ }

size_t ByteStream::write(const string &data) {
    size_t count = 0;
    const size_t length = data.size();
    while ((_capacity - byteBuffer.size() > 0) && count < length) {
        byteBuffer.push_back(data[count++]);
        writtenBytesCount++;
    }
    return count;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t length = byteBuffer.size() > len ? len : byteBuffer.size();
    size_t count = 0;
    ostringstream stringTemp;
    while (count < length) {
        stringTemp << byteBuffer.at(count++);
    }
    return stringTemp.str();
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t length = byteBuffer.size() > len ? len : byteBuffer.size();
    size_t count = 0;
    while (count < length) {
        byteBuffer.pop_front();
        count++;
        readBytesCount++;
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t count = 0;
    ostringstream stringTemp;
    while (byteBuffer.size() > 0 && count < len) {
        stringTemp << byteBuffer.front();
        byteBuffer.pop_front();
        count++;
        readBytesCount++;
    }
    return stringTemp.str();
}

void ByteStream::end_input() {_input_end_flag = true;}

bool ByteStream::input_ended() const { return _input_end_flag; }

size_t ByteStream::buffer_size() const { return byteBuffer.size(); }

bool ByteStream::buffer_empty() const { return byteBuffer.empty(); }

bool ByteStream::eof() const { return byteBuffer.empty() && _input_end_flag; }

size_t ByteStream::bytes_written() const { return writtenBytesCount; }

size_t ByteStream::bytes_read() const { return readBytesCount; }

size_t ByteStream::remaining_capacity() const { return _capacity - byteBuffer.size(); }
