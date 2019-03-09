#include "cmd_options.h"


Cmd_Options::Cmd_Options(){}

Cmd_Options::Cmd_Options(int argc, char** argv) {
      setlocale(LC_NUMERIC, "");

  static struct option long_options[] =
  {
    {"blocksize",     required_argument, NULL, 'b'},
    {"offset_start",  required_argument, NULL, 'S'},
    {"offset_end",    required_argument, NULL, 'E'},
    {"input",         required_argument, NULL, 'i'},
    {"output_folder", required_argument, NULL, 'o'},
    {"carve_files",   no_argument,       NULL, 'c'},
    {"verbose",       no_argument,       NULL, 'v'},
    {"help",          no_argument,       NULL, 'h'},
    {NULL, 0, NULL, 0}
  };

  int           cmd_option;
  extern char*  optarg;

  while ((cmd_option = getopt_long(argc, argv, "b:S:E:s:i:o:vh",
                                   long_options, NULL)) != -1)
  {
    switch (cmd_option)
    {
      case 'b': blocksize     = atoll(optarg);  break;
      case 'S': offset_start  = atoll(optarg);  break;
      case 'E': offset_end    = atoll(optarg);  break;
      case 'i': input         = string{optarg}; break;
      case 'o': output_folder = string{optarg}; break;
      case 'c': carve_files   = true;           break;
      case 'v': verbose       = true;           break;

      case 'h':
        printf (
        "\n\t--blocksize      -b  <size of logical blocks in bytes>"
        "\n\t--offset_start   -S  <offset from start of disk in bytes>"
        "\n\t--offset_end     -E  <offset from start of disk in bytes>"
        "\n\t--input          -i  <file to use for header search>"
        "\n\t--output_folder  -o  <folder to use for output files>"
        "\n\t--carve_files    -c  <write found files to current directory>"
        "\n\t--verbose        -v  <display verbose output of ongoing process>"
        "\n\t--help           -h  <print help options>\n\n"
        );

        exit(0);

      default: break;
    }
  }

  if (output_folder == "")
  {
    char tmp_output_folder[50];
    if (!(getcwd(tmp_output_folder, 50)))
      printf("\nCould not retrieve directory with 'getcwd'");
    output_folder = string{tmp_output_folder};
  }

  if (offset_end == 0)
  {
    printf("\n>> No offset end detected, setting offset to end of file");
    FILE* file_size_stream = fopen(input.c_str(), "r");
    fseek(file_size_stream, 0, SEEK_END);
    offset_end = ftell(file_size_stream);
    fclose(file_size_stream);
  }

  long long filesize = offset_end - offset_start;

  if (input == "")
  {
    printf("\n>> Input file required");
    exit(1);
  }
}



Cmd_Options::Cmd_Options(int _argc, char** _argv, bool v, disk_pos b, disk_pos os,
              disk_pos oe, bool c)
      : argc{_argc},
        argv{_argv},
        verbose{v},
        blocksize{b},
        offset_start{os},
        offset_end{oe},
        carve_files{c} {}
Cmd_Options::Cmd_Options(const Cmd_Options& cpy){
    blocksize      = cpy.blocksize;
    offset_start   = cpy.offset_start;
    offset_end     = cpy.offset_end;
    output_folder  = cpy.output_folder;
    input          = cpy.input;
    output_file    = cpy.output_file;
    argc           = cpy.argc;
    argv           = cpy.argv;
    verbose        = cpy.verbose;
  }
Cmd_Options::Cmd_Options(Cmd_Options&& cpy){
    blocksize      = move(cpy.blocksize);
    offset_start   = move(cpy.offset_start);
    offset_end     = move(cpy.offset_end);
    output_folder  = move(cpy.output_folder);
    input          = move(cpy.input);
    output_file    = move(cpy.output_file);
    argc           = move(cpy.argc);
    argv           = move(cpy.argv);
    verbose        = move(cpy.verbose);
  }


/*=========------------=============
          PRINT ALL OPTIONS
 -------------======================*/
vector<string> Cmd_Options::print_all_options() {

  vector<string> str_vect;

  str_vect.push_back(string{"blocksize     : " + to_string(blocksize)    });
  str_vect.push_back(string{"offset_start  : " + to_string(offset_start) });
  str_vect.push_back(string{"offset_end    : " + to_string(offset_end)   });
  str_vect.push_back(string{"output_folder : " + output_folder           });
  str_vect.push_back(string{"input         : " + input                   });
  str_vect.push_back(string{"output_file   : " + output_file             });
  str_vect.push_back(string{"carve_files   : " + to_string(carve_files)  });

  return str_vect;
}
