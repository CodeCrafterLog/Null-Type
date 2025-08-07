#include "typing.h"
#include "sounds.h"
#include <vibend.h>
#include <sstream>
#include <iomanip>
#include <thread>
#include <iostream>

unsigned char enter = 170;

using namespace Vibend::Literals;

void printSymbol(char ch)
{
    Vibend::print(ch == '\n' ? enter : ch);
}

std::string formatDuration(std::chrono::duration<double> duration) {
    using namespace std::chrono;

    auto totalSeconds = duration.count();
    auto hours = static_cast<int>(totalSeconds / 3600);
    totalSeconds -= hours * 3600;
    auto minutes = static_cast<int>(totalSeconds / 60);
    totalSeconds -= minutes * 60;
    auto seconds = static_cast<int>(totalSeconds);
    auto milliseconds = static_cast<int>((totalSeconds - seconds) * 1000);

    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << hours << ":"
        << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << seconds << "."
        << std::setw(2) << std::setfill('0') << milliseconds;

    return oss.str();
}

uint8_t getAndEcho(char correct)
{
    char ch = Vibend::getch(false);

    if (ch == NULL) // No input, quick exit
        return NO_INPUT;

    if (ch == 27)
        return EXIT;

    // Replace 13 (user input enter) with 10 (console enter)
    if (ch == 13)
        ch = '\n';

    bool iscorrect = ch == correct;

    // Green if you type corret, else red
    Vibend::Color color = iscorrect ? Vibend::Color::GREEN : Vibend::Color::RED;

    Vibend::setbackground(color);

    // Echo
    printSymbol(correct);

    // What it sounds to do
    Vibend::resetbackground();

    // Return the success of result and duration
    return iscorrect ? CORRECT : WRONG;
}

uint8_t waitForInput(char totype, std::chrono::steady_clock::time_point& previostime, bool forceType)
{
    using namespace std::chrono;
    uint8_t result = NO_INPUT;
    auto now = steady_clock::now();

    while (result == NO_INPUT) {
        result = getAndEcho(totype);
        now = steady_clock::now();

        if (result == EXIT)
            break;

        if (!forceType && duration_cast<seconds>(now - previostime).count() >= 1)
            break;
    }

    return result;
}

void drawStar()
{
    Vibend::print(R"(    A    )"); Vibend::move(1, -9);
    Vibend::print(R"(___/_\___)"); Vibend::move(1, -9);
    Vibend::print(R"( ',. ..' )"); Vibend::move(1, -9);
    Vibend::print(R"( /.'^'.\ )"); Vibend::move(1, -9);
    Vibend::print(R"(/'     '\)"); Vibend::move(1, -9);
}

void printcurrent(char current)
{
    // Draw the next character
    Vibend::setformat(Vibend::Format::REVERSE);
    printSymbol(current);
    Vibend::resetstyle();
    Vibend::movecols(-1);
}

void showDetails(std::chrono::duration<double> timepassed, int mistakes, int typed)
{
    using namespace Vibend::Literals;

    auto time = timepassed.count();
    double wpm = time == 0 ? 0 : ((typed - mistakes) / 5.0) * (60.0 / time);
    double acc = typed == 0 ? 100 : (typed - mistakes) * 100.0 / typed;

    constexpr uint16_t w = 10 * 3 - 1;
    constexpr uint16_t h = 5 + 4 + 2;
    
    uint16_t x = (100_vw - w) / 2;
    uint16_t y = (100_vh - h) / 2;

    Vibend::Box box(x, y, w, h);

    Vibend::Panel(box.outer().outer()).draw();

    Vibend::setforeground(Vibend::Color::RED);
    Vibend::teleport(y, x);
    drawStar();
    Vibend::teleport(y, x + 10);
    drawStar();
    Vibend::teleport(y, x + 20);
    drawStar();

    Vibend::setforeground(Vibend::Color::YELLOW);
    if (acc > 50) {
        Sleep(500);
        Sounds::playStar(0);
        Vibend::teleport(y, x);
        drawStar();
    }

    if (acc > 75) {
        Sleep(500);
        Sounds::playStar(1);
        Vibend::teleport(y, x + 10);
        drawStar();
    }

    if (acc > 95) {
        Sleep(500);
        Sounds::playStar(2);
        Vibend::teleport(y, x + 20);
        drawStar();
    }

    Sleep(1000);

    Vibend::setforeground(Vibend::Color::CYAN);

    std::stringstream details;
    details << "You have typed " << typed << " characters in " << formatDuration(timepassed)
        << " with " << mistakes << " mistakes and " << std::fixed << std::setprecision(2) << acc
        << "% accuracy" << " and speed of " << (uint16_t)wpm << " words per minute!\n"
        << "Press [enter] to continue";

    Vibend::TextBox detailstext(box, details.str().c_str());
    detailstext.box.y += 6;
    detailstext.draw();

    Sounds::playEnd();

    while (Vibend::getch() != 13);
}

std::string getStatusText(std::chrono::duration<double> timepassed, int mistakes, int typed)
{
    std::stringstream txt;

    auto time = timepassed.count();
    double wpm = time == 0 ? 0 : ((typed - mistakes) / 5.0) * (60.0 / time);
    double acc = typed == 0 ? 100 : (typed - mistakes) * 100.0 / typed;

    txt << "Press [enter] whenever you see'" << enter << "'" << std::endl
        << "Mistakes: " << mistakes << std::endl
        << "Typed: " << typed << std::endl
        << "Time passed: " << (uint16_t)time << "s" << std::endl
        << "WPM: " << (uint16_t)wpm << std::endl
        << "Acc: " << std::fixed << std::setprecision(1) << acc << "%  " << std::endl;

    return txt.str();
}

void updateStatusText(Vibend::TextBox& status, std::string& newtext)
{
    // Delete last text
    delete[] status.text;

    // Reserve new text
    status.text = new char[newtext.size() + 1];
    strcpy_s(status.text, newtext.size() + 1, newtext.c_str());

    // Check and draw
    status.checkbreaks();
    status.draw();
}

std::chrono::duration<double> takeTest(const char* text)
{
    using namespace std::chrono;

    Vibend::TextBox textbox(Vibend::Box(25_vw, 25_vh, 50_vw, 50_vh), text);
    textbox.enter = enter;
    textbox.draw();
    Vibend::Panel textboxpanel(textbox.box.outer());
    textboxpanel.drawborder();

    time_point<high_resolution_clock> starttime;
    time_point<steady_clock> previostime;

    std::string statustext = "Hello!";
    Vibend::TextBox status(Vibend::Box(2, textbox.box.y, textbox.box.x - 3, textbox.box.h), statustext.c_str());
    status.draw();

    Vibend::Panel statuspanel(status.box.outer());
    statuspanel.drawborder();

    Vibend::ProgressBar progressbar(
        Vibend::Box(textbox.box.x, textbox.box.y - 4, textbox.box.w, 1),
        '|', ' ', ' ',
        []() {Vibend::setforeground(Vibend::Color::GREEN);},
        []() {Vibend::resetforeground();}
    );

    Vibend::Panel(progressbar.box.outer(), ' ', ' ', '[', ']', ' ', ' ', ' ', ' ').drawborder();

    uint16_t textsize = (uint16_t)std::strlen(text);
    uint16_t typedsize = 0;
    uint8_t lastprogress = 0;

    uint16_t mistakes = 0;
    uint8_t page = 0;
    bool lastwrong = false;

    for (uint16_t y = 0; y < textbox.lines; y++) {
        if (y - page * textbox.box.h >= textbox.box.h) {
            page++;
            textboxpanel.drawcenter();
            textbox.draw(page * textbox.box.h);
        }

        Vibend::teleport(textbox.box.y + y - page * textbox.box.h, textbox.box.x);

        bool islastline = y == textbox.breaks->size();
        uint16_t linesize = islastline ? -1 : textbox.breaks->at(y);
        for (uint16_t x = 0;*(text + typedsize) && x < linesize;) {
            char current = *(text + typedsize);
            auto now = steady_clock::now();

            printcurrent(current);

            Vibend::savepos();

            duration<double> time = duration<double>(0);

            bool isfirstkey = starttime == high_resolution_clock::time_point();
            // Show the real duration if the test is started 
            if (!isfirstkey)
                time = duration_cast<seconds>(now - starttime);

            std::string statustext = getStatusText(time, mistakes, typedsize + lastwrong);
            updateStatusText(status, statustext);

            uint8_t progress = typedsize * 100 / textsize;

            if (lastprogress != progress) {
                progressbar.progress = progress / 100.0f;
                progressbar.draw();
                lastprogress = progress;
            }

            Vibend::loadpos();

            uint8_t result = waitForInput(current, previostime, typedsize == 0);

            now = steady_clock::now();

            if (result == EXIT) {
                y = -1;
                break;
            }

            if (result == WRONG) {
                if (!lastwrong)
                    mistakes++;

                Vibend::movecols(-1);
                lastwrong = true;
                Sounds::playWrong();

            }

            if (result == CORRECT) {
                Sounds::playCorrect();
                if (lastwrong) {
                    lastwrong = false;
                    Vibend::movecols(-1);
                    Vibend::setbackground(Vibend::Color::RED);
                    printSymbol(current);
                    Vibend::resetbackground();
                }
                typedsize++;
                x++;
            }
            else {
                previostime = now;
            }

            // Set the timer if this was the first letter to type
            if (isfirstkey) {
                starttime = high_resolution_clock::now();
                previostime = now;
            }
        }

        if (y == (uint16_t)-1)
            break;
    }

    if (starttime == high_resolution_clock::time_point()) {
        return duration<double>(0);
    }

    auto endtime = high_resolution_clock::now();
    duration<double> duration = endtime - starttime;

    showDetails(duration, mistakes, typedsize);

    return duration;
}

std::chrono::duration<double> takeWords(std::vector<std::string>& words)
{
    // TODO Implement streaks
    using namespace Vibend::Literals;

    Vibend::Box wordBox(50_vw, 50_vh, max(10_vw, 20), 1);
    wordBox.x -= wordBox.w / 2;

    Vibend::Panel panel(wordBox.outer());
    panel.drawborder();

    using namespace std::chrono;

    time_point<high_resolution_clock> startTime;
    int wordsCount = words.size();

    std::string currentWord = "";
    uint8_t currentIndex = 0;

    uint8_t input = NO_INPUT;
    // Change this to uint32_t if you are better
    uint16_t streak = 0;
    uint16_t bestStreak = 0;
    auto now = high_resolution_clock::now();
    srand((unsigned int)(now- high_resolution_clock::time_point()).count());

    while (currentIndex >= currentWord.size() || (input = waitForInput(currentWord[currentIndex], now, false)) != EXIT) // Exit if user asks
    {
        now = high_resolution_clock::now();
        if (currentIndex >= currentWord.size())
        {
            input = NO_INPUT;
            currentIndex = 0;
            currentWord = words[rand() % wordsCount];
            panel.drawcenter();
            uint16_t x = wordBox.x + (wordBox.w - (uint16_t)currentWord.size()) / 2;
            Vibend::teleport(wordBox.y, x);
            Vibend::print(currentWord.c_str());

            // Check if this was the loading or not
            if (startTime != time_point<high_resolution_clock>()) {
                Sounds::playEnd();
                streak++; // Well Done!
                // Update the best streak
                bestStreak = max(bestStreak, streak);
            }
            else // It is loading
                startTime = now;

            Vibend::teleport(wordBox.y, x);
            printcurrent(currentWord[currentIndex]);
        }

        /* Draw details */ {
            Vibend::savepos();
            // Some additional spaces to override the last text
            std::string overrideSpaces(10, ' ');
            std::stringstream ss;
            ss  << overrideSpaces
                << "Streak: " << streak
                << " | Best: " << bestStreak
                << " | Time: " << duration_cast<seconds>(now - startTime).count() << 's'
                << overrideSpaces;

            Vibend::teleport(wordBox.y - 3, 50_vw - (uint16_t)ss.str().size() / 2);
            Vibend::print(ss.str().c_str());
            Vibend::loadpos();
        }

        if (input == NO_INPUT)
            continue;

        if (input == CORRECT) {
            currentIndex++;
            printcurrent(currentWord[currentIndex]);
            Sounds::playCorrect();
            continue;
        }

        // All cases passed else than WRONG:
        // Sorry for player, the streak has been deleted
        streak = 0;
        Sounds::playWrong(); // Avoid this sound if you can
        Vibend::movecols(-1);
    }

    auto endTime = high_resolution_clock::now();
    return endTime - startTime;
}
