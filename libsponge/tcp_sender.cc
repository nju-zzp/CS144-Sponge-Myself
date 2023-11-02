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
    , _stream(capacity)
    , _current_retransmission_timeout(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const {
    uint64_t count = 0;
    for(auto& s : this->_segments_outstanding) {
        count += s.length_in_sequence_space();
    }
    return count;
}

void TCPSender::fill_window() {
    while (!this->fin_send_flag) {
        TCPSegment segment;

        uint16_t _window_size = this->_window_size_recently;
        if (_window_size == 0) {
            _window_size = 1;
        }
        if (_window_size <= bytes_in_flight()) {
            return;
        } else {
            _window_size -= bytes_in_flight();
        }

        if (_next_seqno == 0) {
            _window_size -= 1;
            segment.header().syn = true;
        }

        segment.header().seqno = wrap(_next_seqno, _isn);

        size_t payload_expect_size = _window_size;

        if (_window_size > _stream.buffer_size()) {
            if (_stream.buffer_size() > TCPConfig::MAX_PAYLOAD_SIZE) {
                payload_expect_size = TCPConfig::MAX_PAYLOAD_SIZE;
            } else{
                payload_expect_size = _stream.buffer_size();
                if (_stream.input_ended()) {
                    segment.header().fin = true;
                    this->fin_send_flag = true;
                }
            }
        } else {
            if (payload_expect_size > TCPConfig::MAX_PAYLOAD_SIZE) {
                payload_expect_size = TCPConfig::MAX_PAYLOAD_SIZE;
            }
        }

        segment.payload() = Buffer(std::move(_stream.read(payload_expect_size)));

        if (segment.length_in_sequence_space() > 0) {
            _segments_out.push(segment);
            _next_seqno += segment.length_in_sequence_space();
            TCPSegment outstandingSegment(segment);
            _segments_outstanding.push_back(outstandingSegment);
            if (!_timer_start_flag) {
                timer_start();
            }
        } else {
            break;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    this->_window_size_recently = window_size;
    bool valid_ack = false;
    uint64_t ack_absolute_next_seqno = unwrap(ackno, this->_isn, this->_next_seqno);
    while (true) {
        if (_segments_outstanding.empty() || ack_absolute_next_seqno > _next_seqno) {
            break;
        }
        TCPSegment& front_segment = _segments_outstanding.front();
        uint64_t element_abs_next_seqno = unwrap(front_segment.header().seqno, this->_isn, this->_next_seqno) + front_segment.length_in_sequence_space();
        if (ack_absolute_next_seqno >= element_abs_next_seqno) {
            _segments_outstanding.pop_front();
            valid_ack = true;
        } else {
            break;
        }
    }
    if (valid_ack) {
        _current_retransmission_timeout = _initial_retransmission_timeout;
        _consecutive_retransmissions = 0;
        timer_stop();
        if (!_segments_outstanding.empty()) {
            timer_start();
        }
    }
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    this->_alive_ms += ms_since_last_tick;
    if (check_timer_expired() && !_segments_outstanding.empty()) {
        _segments_out.push(_segments_outstanding.front());
        if (_window_size_recently > 0) {
            _current_retransmission_timeout *= 2;
            _consecutive_retransmissions += 1;
        }
        timer_stop();
        timer_start();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(segment);
}
