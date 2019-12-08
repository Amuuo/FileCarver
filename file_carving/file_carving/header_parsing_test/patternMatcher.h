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

struct PatternMatcher {

  PatternMatcher() {}

  map<uint8_t, map<int, uint8_t>>  go_to{};
  map<uint8_t, vector<int>>     output{};
  vector<list<uint8_t>>         positions{};
  vector<uint8_t>               failure{};

  void create_goto(const vector<FileHeader>& patterns);
  void create_failure();
  void find_matches(uint8_t*, disk_pos);
};
