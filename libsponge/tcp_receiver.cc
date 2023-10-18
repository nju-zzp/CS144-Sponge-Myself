#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (seg.header().syn) {
        _start_flag = true;
        this->_isn = seg.header().seqno;
    }
    if (!_start_flag)
        return;
    uint64_t checkpoint = _reassembler.stream_out().bytes_written() + 1;
    uint64_t substring_stream_index = unwrap(seg.header().syn ? seg.header().seqno + 1 : seg.header().seqno, this->_isn, checkpoint) - 1;
    bool eof = seg.header().fin;
    this->_reassembler.push_substring(string(seg.payload().str()), substring_stream_index, eof);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_start_flag) {
        auto temp1 = wrap(_reassembler.stream_out().bytes_written() + 1, this->_isn);
        uint32_t eof_1 = (_reassembler.stream_out().input_ended() ? 1 : 0);
        auto result = temp1 + eof_1;
        return result;
    } else {
        return nullopt;
    }
}

size_t TCPReceiver::window_size() const {
    return _reassembler.stream_out().remaining_capacity();
}
