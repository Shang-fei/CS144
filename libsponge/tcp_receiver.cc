#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    //LISTEN:
    if(!_set_syn_flag){
        if(!seg.header().syn)
            return;
        _set_syn_flag = true;
        _isn = seg.header().seqno;
    }
    //SYN_RECEIVED:
    uint64_t checkpoint = stream_out().bytes_written();
    uint64_t abs_seqno = unwrap(seg.header().seqno, _isn, checkpoint);

    uint64_t index = seg.header().syn ? 0 : abs_seqno - 1;  
    _reassembler.push_substring(seg.payload().copy(), index, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const{
    if(!_set_syn_flag){
        return nullopt;
    }

    //SYN_RECEIEVED:
    uint64_t offset = stream_out().bytes_written() + 1;

    //FIN_RECEIEVED:
    if(stream_out().input_ended())
        offset++;

    return wrap(offset, _isn);
}

size_t TCPReceiver::window_size() const {return _capacity - stream_out().buffer_size();}
