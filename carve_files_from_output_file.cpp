
#include <unistd.h>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>

using namespace std;
typedef long long int disk_pos;

disk_pos filesize = 1024 * 1024 * 35;

int main(){

  char* file_buff = new char[filesize];
  // write found file in output directory

  ifstream input_disk;
  input_disk.open("/dev/sdc2");

  ifstream file_markers;
  file_markers.open("output.txt");
  string carved_files_folder_name = "/home/adam/Desktop/carved_files/";
  //mkdir(carved_files_folder_name.c_str(), O_RDWR);

  int i = 0;

  disk_pos file_mark;

  while (!file_markers.eof()) {
    string line;
    getline(file_markers, line);
    istringstream iss{line};
    iss >> file_mark;

    input_disk.seekg(file_mark);

    input_disk.read(file_buff, filesize);

    ofstream carved_file;

    carved_file.open(string{carved_files_folder_name + to_string(i++) + ".cr2"});

    carved_file.write(file_buff, filesize);
  }

  input_disk.close();
  // print results to console
  printf("\n\nSUCCESS!\n\n");

}
