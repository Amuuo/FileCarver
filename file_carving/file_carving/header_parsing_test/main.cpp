#include "main.h"





vector<FileHeader> patterns;



int main(){

  PatternMatcher PM;

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

  ifstream testFile{"testinput.txt"};
  vector<uint8_t> test_array;

  uint8_t current_byte;
  
  while(!testFile.eof()) {
    
    current_byte = (uint8_t)testFile.get();
    
    if((char)current_byte != '\n')
      test_array.push_back(current_byte);    
  }

  PM.create_goto(patterns);
  PM.find_matches(&test_array[0], test_array.size());
  return 0;
}