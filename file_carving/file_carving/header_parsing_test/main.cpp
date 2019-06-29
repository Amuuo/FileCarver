#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <fcntl.h>
#include <getopt.h>
#include <locale.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <array>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <mutex>
#include <queue>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <utility>
#include <functional>
#include <ctime>
#include <cmath>
#include <dirent.h>
#include <cstdio>
#include <memory>

using namespace std;

map<char, int8_t> hex_map {
    {'1', 1},  {'2', 2},  {'3', 3},
    {'4', 4},  {'5', 5},  {'6', 6},
    {'7', 7},  {'8', 8},  {'9', 9},
    {'A', 10}, {'a', 10}, {'B', 11},
    {'b', 11}, {'C', 12}, {'c', 12},
    {'d', 13}, {'D', 13}, {'E', 14},
    {'e', 14}, {'F', 15}, {'f', 15}
};

struct FileHeader {

    FileHeader(){}
    FileHeader(const string& s) : file_extension{s}{}

    string          file_extension;
    vector<uint8_t> header_byte_array;
    vector<uint8_t> footer_byte_array;
    int             max_carve_size;


    void get_header_byte_array(vector<string>& tmp_vec){
        for(auto pair : tmp_vec) {
          header_byte_array.push_back(convert_hex(pair));
        }
    }
    void get_footer_byte_array(vector<string>& tmp_vec){
        for(auto pair : tmp_vec) {
          footer_byte_array.push_back(convert_hex(pair));
        }
    }


    int8_t convert_hex(string& pair) {
      return ((hex_map[pair[0]] * 16) + hex_map[pair[1]]);
    }
};


vector<FileHeader> patterns;







int main(){

  ifstream       header_file;
  string         current_line;
  //string         current_word;

  header_file.open("file_headers.txt");
  getline(header_file, current_line);


  while(!header_file.eof())
  {
    getline(header_file, current_line);
    istringstream iss{current_line};
    cout << "current line: " << current_line << endl;
    while(iss)
    {
      string current_word;
      iss >> current_word;
      cout << "current word: " << current_word << endl;
      patterns.push_back(current_word);

      iss >> current_word;

      cout << "current word: " << current_word << endl;
      if(current_word == "[")
      {
        vector<string> tmp_vec;
        iss >> current_word;
        cout << "\n\tHEADER: ";
        while (current_word != "]")
        {
          tmp_vec.push_back(current_word);
          cout << tmp_vec.back() << " ";
          iss >> current_word;
        }
        cout << endl;
        patterns.back().get_header_byte_array(tmp_vec);
        iss >> current_word;
      }
      /*
      else
      {
        cout << "\ncurrent word: " << current_word;
        cout << "\nSignature file formatting error... Exiting....";
        exit(1);
      }
      */
      if (current_word == "[")
      {
        vector<string> tmp_vec;
        iss >> current_word;
        cout << "\n\tFOOTER: ";
        while (current_word != "]")
        {
          tmp_vec.push_back(current_word);
          cout << tmp_vec.back() << " ";
          iss >> current_word;
        }
        cout << endl;
        patterns.back().get_footer_byte_array(tmp_vec);
      }
      iss >> current_word;
      patterns.back().max_carve_size = stol(current_word);
      while (iss) iss.get();
      cout << "carve_size: " << patterns.back().max_carve_size << endl;
    }
  }

  for(auto pattern : patterns)
  {
    cout << "\nFile extension: " << pattern.file_extension;
    cout << "\nHeader signature: ";

    for(auto byte : pattern.header_byte_array)
      cout << setw(2) << setfill('0') << uppercase << hex << (int)byte << " ";

    cout << "\nFooter signature: ";

    if(!pattern.footer_byte_array.empty())
    {
        for(auto byte : pattern.footer_byte_array)
            cout << setw(2) << uppercase << hex << (int)byte << " ";

    }
    cout << "\nMax Carve Size: " << dec << pattern.max_carve_size << endl;
  }

  return 0;
}