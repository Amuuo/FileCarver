#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <map>
using namespace std;


struct FileHeader {

    FileHeader(){}
    FileHeader(const string& s) : file_extension{s}{ }

    string          file_extension;
    vector<uint8_t> header_byte_array;
    vector<uint8_t> footer_byte_array;
    int             max_carve_size;

    void get_header_byte_array(vector<string>&);        
    void get_footer_byte_array(vector<string>&);        
    int8_t convert_hex(string&);
    
};