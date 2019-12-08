#include "fileHeader.h"

map<char, uint8_t> hex_map {
    {'0', 0},  {'1', 1},  {'2', 2},  {'3', 3},
    {'4', 4},  {'5', 5},  {'6', 6},
    {'7', 7},  {'8', 8},  {'9', 9},
    {'A', 10}, {'a', 10}, {'B', 11},
    {'b', 11}, {'C', 12}, {'c', 12},
    {'d', 13}, {'D', 13}, {'E', 14},
    {'e', 14}, {'F', 15}, {'f', 15}
};

FileHeader::FileHeader(){}

FileHeader::FileHeader(string s) : file_extension{ s } { }


void FileHeader::import_header_array(string tmp_vec){
  
  istringstream iss{ tmp_vec };
  string pair;
  while(getline(iss, pair, ' '))
    header_byte_array.push_back(convert_hex(pair));  
}


void FileHeader::import_footer_array(string tmp_vec){
  
  istringstream iss{ tmp_vec };
  string pair;
  while (getline(iss, pair, ' ')) {
    footer_byte_array.push_back(convert_hex(pair));
  }
}


uint8_t FileHeader::convert_hex(string& pair) {
  return ((hex_map[pair[0]] * 16) + hex_map[pair[1]]);
}





void FileHeader::print_info()
{
  cout << endl;
  cout << setw(17) << left << "EXTENSION" << ": " << file_extension << endl;
  cout << setw(17) << left << "HEADER SIGNATURE" << ": ";

  for (auto byte : header_byte_array) {
    cout << "\\x" << hex << setw(2) << setfill('0') << (int)byte;
  }
  if (!footer_byte_array.empty()) {
    cout << setw(17) << setfill(' ') << left <<
      endl << "FOOTER SIGNATURE" << ": ";

    for (auto byte : footer_byte_array) {
      cout << "\\x" << hex << setw(2) << setfill('0') << (int)byte;
    }
  }
  cout << setw(17) << setfill(' ') << dec << left <<
    endl << "MAX CARVE SIZE" << ": " << max_carve_size << endl;
}