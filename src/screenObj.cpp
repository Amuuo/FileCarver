#include "screenObj.h"


ScreenObj::ScreenObj() {}

void ScreenObj::init() {


    title_art.push_back("                                   ");
    title_art.push_back(" '||''''| '||' '||'      '||''''|  ");
    title_art.push_back("  ||  .    ||   ||        ||  .    ");
    title_art.push_back("  ||''|    ||   ||        ||''|    ");
    title_art.push_back("  ||       ||   ||        ||       ");
    title_art.push_back(" .||.     .||. .||.....| .||.....| ");
    title_art.push_back("                                                              ");
    title_art.push_back("   ..|'''.|     |     '||''|.   '||'  '|' '||''''|  '||''|.   ");
    title_art.push_back(" .|'     '     |||     ||   ||   '|.  .'   ||  .     ||   ||  ");
    title_art.push_back(" ||           |  ||    ||''|'     ||  |    ||''|     ||''|'   ");
    title_art.push_back(" '|.      .  .''''|.   ||   |.     |||     ||        ||   |.  ");
    title_art.push_back("   '|....'  .|.  .||. .||.  '|'     |     .||.....| .||.  '|' ");
    title_art.push_back("                                                              ");

    title_width = title_art.back().size();
    win = initscr();
    start_color();
    // resizeterm(25, 50);

    clear();
    curs_set(0);
    box(stdscr, 0, 0);
    getmaxyx(win, row, col);
    screen_center_row = row / 3;
    screen_center_col = col / 2;

    init_pair(1, COLOR_WHITE, COLOR_RED);
    init_pair(2, COLOR_WHITE, COLOR_WHITE);
    init_pair(3, COLOR_BLACK, COLOR_GREEN);
    init_pair(4, COLOR_BLACK, COLOR_YELLOW);

    attron(COLOR_PAIR(1) | A_BOLD);
    for(int i = 0; i < title_art.size(); ++i){
        oss << title_art[i];
        print_centered_string((screen_center_row - 10) + i);
    }
    attroff(COLOR_PAIR(1) | A_BOLD);
    title_ending_row = getcury(win);
    refresh_screen();
    attron(COLOR_PAIR(2));
    mvprintw(title_ending_row + 6,
             screen_center_col - (static_cast<int>(title_width / 2)),
             string(static_cast<int>(title_width), ' ').c_str());
    attroff(COLOR_PAIR(2));
    refresh();
}





ScreenObj::~ScreenObj() {
    resetty();
    clear();
    endwin();
}

void ScreenObj::print_latest_find(string latest_find) {

    oss << latest_find;
    attron(A_UNDERLINE | A_BOLD);
    print_centered_string(title_ending_row + 9);
    attroff(A_UNDERLINE | A_BOLD);
}

void ScreenObj::print_str_vec()
{
    for (int i = 0; i < opts.size(); ++i){
        mvprintw(title_ending_row + 12 + i, screen_center_col - 26, "%s", opts[i].c_str());
        refresh();
    }
}




int ScreenObj::half_len() { return oss.str().size()/2;}
int ScreenObj::half_len(string& line) { return line.size() / 2; }





void ScreenObj::print_centered_string(int row) {

    mvprintw(row, screen_center_col - half_len(), "%s",
                oss.str().c_str(), NULL);
    oss.str("");
}





void ScreenObj::print_centered_string(int row, string& line){
    mvprintw(row, screen_center_col - half_len(line), "%s",
                line.c_str(), NULL);
}





void ScreenObj::update_found_counter() {

  flash();
  ++file_counter;
  refresh_screen();
}





void ScreenObj::update_scan_counter(disk_pos&& i) {

    blocks_scanned = i;
    refresh_screen();
}





void ScreenObj::refresh_screen() {
    attron(COLOR_PAIR(3) | A_BOLD);
    oss << " Files found: " << file_counter << " ";
    print_centered_string(title_ending_row +4);
    attroff(COLOR_PAIR(3) | A_BOLD);

    refresh();
}





void ScreenObj::refresh_progress_bar() {

  attron(COLOR_PAIR(3));
  mvprintw(
      title_ending_row + 6,
      screen_center_col - (static_cast<int>(title_width / 2)),
      string(static_cast<int>(title_width * progress_counter), ' ').c_str());
  attroff(COLOR_PAIR(3));

  oss << (boost::format("%-3.2f%%") % (progress_counter * 100.0f)).str();

  print_centered_string(title_ending_row + 7);
  refresh();
}
