#pragma once
#include <cstdio>
#include <map>
#include <iostream>
#include <list>
#include <vector>
#include <queue>
#include <string>
#include "fileHeader.h"

using namespace std;
typedef unsigned long long disk_pos;

enum matches {
  EXTENSION = 1,
  HEADER,
  FOOTER,
  MAX_SIZE
};


struct PatternMatcher {

  PatternMatcher() {}


  vector<FileHeader>            patterns;
  map<int, map<uint8_t, int>>   go_to{};
  map<int, vector<uint8_t>>     output{};
  vector<list<uint8_t>>         positions{};
  vector<uint8_t>               failure{};
  smatch                        regex_matches;

  void         create_goto();
  void         create_failure();
  void         find_matches(uint8_t*, disk_pos);
  void         test_input_for_matches(const char*);
  void         check_for_matches();
  void         import_file_signatures_and_setup_patterns();
};
