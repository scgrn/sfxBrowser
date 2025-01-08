#include <iostream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <ncurses.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

std::vector<std::string> getWavFiles(std::filesystem::path path) {
    std::vector<std::string> wavFiles;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.path().extension() == ".wav") {
            wavFiles.push_back(entry.path().filename().string());
        }
    }
    return wavFiles;
}

void displayMenu(WINDOW* menuWin, const std::vector<std::string>& files, int selected, int scrollOffset) {
    int yMax, xMax;
    getmaxyx(menuWin, yMax, xMax);
    box(menuWin, 0, 0);
    mvwprintw(menuWin, 0, 2, " Select a .wav file ");

    int maxDisplay = yMax - 2;
    for (int i = 0; i < maxDisplay && i + scrollOffset < static_cast<int>(files.size()); ++i) {
        if (i + scrollOffset == selected) {
            wattron(menuWin, A_REVERSE);
            mvwprintw(menuWin, i + 1, 1, files[i + scrollOffset].c_str());
            wattroff(menuWin, A_REVERSE);
        } else {
            mvwprintw(menuWin, i + 1, 1, files[i + scrollOffset].c_str());
        }
    }
    wrefresh(menuWin);
}

void playSound(ma_engine &engine, std::string path) {
    ma_result result;
    ma_sound sound;

    result = ma_sound_init_from_file(&engine, path.c_str(), 0, NULL, NULL, &sound);
    if (result != MA_SUCCESS) {
        std::cout << "Unable to load sound." << std::endl;
        return;
    }

    ma_sound_start(&sound);
    while (!ma_sound_at_end(&sound));
    ma_sound_uninit(&sound);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> wavFiles;
    std::string path = "./";

    ma_result result;
    ma_engine engine;
    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        std::cout << "Failed to initialize audio engine." << std::endl;
        return -1;
    }

    if (argc >= 2) {
        if (std::filesystem::is_directory(argv[1])) {
            wavFiles = getWavFiles(argv[1]);
            path = std::string(argv[1]) + "/";
        } else {
            playSound(engine, argv[1]);
            ma_engine_uninit(&engine);
            return 0;
        }
    } else {
        wavFiles = getWavFiles(std::filesystem::current_path());
    }
    if (wavFiles.empty()) {
        std::cout << "No .wav files found. Check path and try again." << std::endl;
        ma_engine_uninit(&engine);
        return 0;
    }
    std::sort(wavFiles.begin(), wavFiles.end());

    //  init ncurses
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);

    int selected = 0;
    int scrollOffset = 0;
    int inputKey;

    int yMax, xMax;
    getmaxyx(stdscr, yMax, xMax);
    int maxDisplay = yMax - 2;

    WINDOW* menuWin = newwin(yMax, xMax, 0, 0);
    displayMenu(menuWin, wavFiles, selected, scrollOffset);

    while (true) {
        inputKey = wgetch(menuWin);

        if (inputKey == 27) {
            nodelay(menuWin, TRUE);     //  disable blocking input
            int nextKey = wgetch(menuWin);
            if (nextKey == -1) {
                break;
            } else if (nextKey == '[') {
                int arrowKey = wgetch(menuWin);
                //  up arrow 
                if (arrowKey == 'A') {
                    if (selected > 0) {
                        selected--;
                        if (selected < scrollOffset) {
                            scrollOffset--;
                            if (scrollOffset < 0) {
                                scrollOffset = 0;
                            }
                        }
                    }
                }
                //  down arrow
                if (arrowKey == 'B') {
                    if (selected < static_cast<int>(wavFiles.size()) - 1) {
                        selected++;
                        if (selected >= scrollOffset + maxDisplay) {
                            scrollOffset++;
                            if (scrollOffset > static_cast<int>(wavFiles.size()) - maxDisplay) {
                                scrollOffset = std::max(0, static_cast<int>(wavFiles.size()) - maxDisplay);
                            }
                        }
                    }
                }
           }
            nodelay(menuWin, FALSE);    //  restore blocking input
        } else if (inputKey == 10) {    //  enter
            ma_engine_play_sound(&engine, (path + wavFiles[selected]).c_str(), NULL);
        }

        displayMenu(menuWin, wavFiles, selected, scrollOffset);
    }

    ma_engine_uninit(&engine);
    endwin();

    return 0;
}
