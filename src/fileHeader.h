#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <regex>
#include <fstream>
//#include "patternMatcher.h"
using namespace std;


struct FileHeader {
  
  using byte_vec = vector<uint8_t>;

  FileHeader ();
  FileHeader (string);

  string    file_extension{};
  byte_vec  header_byte_array{};
  byte_vec  footer_byte_array{};
  int       max_carve_size{};

  void       import_header_array (string);
  void       import_footer_array           (string);
  void       print_info                    ();
  uint8_t    convert_hex                   (string&);
};