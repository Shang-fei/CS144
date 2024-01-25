#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _last_ackno; }

void TCPSender::fill_window() {
    TCPSegment seg;
    //CLOSED : waiting for stream to begin (no SYN sent)
    if(next_seqno_absolute() == 0){
        seg.header().syn = true;
        send_segment(seg);
        return;
    }
    //SYN_SENT : stream started but nothing acknowledged
    else if(next_seqno_absolute() > 0 && next_seqno_absolute() == bytes_in_flight()){
        return;
    }

    uint16_t current_window_size = _last_window_size ? _last_window_size : 1;
    size_t remaining_window_size;
    while((remaining_window_size = current_window_size - bytes_in_flight())){
        //SYN_ACKED : stream ongoing
        if(!stream_in().eof() && next_seqno_absolute() > bytes_in_flight()){
            if(stream_in().buffer_empty()) return;
            size_t payload_size = min(TCPConfig::MAX_PAYLOAD_SIZE, remaining_window_size);
            seg.payload() = Buffer(stream_in().read(payload_size));
            if(stream_in().eof() && seg.length_in_sequence_space() < remaining_window_size){
                seg.header().fin = true;
            }
            send_segment(seg);
        }
        else if(stream_in().eof()){
            //SYN_ACKED : stream has reach EOF, but FIN flag hasn't benn set yet;
            if(next_seqno_absolute() < stream_in().bytes_written() + 2){
                seg.header().fin = true;
                send_segment(seg);
                return;
            }
            //FIN_SENT:
            //FIN_ACKED:
            else 
                return;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno, _isn, _last_ackno);
    if(abs_ackno > next_seqno_absolute()) 
        return;
    
    if(abs_ackno > _last_ackno){
        _last_ackno = abs_ackno;
        while(_segments_outstanding.size()){
            TCPSegment seg = _segments_outstanding.front().second;
            uint64_t abs_seqno = _segments_outstanding.front().first;
            if(abs_seqno + seg.length_in_sequence_space() <= abs_ackno)
                _segments_outstanding.pop();
            else
                break;
        }
        _RTO = _initial_retransmission_timeout;
        _consecutive_retransmission_counts = 0;

        if(!_segments_outstanding.empty())    
            _timer.start(_RTO);
        else
            _timer.stop();
    }

    _last_window_size = window_size;
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call
//! to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(_timer.is_started()){
        _timer.tick(ms_since_last_tick);
        if(_timer.is_expired()){
            TCPSegment seg = _segments_outstanding.front().second;
            _segments_out.push(seg);
            if(_last_window_size > 0){
                _RTO *= 2;
                _consecutive_retransmission_counts++;
            }
            _timer.start(_RTO);
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission_counts; }

void TCPSender::send_empty_segment(){
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    send_segment(seg);
}

void TCPSender::send_segment(TCPSegment &seg){
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
    _segments_outstanding.push({next_seqno_absolute(), seg});
    _next_seqno += seg.length_in_sequence_space();

    if(!_timer.is_started())
        _timer.start(_RTO);
}
