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
	auto pos_iter = _unassemble_strs.upper_bound(index);
	if(pos_iter != _unassemble_strs.begin())
		pos_iter--;

	size_t new_idx = index;
	if(pos_iter != _unassemble_strs.end() && pos_iter->first <= index){
		if(pos_iter->first + pos_iter->second.size() > index){
			if(pos_iter->first + pos_iter->second.size() < index + data.size())
				new_idx = pos_iter->first + pos_iter->second.size();
			else
				return;
		}
	}

	else if(index < _next_assembled_idx){
		if(index + data.size() > _next_assembled_idx)
			new_idx = _next_assembled_idx;
		else
			return;
    }

	const size_t data_start_pos = new_idx - index;
	ssize_t data_size = data.size() - data_start_pos;
	
	pos_iter = _unassemble_strs.lower_bound(new_idx);
	while(pos_iter != _unassemble_strs.end()){
		const size_t data_end_idx = new_idx + data_size;
		if(data_end_idx > pos_iter->first){
			if(data_end_idx < pos_iter->second.size() + pos_iter->first){
				data_size = pos_iter->first - new_idx;
				break;
			}
			else{
				_unassembled_bytes_num -= pos_iter->second.size();
				pos_iter = _unassemble_strs.erase(pos_iter);
				continue;
			}
		}
		else
			break;
	}

	const size_t first_unread = _output.bytes_read();
    size_t first_unacceptable_idx = _capacity + first_unread;
    if (new_idx >= first_unacceptable_idx){
        return;
	}
	if(data_size > 0){
		string new_data = data.substr(data_start_pos, data_size);
		if(_next_assembled_idx == new_idx){
			const size_t write_size = _output.write(new_data);
			_next_assembled_idx += write_size;
			if(write_size < new_data.size()){
				const string data_to_store = new_data.substr(write_size);
				_unassembled_bytes_num += data_to_store.size();
                _unassemble_strs.insert(make_pair(_next_assembled_idx, std::move(data_to_store)));
			}
		}
		else{
			const string data_to_store = new_data.substr(0, new_data.size());
			_unassembled_bytes_num += data_to_store.size();
            _unassemble_strs.insert(make_pair(new_idx, std::move(data_to_store)));
		}
	}
	
    for (auto iter = _unassemble_strs.begin(); iter != _unassemble_strs.end(); ) {
        if (iter->first == _next_assembled_idx) {
            const size_t write_num = _output.write(iter->second);
            _next_assembled_idx += write_num;
            if (write_num < iter->second.size()) {
                _unassembled_bytes_num += iter->second.size() - write_num;
                _unassemble_strs.insert(make_pair(_next_assembled_idx, std::move(iter->second.substr(write_num))));

                _unassembled_bytes_num -= iter->second.size();
                _unassemble_strs.erase(iter);
                break;
            }
            // 如果写全了，则删除原有迭代器，并进行更新
            _unassembled_bytes_num -= iter->second.size();
            iter = _unassemble_strs.erase(iter);
        }
        // 否则直接离开
        else
            break;
    }

    if (eof)
        _eof_idx = index + data.size();

    if (_eof_idx <= _next_assembled_idx)
        _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes_num; }
bool StreamReassembler::empty() const { return _unassembled_bytes_num == 0; }
