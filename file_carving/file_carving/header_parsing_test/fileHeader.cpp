#include "fileHeader.h"

map<char, int8_t> hex_map {
    {'1', 1},  {'2', 2},  {'3', 3},
    {'4', 4},  {'5', 5},  {'6', 6},
    {'7', 7},  {'8', 8},  {'9', 9},
    {'A', 10}, {'a', 10}, {'B', 11},
    {'b', 11}, {'C', 12}, {'c', 12},
    {'d', 13}, {'D', 13}, {'E', 14},
    {'e', 14}, {'F', 15}, {'f', 15}
};


void FileHeader::get_header_byte_array(vector<string>& tmp_vec){
    for(auto pair : tmp_vec) {
      header_byte_array.push_back(convert_hex(pair));
    }
}
void FileHeader::get_footer_byte_array(vector<string>& tmp_vec){
    for(auto pair : tmp_vec) {
      footer_byte_array.push_back(convert_hex(pair));
    }
}
int8_t FileHeader::convert_hex(string& pair) {
  return ((hex_map[pair[0]] * 16) + hex_map[pair[1]]);
}
