
#include "patternMatcher.h"



void PatternMatcher::find_matches(uint8_t* tmp, disk_pos blocksize)
{
  printf("\n\nFunction: find_matches()");
  int state = 0;
  for (int i = 0; i < blocksize; i++) {
    while (go_to[state].count(tmp[i]) == 0) {
      state = failure[state];
    }
    state = go_to[state][tmp[i]];

    if (output.count(state) > 0)
    {
      auto t = output[state].size();
      vector<uint8_t> tmp = output[state];

      for (int j = 0; j < t; j++)
      {
        positions[tmp[j]].push_back(i);
      }
    }
  }
}





void PatternMatcher::check_for_matches() {

  // check for matches in each regex match group
  if (regex_matches[EXTENSION].matched) {
    patterns.push_back(regex_matches[EXTENSION].str());
    if (regex_matches[HEADER].matched) {
      patterns.back().import_header_array(regex_matches[HEADER].str());
      if (regex_matches[FOOTER].matched)
        patterns.back().import_footer_array(regex_matches[FOOTER].str());
      if (regex_matches[MAX_SIZE].matched)
        patterns.back().max_carve_size = stol(regex_matches[MAX_SIZE].str());
      else {
        cerr << "Error with max size formatting. Exiting..." << endl;
        exit(1);
      }
    }
    else {
      cerr << "Error with header signature formatting. Exiting..." << endl;
      exit(1);
    }
  }
}





void PatternMatcher::import_file_signatures_and_setup_patterns()
{
  //==================================================
  // regex to parse the file signature file
  regex regex_pattern{
    "([[:w:]]+)[[:s:]]+\\[[[:s:]]((?:[[:xdigit:]]{2}"
    "[[:s:]])+)\\][[:s:]]*(?:\\[[[:s:]]((?:[[:xdigit:]]{2}"
    "[[:s:]])+)\\][[:s:]]*)?([[:digit:]]+)[[:s:]]*"
  };
  ifstream    header_file{ "file_headers.txt" };
  string      current_line;
  //==================================================

  while (header_file.good()) {
    getline(header_file, current_line);
    regex_search(current_line, regex_matches, regex_pattern);
    check_for_matches();
  }
  create_goto();
  create_failure();
}






void PatternMatcher::test_input_for_matches(const char* input_file)
{
  //==================================================
  ifstream  testFile   { input_file, ios::ate };
  uint8_t*  test_array { nullptr };
  //==================================================

  auto file_size = testFile.tellg();

  // allocate array of filesize
  test_array = new uint8_t[file_size];
  testFile.seekg(0, testFile.beg);

  // copy file into array
  testFile.read((char*)test_array, file_size);

  find_matches(test_array, file_size);

  for (int i = 0; i < positions.size(); i++) {
    printf("\n%s: ", patterns[i].file_extension.c_str());

    for (auto itr = positions[i].begin();
      itr != positions[i].end(); ++itr)
      printf("\t-%zd\n", static_cast<int>(*itr) -
      (patterns[i].header_byte_array.size() - 1));
  }
}




void PatternMatcher::create_goto()
{
  positions.resize(patterns.size());
  int new_state = 0;

  for (int i = 0; i < patterns.size(); i++) {
    int state = 0, j;

    for (j = 0; j < patterns[i].header_byte_array.size() &&
                go_to[state].count(patterns[i].header_byte_array[j]) >= 1; j++) {
      state = go_to[state][patterns[i].header_byte_array[j]];
    }

    for (int k = j; k < patterns[i].header_byte_array.size(); k++) {
      go_to[state][patterns[i].header_byte_array[k]] = ++new_state;
      state = new_state;
    }

    if (output.find(state) == output.end())
      output[state] = vector<uint8_t>(1, i);
    else {
      output[state].push_back(i);
    }
  }
  failure.resize(new_state + 1, 0);

  for (uint8_t i = 0x00; i < 0xFF; i++)
    if (go_to[0].count((int)i) == 0)
      go_to[0][i] = 0;
}





void PatternMatcher::create_failure() {

  queue<int> q;
  int        current;
  int        state;

  for(auto& itr : go_to[0])
    if (itr.second != 0) {
      q.push(itr.second);
      failure[itr.second] = 0;
    }

  while (!q.empty()) {
    current = q.front();
    q.pop();
    state = failure[current];
    for (auto& itr : go_to[current]){
      if (go_to[current].count(itr.first) != 0) {
        q.push(itr.second);
        state = failure[current];
        while (go_to[state].count(itr.first) == 0)
          state = failure[state];
        failure[itr.second] = go_to[state][itr.first];
        output[itr.second].insert(output[itr.second].end(),
                                  output[failure[itr.second]].begin(),
                                  output[failure[itr.second]].end());
      }
    }
  }
}