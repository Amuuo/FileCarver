#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

map<char, int8_t> hex_map = {
    ('1', 1),  ('2', 2),  ('3', 3),
    ('4', 4),  ('5', 5),  ('6', 6),
    ('7', 7),  ('8', 8),  ('9', 9),
    ('A', 10), ('a', 10), ('B', 11),
    ('b', 11), ('C', 12), ('c', 12),
    ('d', 13), ('D', 13), ('E', 14),
    ('e', 14), ('F', 15), ('f', 15)
};

struct FileHeader {

    FileHeader(){}
    FileHeader(const string& s) : file_extension{s}{}

    string          file_extension;
    vector<uint8_t> header_byte_array;
    vector<uint8_t> footer_byte_array;
    int             max_carve_size;

    void get_byte_array(vector<string>& tmp_vec){
        for(auto pair : tmp_vec) {
          header_byte_array.push_back(convert_hex(pair));
        }
    }

    int8_t convert_hex(string& pair) {
      return ((hex_map(pair[0]) * 16) + hex_map(pair[1]));
    }
};