#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
		: _unassemble_strs()
		, _next_assembled_idx(0)
		, _unassembled_bytes_num(0)
		, _eof_idx(-1)
		, _output(capacity)
		, _capacity(capacity){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
	size_t data_start_idx = index;
	auto pos_iter = _unassemble_strs.upper_bound(index);
	if(!(_unassemble_strs.empty() || pos_iter == _unassemble_strs.begin())){
		pos_iter--;
		if(pos_iter->first + pos_iter->second.size() > index){
			if(pos_iter->first + pos_iter->second.size() < index + data.size())
				data_start_idx = pos_iter->first + pos_iter->second.size();
			else
				return;
		}
	}
	else if(_next_assembled_idx > index){
		if(_next_assembled_idx < index + data.size())
			data_start_idx = _next_assembled_idx;
		else
			return;
	}

	const size_t first_unread = _output.bytes_read();
    size_t first_unacceptable_idx = _capacity + first_unread;
    if (data_start_idx >= first_unacceptable_idx){
        return;
	}

	const size_t data_start_pos = data_start_idx - index;
	string new_data = data.substr(data_start_pos);

	pos_iter = _unassemble_strs.lower_bound(data_start_idx);
	while(pos_iter != _unassemble_strs.end()){
		const size_t data_end_idx = data_start_idx + new_data.size();
		if(data_end_idx > pos_iter->first){
			if(data_end_idx < pos_iter->first + pos_iter->second.size()){
				new_data += pos_iter->second.substr(data_end_idx - pos_iter->first);
			}
			_unassembled_bytes_num -= pos_iter->second.size();
			pos_iter = _unassemble_strs.erase(pos_iter);
		}
		else if(data_end_idx == pos_iter->first){
			new_data += pos_iter->second;
			_unassembled_bytes_num -= pos_iter->second.size();
			pos_iter = _unassemble_strs.erase(pos_iter);
		}
		else
			break;
	}

	if(_next_assembled_idx == data_start_idx){
		const size_t write_size = _output.write(new_data);
		_next_assembled_idx += write_size;
		if(write_size < new_data.size()){
			new_data = new_data.substr(write_size);
			_unassembled_bytes_num += new_data.size();
            //_unassemble_strs.insert(make_pair(_next_assembled_idx, std::move(new_data)));
		}
	}
	else{
		_unassembled_bytes_num += new_data.size();
        _unassemble_strs.insert(make_pair(data_start_idx, std::move(new_data)));
	}

    if (eof)
        _eof_idx = index + data.size();

    if (_eof_idx <= _next_assembled_idx)
        _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes_num; }
bool StreamReassembler::empty() const { return _unassembled_bytes_num == 0; }
