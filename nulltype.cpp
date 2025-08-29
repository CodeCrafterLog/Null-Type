#include <vibend.h>
#include <string>
#include <iostream>
#include <fstream>

#include "typing.h"
#include "resource.h"
#include "api.h"
#include "constants.hpp"

namespace Resources
{
    std::vector<std::string> wordslvls[3];

    constexpr uint8_t OK                    = 0;
    constexpr uint8_t RESOURCE_NOT_FOUND    = 1;
    constexpr uint8_t RESOURCE_LOAD_FAIL    = 2;
    constexpr uint8_t EMPTY                 = 3;

    // Load a text resource and save it into 'result' refrence
    uint8_t loadTextResource(int resourceID, std::vector<std::string>& result) {
        // Get handle to the resource
        HRSRC hResource = FindResource(nullptr, MAKEINTRESOURCE(resourceID), L"TEXT");
        if (!hResource) {
            return Resources::RESOURCE_NOT_FOUND;
        }

        // Load the resource
        HGLOBAL hLoadedResource = LoadResource(nullptr, hResource);
        if (!hLoadedResource) {
            return Resources::RESOURCE_LOAD_FAIL;
        }

        // Lock the resource to get a pointer to its data
        LPVOID pResourceData = LockResource(hLoadedResource);
        DWORD resourceSize = SizeofResource(nullptr, hResource);

        if (!(pResourceData && resourceSize > 0))
            return Resources::EMPTY;

        std::string content(static_cast<char*>(pResourceData), resourceSize);
        size_t estimatedLines = std::count(content.begin(), content.end(), '\n') + 1;
        result.reserve(estimatedLines);

        // Split content into lines
        size_t pos = 0, last = 0;
        while ((pos = content.find('\n', last)) != std::string::npos) {
            result.push_back(content.substr(last, pos - last));
            last = pos + 1;
        }
        if (last < content.size()) {
            result.push_back(content.substr(last));
        }

        return Resources::OK;
    }
}

// Inputs a single line of code
std::string inputLine() {
    using namespace Vibend::Literals;

    std::string input;

    char ch;
    while ((ch = Vibend::getch()) != 13)
    {
        if (ch == '\x1b') // Escape
            return "";

        if (ch != '\b') {
            Vibend::print(ch);
            input += ch;
            continue;
        }

        if (!input.size())
            continue;

        Vibend::movecols(-1);
        Vibend::print(' ');
        Vibend::movecols(-1);
        input.pop_back();
        continue;
    }

    return input;
}

void printTitle()
{
    using namespace Vibend::Literals;
    Vibend::teleport(10_vh, 50_vw - MainMenu::TITLE_CENTER);
    Vibend::print(MainMenu::TITLE);
}

int init()
{
    // Load Resources
    // TODO Debug errors
    Resources::loadTextResource(IDR_TEXT1, Resources::wordslvls[0]);
    Resources::loadTextResource(IDR_TEXT2, Resources::wordslvls[1]);
    Resources::loadTextResource(IDR_TEXT3, Resources::wordslvls[2]);

    // Initialize Vibend
    Vibend::init();
    Vibend::setcursor(false);

    // Set options labels
    MainMenu::options.resize(MainMenu::Options::COUNT);
    MainMenu::options[MainMenu::Options::EXAMPLE] = "Example text";
    MainMenu::options[MainMenu::Options::AI] = "Type AI text";
    MainMenu::options[MainMenu::Options::WORDS] = "Words";
    MainMenu::options[MainMenu::Options::CREDITS] = "Credits";
    MainMenu::options[MainMenu::Options::EXIT] = "Exit";

    using namespace Vibend::Literals;

#ifndef _DEBUG
    Vibend::setforeground(75, 75, 75);
    printTitle();
    Vibend::teleport(10_vh, 50_vw - MainMenu::TITLE_CENTER);

    Vibend::setforeground(Vibend::Color::WHITE);
    for (const char* cur = MainMenu::TITLE; *cur != '\0'; cur++)
    {
        Sleep(50);
        Vibend::print(*cur);
    }
#endif // !_DEBUG

    return 0;
}

int main()
{
    init();

    using namespace Vibend::Literals;

    Vibend::ItemSelect optionsUI(
        Vibend::Box(),
        &MainMenu::options
    );

    Vibend::ItemSelect difficultiesUI(
        Vibend::Box(),
        &MainMenu::difficulties
    );

    do {
        optionsUI.current = 0;

        uint16_t lastwidth = 0;
        uint16_t lastheight = 0;

        while (optionsUI.refresh(false) != Vibend::ItemSelect::SELECT) {
            Vibend::checksize();

            uint16_t curwidth = Vibend::screenwidth;
            uint16_t curheight = Vibend::screenheight;

            if (lastwidth == curwidth && lastheight == curheight)
                continue;

            lastwidth = curwidth;
            lastheight = curheight;
            
            Vibend::resetstyle();
            Vibend::clear();

            printTitle();

            optionsUI.box = Vibend::Box(25_vw, 50_vh - (uint16_t)MainMenu::Options::COUNT / 2, 50_vw, (uint16_t)MainMenu::Options::COUNT);
            optionsUI.drawall();
            optionsUI.hover(optionsUI.current);
            Vibend::Panel(optionsUI.box.outer()).drawborder();
        }

        // Exit if the current options is last one (Exit)
        // Making the while condition useless
        if (optionsUI.current == MainMenu::Options::EXIT)
            break;

        Vibend::resetstyle();
        Vibend::clear();
        Vibend::checksize();

        difficultiesUI.box = Vibend::Box(25_vw, 50_vh - (uint16_t)MainMenu::difficulties.size() / 2, 50_vw, (uint16_t)MainMenu::difficulties.size());
        Vibend::Panel(difficultiesUI.box.outer()).drawborder();
        difficultiesUI.drawall();
        difficultiesUI.hover(0);
        difficultiesUI.current = 0;

        difficultiesUI.refresh(false);

        // Get the difficulty
        if (optionsUI.current != MainMenu::Options::CREDITS)
            while (difficultiesUI.refresh(false) != Vibend::ItemSelect::SELECT)
                if (difficultiesUI.pressedch() == '\x1b') break;

        if (difficultiesUI.pressedch() == '\x1b') continue;

        Vibend::resetstyle();
        Vibend::clear();
        Vibend::checksize();

        switch (optionsUI.current)
        {
        case MainMenu::Options::EXAMPLE:
            takeTest(Text::EXAMPLE_TEXTS[difficultiesUI.current]);
            break;

        case MainMenu::Options::AI: {
            Vibend::teleport(10_vh, 25_vw);
            Vibend::print(MainMenu::SUBJECT_INPUT_MSG);

            Vibend::teleport(10_vh +1, 25_vw);
            auto input = inputLine();

            if (input.empty()) break;

            Vibend::teleport(10_vh +2, 25_vw);
            Vibend::print(MainMenu::TEXT_GENERATION_MSG);
            
            auto response = sendRequest(AI::AI_PROMPT_PREFIX + input + AI::DIFFICULTY_SUFFIX[difficultiesUI.current]);
            auto text = extractText(response);

            Vibend::clear();
            takeTest(text.c_str());
            break;
        }

        case MainMenu::Options::WORDS:
            takeWords(Resources::wordslvls[difficultiesUI.current]);
            break;
        
        case MainMenu::Options::CREDITS:
            takeTest(Text::CREDITS);
            break;
        
        default:
            break;
        }

    } while (optionsUI.current != MainMenu::Options::EXIT);

    Vibend::setcursor(true);

    return 0;
}