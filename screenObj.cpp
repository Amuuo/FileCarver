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



    win = initscr();
    // resizeterm(25, 50);

    init_pair(2, COLOR_BLACK, COLOR_BLACK);


    clear();
    start_color();
    curs_set(0);
    box(stdscr, 0, 0);
    getmaxyx(win, row, col);
    screen_center_row = row / 3;
    screen_center_col = col / 2;

    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(3, COLOR_BLACK, COLOR_GREEN);
    init_pair(4, COLOR_BLACK, COLOR_YELLOW);
    attron(COLOR_PAIR(1));
    for(int i = 0; i < title_art.size(); ++i){
        oss << title_art[i];
        print_centered_string((screen_center_row - 6) + i);
    }
    attroff(COLOR_PAIR(1));
    title_ending_row = getcury(win);
    refresh_screen();
    refresh();
}

ScreenObj::~ScreenObj() {
    resetty();
    clear();
    endwin();
}

void ScreenObj::print_str_vec()
{
    for (int i = 0; i < opts.size(); ++i){
        mvprintw(title_ending_row + 11 + i, screen_center_col - 16, "%s", opts[i].c_str());
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
    attron(COLOR_PAIR(3));
    oss << " Files found: " << file_counter << " ";
    print_centered_string(title_ending_row +4);
    attroff(COLOR_PAIR(3));
    attron(COLOR_PAIR(4));
    oss << " MB scanned: " << blocks_scanned/mb_size << " ";
    print_centered_string(title_ending_row +6);

    oss << " GB scanned: " << blocks_scanned/gb_size << " ";
    print_centered_string(title_ending_row+7);
    attroff(COLOR_PAIR(4));
    refresh();
}
