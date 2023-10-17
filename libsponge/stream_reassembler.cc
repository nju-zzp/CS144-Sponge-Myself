#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), _unassembled_list({}) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    // cut inside capacity window
    uint64_t index_border = _output.bytes_read() + _capacity;
    const string *new_string_ptr = &data;
    uint64_t new_index = index;
    bool new_eof = eof;
    if (index + data.size() > index_border) {
        new_string_ptr = new string(data.substr(0, index_border - index));
        new_eof = false;
    }

    // insert list
    auto it = _unassembled_list.begin();
    while (true) {
        if (it == _unassembled_list.end()) {
            _unassembled_list.push_back(make_shared<SubstringEntry>(*new_string_ptr, new_index, new_eof));
            break;
        } else if (new_index > (*it)->index) {
            // 去头重叠部分，迭代器递增，下一次循环
            new_string_ptr = cut_overlap_head(new_string_ptr, new_index, (*it)->data, (*it)->index);
            ++it;
            continue;
        } else {
            uint64_t tail_index = new_index + new_string_ptr->size();
            uint64_t it_entry_tail_index = (*it)->index + ((*it)->data).size();
            if (tail_index > it_entry_tail_index) {
                it = _unassembled_list.erase(it);
                continue;
            } else {
                // 去尾重叠部分，插入当前位置，结束迭代
                new_string_ptr = cut_overlap_tail(new_string_ptr, new_index, (*it)->index);
                _unassembled_list.insert(it, make_shared<SubstringEntry>(*new_string_ptr, new_index, new_eof));
                break;
            }
        }
    }

    // loop assemble consequent list front
    while (!_unassembled_list.empty() && _unassembled_list.front()->index <= _output.bytes_read() + _output.buffer_size()) {
        auto front_ptr = _unassembled_list.front();
        auto buffer_tail = _output.bytes_read() + _output.buffer_size();
        if (front_ptr->index == buffer_tail) {
            _output.write(front_ptr->data);
        } else  {
            if (front_ptr->index + (front_ptr->data).size() > _output.bytes_written()) {
                _output.write((front_ptr->data).substr(buffer_tail - front_ptr->index));
            }
        }
        if (front_ptr->eof) {
            _output.end_input();
        }
        _unassembled_list.pop_front();
    }

}

size_t StreamReassembler::unassembled_bytes() const {
    size_t count = 0;
    for (auto it = _unassembled_list.begin(); it != _unassembled_list.end(); ++it) {
        count += (*it)->data.size();
    }
    return count;
}

bool StreamReassembler::empty() const { return _unassembled_list.empty(); }


const string* StreamReassembler::cut_overlap_head(const string *new_string_ptr, uint64_t& new_index, const string& data_str, const uint64_t& data_index) {
    if (new_index >= data_index + data_str.size()) {
        return new_string_ptr;
    } else {
        uint64_t tail_index = new_index + new_string_ptr->size();
        if (tail_index <= data_index + data_str.size()) {
            new_index = tail_index;
            return new string("");
        } else {
            uint64_t start_position = data_index + data_str.size() - new_index;
            new_index = data_index + data_str.size();
            return new string(new_string_ptr->substr(start_position));
        }
    }

}

const string* StreamReassembler::cut_overlap_tail(const string *new_string_ptr, uint64_t& new_index, const uint64_t& data_index) {
    if ((new_index + new_string_ptr->size())) {
        return new_string_ptr;
    } else {
        if (new_index >= data_index) {
            new_index = new_index + new_string_ptr->size();
            return new string("");
        } else {
            uint64_t length = data_index - new_index;
            return new string(new_string_ptr->substr(0, length));
        }
    }
}


