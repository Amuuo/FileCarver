#include "cmd_options.h"


Cmd_Options::Cmd_Options(){}


Cmd_Options::Cmd_Options(int _argc,
                         char** _argv,
                         bool v,
                         disk_pos b,
                         disk_pos os,
                         disk_pos oe,
                         bool c) :
    argc{_argc},
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





Cmd_Options::Cmd_Options(int argc, char** argv) {


  static struct option long_options[] =
  {
    {"blocksize",     required_argument, NULL, 'b'},
    {"offset_start",  required_argument, NULL, 'S'},
    {"offset_end",    required_argument, NULL, 'E'},
    {"input",         required_argument, NULL, 'i'},
    {"output_folder", required_argument, NULL, 'o'},
    {"carve_files",   no_argument,       NULL, 'c'},
    {"graphical",     no_argument,       NULL, 'G'},
    {"verbose",       no_argument,       NULL, 'v'},
    {"help",          no_argument,       NULL, 'h'},
    {NULL, 0, NULL, 0}
  };

  int           cmd_option;
  extern char*  optarg;

  while ((cmd_option = getopt_long(argc, argv, "b:S:E:s:i:o:cGvh",
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

      case 'G': startup_screen();               break;

      case 'h':
        printf (
        "\n\t--blocksize      -b  <size of logical blocks in bytes>"
        "\n\t--offset_start   -S  <offset from start of disk in bytes>"
        "\n\t--offset_end     -E  <offset from start of disk in bytes>"
        "\n\t--input          -i  <file to use for header search>"
        "\n\t--output_folder  -o  <folder to use for output files>"
        "\n\t--carve_files    -c  <write found files to current directory>"
        "\n\t--graphical      -G  <set program parameters through graphical interface"
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

  filesize = offset_end - offset_start;

  if (input == "")
  {
    printf("\n>> Input file required");
    exit(1);
  }
}














/*=========------------=============
          PRINT ALL OPTIONS
 -------------======================*/
inline vector<string> Cmd_Options::print_all_options() {

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











/*=========------------=============
           STARTUP_SCREEN
 -------------======================*/
void Cmd_Options::startup_screen(){

  auto print_in_middle = [](WINDOW *win, int starty, int startx, int width,
                             char *string, chtype color) {
    int length, x, y;
    float temp;

    if (win == NULL) win = stdscr;
    getyx(win, y, x);
    if (startx != 0) x = startx;
    if (starty != 0) y = starty;
    if (width == 0) width = 80;

    length = strlen(string);
    temp = (width - length) / 2;
    x = startx + (int)temp;
    wattron(win, color);
    mvwprintw(win, y, x, "%s", string);
    wattroff(win, color);
    refresh();
  };


  const int INSTRUCTION_1      = 0;
  const int INSTRUCTION_2      = 1;
  const int PATH_TO_DISK_LABEL = 2;
  const int PATH_TO_DISK       = 3;
  const int OFFSET_START_LABEL = 4;
  const int OFFSET_START       = 5;
  const int OFFSET_END_LABEL   = 6;
  const int OFFSET_END         = 7;
  const int BLOCKSIZE_LABEL    = 8;
  const int BLOCKSIZE          = 9;
  const int FINAL_FIELD        = 10;


  FIELD  *field[11];
  FORM   *my_form;
  WINDOW *my_form_win;
  int ch, rows, cols;

  /* Initialize curses */
  initscr();
  start_color();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);

  /* Initialize few color pairs */
  init_pair(1, COLOR_BLUE, COLOR_BLACK);



  /* Initialize the fields */
  field[INSTRUCTION_1]      = new_field(1, 25, 1, 1, 0, 0);
	field[INSTRUCTION_2]      = new_field(1, 25, 2, 1, 0, 0);
  field[PATH_TO_DISK_LABEL] = new_field(1, 25, 5, 1, 0, 0);
  field[PATH_TO_DISK]       = new_field(1, 25, 6, 1, 0, 0);
  field[OFFSET_START_LABEL] = new_field(1, 25, 8, 1, 0, 0);
  field[OFFSET_START]       = new_field(1, 25, 9, 1, 0, 0);
  field[OFFSET_END_LABEL]   = new_field(1, 25, 11, 1, 0, 0);
  field[OFFSET_END]         = new_field(1, 25, 12, 1, 0, 0);
  field[BLOCKSIZE_LABEL]    = new_field(1, 25, 14, 1, 0, 0);
  field[BLOCKSIZE]          = new_field(1, 25, 15, 1, 0, 0);
  field[FINAL_FIELD]        = NULL;

  /* Set field options */

  auto set_label = [&](const int tag, const char *label) {
    set_field_back  (field[tag], A_ITALIC);
    set_field_buffer(field[tag], 0, label);
    set_field_just  (field[tag], JUSTIFY_CENTER);
    set_field_opts  (field[tag], O_AUTOSKIP | O_VISIBLE | O_PUBLIC);
  };
  auto set_field = [&](const int tag_label, const char *label,
                              const char* default_value, const int tag) {
    set_label(tag_label, label);
    set_field_buffer(field[tag], 0, default_value);
    set_field_back(field[tag], A_COLOR | A_BOLD);
    set_field_just(field[tag], JUSTIFY_CENTER);
    set_field_opts(field[tag], O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE);
  };

  set_label(INSTRUCTION_1,      "(KEY_LEFT to clear field)");
  set_label(INSTRUCTION_2,      "(F1 to submit)");

  set_field(PATH_TO_DISK_LABEL, "Path To Disk (input source)", "/dev/sdc1\0",
            PATH_TO_DISK);
  set_field(OFFSET_START_LABEL, "Offset Start (in Bytes)", "0", OFFSET_START);
  set_field(OFFSET_END_LABEL, "Offset End (in Bytes)", "0", OFFSET_END);
  set_field(BLOCKSIZE_LABEL, "Offset End (in Bytes)", "32768", BLOCKSIZE);



  my_form = new_form(field);
  scale_form(my_form, &rows, &cols);
  my_form_win = newwin(rows + 5, cols + 5, (LINES / 2) - ((rows + 5) / 2),
                       (COLS / 2) - ((cols + 5) / 2));
  keypad(my_form_win, TRUE);
  set_form_win(my_form, my_form_win);
  set_form_sub(my_form, derwin(my_form_win, rows, cols, 2, 2));

  box(stdscr, 0, 0);
  box(my_form_win, 0, 0);
  print_in_middle(my_form_win, 0, 1, cols + 4, " C A R V E  O P T I O N S ",
                  COLOR_PAIR(1));

  post_form(my_form);
  wrefresh(my_form_win);

  mvprintw(LINES - 2, 0, "Use UP, DOWN arrow keys to switch between fields");
  refresh();


  while ((ch = wgetch(my_form_win)) != /*KEY_F(1)*/KEY_RIGHT) {
    switch (ch) {
      case KEY_DOWN:
        form_driver(my_form, REQ_NEXT_FIELD);
        form_driver(my_form, REQ_END_LINE);
        break;
      case KEY_UP:
        form_driver(my_form, REQ_PREV_FIELD);
        form_driver(my_form, REQ_END_LINE);
        break;
      case KEY_LEFT:
        form_driver(my_form, REQ_CLR_FIELD);
        break;
      case KEY_BACKSPACE:
        form_driver(my_form, REQ_CLR_FIELD);
        break;
      default:
        form_driver(my_form, ch);
        break;
    }
	}
  form_driver(my_form, REQ_END_LINE);
	unpost_form(my_form);
	free_form(my_form);
	endwin();


  input        = (char*)field[PATH_TO_DISK]->buf;
  offset_start = atoll((char *)field[OFFSET_START]->buf);
  offset_end   = atoll((char *)field[OFFSET_END]->buf);
  blocksize    = atoll((char *)field[BLOCKSIZE]->buf);

  while(isspace(input.back()))
    input.pop_back();
  /*
  for (int i = 0; i < FINAL_FIELD; ++i)
    free_field(field[i]);
    */

}
