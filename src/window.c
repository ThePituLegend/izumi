/*
 * This file is part of Izumi.
 * 
 * Izumi is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 * 
 * Izumi is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * Izumi. If not, see <https://www.gnu.org/licenses/>. 
 */

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "window.h"
#include "settings.h"
#include "files.h"
#include "parser.h"
#include "config.h"

char *menus[MENU_QTTY] = {
    "Load file",
    "Close file",
    "Quit"
};

void get_window_data(WindowData *data) {
    data->width = getmaxx(stdscr);
    data->height = getmaxy(stdscr);
}

void init_window(WindowData *data) {
    initscr();
    cbreak();
    noecho();
    refresh();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();

    init_pair(1, COLOR_BLACK, COLOR_BLUE);
    init_pair(2, COLOR_BLACK, COLOR_RED);
    init_pair(3, COLOR_BLACK, COLOR_GREEN);
    init_pair(4, COLOR_BLACK, COLOR_YELLOW);
    init_pair(5, COLOR_BLACK, COLOR_MAGENTA);
    init_pair(6, COLOR_BLACK, COLOR_CYAN);

    init_pair(9,  COLOR_WHITE, COLOR_BLUE);
    init_pair(10, COLOR_WHITE, COLOR_RED);
    init_pair(11, COLOR_WHITE, COLOR_GREEN);
    init_pair(12, COLOR_WHITE, COLOR_YELLOW);
    init_pair(13, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(14, COLOR_WHITE, COLOR_CYAN);

    init_pair(17, COLOR_BLUE, COLOR_BLACK);
    init_pair(18, COLOR_RED, COLOR_BLACK);
    init_pair(19, COLOR_GREEN, COLOR_BLACK);
    init_pair(20, COLOR_YELLOW, COLOR_BLACK);
    init_pair(21, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(22, COLOR_CYAN, COLOR_BLACK);


    data->x = 0;
    data->y = 0;

    get_window_data(data);
    data->win = newwin(data->height, data->width, data->y, data->x);
    data->first_instruction = 0;
    data->menu_data = CLOSED;
    data->main_menu.selected = LOAD_FILE;
    data->file_menu.loaded = false;

    data->menu_win = newwin(data->height/2, data->width/2, data->height/4, data->width/4);
}

void close_window() {
    endwin();
}

void main_loop(WindowData *data, InstructionTableArray *tables_array) {
    int ch;

    render(data, tables_array);
    while (1) {
        get_window_data(data);

        ch = getch();

        switch (ch) {
            case 'q':
                if (data->menu_data == MAIN) {
                    data->menu_data = CLOSED;
                }
                else {
                    return;
                }
                break;
            case KEY_DOWN:
                if (data->menu_data == CLOSED) {
                    data->first_instruction++;
                }
                else if (data->menu_data == MAIN) {
                    move_down_menu(&data->main_menu);
                }
                else if (data->menu_data == FILES) {
                    // Moves down on the files menu and updates the init_index
                    if (data->file_menu.files_index < data->file_menu.directory_data.files_qtty - 1) {
                        data->file_menu.files_index++;

                        if (data->file_menu.init_index < data->file_menu.directory_data.files_qtty && data->file_menu.files_index == data->file_menu.init_index + data->height/2 - 4) {
                            data->file_menu.init_index++;
                        }
                    }
                }
                break;
            case KEY_UP:
                if (data->menu_data == CLOSED && data->first_instruction > 0) {
                    data->first_instruction--;
                }
                else if (data->menu_data == MAIN) {
                    move_up_menu(&data->main_menu);
                }
                else if (data->menu_data == FILES) {
                    // Moves up on the files menu and updates the init_index
                    if (data->file_menu.files_index > 0) {
                        data->file_menu.files_index--;

                        if (data->file_menu.init_index > 0 && data->file_menu.files_index == data->file_menu.init_index) {
                            data->file_menu.init_index--;
                        }
                    }
                }
                break;
            case KEY_LEFT:
                if (data->menu_data == FILES) {
                    use_directory(&data->file_menu.directory_data, &data->file_menu.path, "..");

                    data->file_menu.files_index = 0;
                    data->file_menu.init_index = 0;
                }
                break;
            case KEY_RIGHT:
                if (data->menu_data == FILES) {
                    use_directory(&data->file_menu.directory_data, &data->file_menu.path, data->file_menu.directory_data.files[data->file_menu.files_index]);

                    data->file_menu.files_index = 0;
                    data->file_menu.init_index = 0;
                }
                break;
            case '\n': // Enter
                if (data->menu_data == MAIN) {
                    use_menu(data, tables_array);
                }
                else if (data->menu_data == FILES) {
                    if (use_file(&data->file_menu.directory_data, data->file_menu.files_index, &data->file_menu.path, tables_array) == FILE_READ) {
                        data->file_menu.loaded = true;
                        data->menu_data = CLOSED;
                        data->first_instruction = 0;
                    }

                    data->file_menu.files_index = 0;
                    data->file_menu.init_index = 0;
                }
                break;
            case 'm':
                if (data->menu_data == CLOSED) {
                    data->menu_data = MAIN;
                }
                else {
                    data->menu_data = CLOSED;
                }
                break;
            default:
                break;
        }
        render(data, tables_array);
    }
}

void open_menu(WindowData *data) {
    werase(data->menu_win);

    box(data->menu_win, 0, 0);

    mvwprintw(data->menu_win, 1, data->width/4 - 2, "MENU");

    // Print menu options, highlighting the selected one
    if (data->menu_data == MAIN) {
        for (u_int64_t i = 0; i < MENU_QTTY; i++) {
            if (i == data->main_menu.selected) {
                wattron(data->menu_win, A_REVERSE);
                mvwprintw(data->menu_win, i+2, 4, "%s", menus[i]);
                wattroff(data->menu_win, A_REVERSE);
            }
            else {
                mvwprintw(data->menu_win, i+2, 4, "%s", menus[i]);
            }
        }
    }
    else if (data->menu_data == FILES) {
        // Print the files in the directory, highlighting the selected one
        for (u_int64_t i = 0; i < data->file_menu.directory_data.files_qtty; ++i) {
            if (i < data->height/2 - 4) {
                u_int64_t files_index = i + data->file_menu.init_index;
                if (data->file_menu.files_index == files_index) {
                    wattron(data->menu_win, A_REVERSE);

                    if (data->file_menu.directory_data.is_directory[files_index]) {
                        wattron(data->menu_win, COLOR_PAIR(17));
                        mvwprintw(data->menu_win, i+2, 4, "%s", data->file_menu.directory_data.files[files_index]);
                        wattroff(data->menu_win, COLOR_PAIR(17));
                    }
                    else {
                        mvwprintw(data->menu_win, i+2, 4, "%s", data->file_menu.directory_data.files[files_index]);
                    }
                    wattroff(data->menu_win, A_REVERSE);
                }
                else {
                    if (data->file_menu.directory_data.is_directory[files_index]) {
                        wattron(data->menu_win, COLOR_PAIR(17));
                        mvwprintw(data->menu_win, i+2, 4, "%s", data->file_menu.directory_data.files[files_index]);
                        wattroff(data->menu_win, COLOR_PAIR(17));
                    }
                    else {
                        mvwprintw(data->menu_win, i+2, 4, "%s", data->file_menu.directory_data.files[files_index]);
                    }
                }
            }
        }

        // Print the scroll indicators
        if (data->file_menu.directory_data.files_qtty > data->height/2 - 4 && data->file_menu.init_index + data->height/2 - 4 < data->file_menu.directory_data.files_qtty) {
            mvwprintw(data->menu_win, data->height/2 - 2, data->width/2 - 4, "v");
        }
        if (data->file_menu.init_index > 0) {
            mvwprintw(data->menu_win, 2, data->width/2 - 4, "^");
        }
    }

    wrefresh(data->menu_win);
}

void render(WindowData *data, InstructionTableArray *tables_array) {
    // clear screen
    werase(data->win);

    box(data->win, 0, 0);

    mvwprintw(data->win, 0, 0, "Izumi v%s", VERSION);

    // verical bar on 32 px for instruction info
    mvwvline(data->win, 1, 32, ACS_VLINE, data->height - 2);

    if (data->file_menu.loaded) {
        // print instructions data

        u_int64_t first_cycle = -1;

        //build grid
        for (u_int64_t i = 1; i < data->height - 1; i++) {
            for (u_int64_t j = 0; j < data->width - 1; j++) {
                if (j >32 && j%3 == 0) {
                    mvwprintw(data->win, i, j, "|");
                }
            }
        }

        // print instructions
        for (u_int64_t i = 0; i < data->height/2; i++) {
            u_int64_t index = data->first_instruction + i;
            Instruction instr = tables_array->tables[index/256]->content[index%256];

            // memory address and instruction
            if (instr.valid) {
                if (instr.mem_addr != NULL && (2*i + 1) < data->height - 1) {
                    mvwprintw(data->win, 2*i + 1, 1, "%lu\t%s", index, instr.mem_addr);
                }
                if (instr.instruction != NULL && (2*i + 2) < data->height - 1) {
                    mvwprintw(data->win, 2*i + 2, 5, "%s", instr.instruction);
                }

                // stages
                if ((2*i + 2) < data->height - 1) {
                    for (u_int64_t j = 0; j < instr.qtty_stages; j++) {
                        if (instr.stages[j].cycle < first_cycle) {
                            first_cycle = instr.stages[j].cycle;
                        }
                        Stage *stage = &instr.stages[j];

                        wattron(data->win, A_BOLD);

                        if (33 + 3*(stage->cycle - first_cycle) + 3 < data->width - 1) {
                            mvwprintw(data->win, 2*i + 2, 33 + 3*(stage->cycle - first_cycle), "|");
                            wattron(data->win, COLOR_PAIR((j%6)+1));
                            mvwprintw(data->win, 2*i + 2, 33 + 3*(stage->cycle - first_cycle) + 1, "%s", stage->name);

                            if (strlen(stage->name) == 1) {
                                mvwprintw(data->win, 2*i + 2, 33 + 3*(stage->cycle - first_cycle) + 2, " ");
                            }
                            wattroff(data->win, COLOR_PAIR((j%6)+1));
                        }

                        // extra cycles
                        if (stage->duration > 1) {
                            for (u_int64_t k = 0; k < stage->duration - 1; k++) {
                                if (33 + 3*(stage->cycle - first_cycle) + 3*(k+1) < data->width - 4) {
                                    wattron(data->win, COLOR_PAIR(8+(j%6)+1));
                                    mvwprintw(data->win, 2*i + 2, 33 + 3*(stage->cycle - first_cycle) + 3*(k+1), "|");
                                    wattroff(data->win, COLOR_PAIR(8+(j%6)+1));

                                    wattron(data->win, COLOR_PAIR((j%6)+1));
                                    mvwprintw(data->win, 2*i + 2, 33 + 3*(stage->cycle - first_cycle) + 3*(k+1) + 1, "  ");
                                    wattroff(data->win, COLOR_PAIR((j%6)+1));

                                }
                            }
                        }

                        wattroff(data->win, A_BOLD);
                    }
                }

            }
        }
    }

    wrefresh(data->win);

    if (data->menu_data == MAIN || data->menu_data == FILES) {
        open_menu(data);
    }
}
