#include <vibend.h>
#include "typing.h"
#include <curl/curl.h>
#include <string>
#include <iostream>
#include <fstream>
#include "resource.h"
#pragma warning(push)
#pragma warning(disable : 26495)
#pragma warning(disable : 28020)
#include <nlohmann/json.hpp>
#pragma warning(pop)

std::vector<std::string> wordslvls[3];

namespace Resources
{
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

using json = nlohmann::json;

// Store response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* output) {
    if (!contents || !output) return 0;  // Prevent access violations
    std::string* response = static_cast<std::string*>(output);
    response->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

// Save CURL verbose log into a file
#pragma warning(push)
#pragma warning(disable : 26812)
static int LogCallback(CURL* handle, curl_infotype type, char* data, size_t size, void* userptr)
#pragma warning(pop)
{
    std::ofstream* logFile = static_cast<std::ofstream*>(userptr);
    if (logFile) {
        (*logFile) << std::string(data, size);
    }
    return 0;
}

// Send the request
std::string sendRequest(const std::string& prompt)
{
    CURL* curl = curl_easy_init();
    if (!curl) {
        return "Error: Failed to initialize CURL";
    }

    std::string response;
    struct curl_slist* headers = nullptr;

    // Set headers
    headers = curl_slist_append(headers, "Authorization: Bearer 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");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!headers) {
        curl_easy_cleanup(curl);
        return "Error: Failed to set headers";
    }

    // Construct JSON request body
    json requestData = {
        {"prompt", prompt},
        {"model", "meta-llama/llama-4-maverick:free"}
    };
    std::string requestBody = requestData.dump();

    // Open log file for writing
    std::ofstream logFile("curl_log.txt");

    // Set CURL options
#pragma warning(push)
#pragma warning(disable : 26812)
    curl_easy_setopt(curl, CURLOPT_URL, "https://openrouter.ai/api/v1/chat/completions");
#pragma warning(pop)
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());

    // Set write callback function
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // Redirect verbose output to a file instead of printing
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, LogCallback);
    curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &logFile);

    // Perform request and handle errors
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        response = "Error: " + std::string(curl_easy_strerror(res));
    }

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    logFile.close(); // Close log file

    return response;
}

std::string extractText(const std::string& jsonResponse) {
    try {
        json parsedData = json::parse(jsonResponse); // Parse the JSON string
        return parsedData["choices"][0]["text"]; // Extract the main response text
    }
    catch (const std::exception& e) {
        return "Error parsing JSON: " + std::string(e.what());
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

#ifdef _DEBUG
constexpr const char* TITLE = "- Null Type Debug -";
constexpr uint16_t TITLE_CENTER = (20 - 1) / 2;
#else
constexpr const char* TITLE = "- Null Type -";
constexpr uint16_t TITLE_CENTER = (14 - 1) / 2;
#endif // _DEBUG
void printTitle()
{
    using namespace Vibend::Literals;
    Vibend::teleport(10_vh, 50_vw - TITLE_CENTER);
    Vibend::print(TITLE);
}

int main()
{
    // TODO Debug errors
    Resources::loadTextResource(IDR_TEXT1, wordslvls[0]);
    Resources::loadTextResource(IDR_TEXT2, wordslvls[1]);
    Resources::loadTextResource(IDR_TEXT3, wordslvls[2]);

    // Initialize Vibend
    Vibend::init();
    Vibend::setcursor(false);

    using namespace Vibend::Literals;

#ifndef _DEBUG
    Vibend::setforeground(64, 64, 64);
    printTitle();
    Vibend::teleport(10_vh, 50_vw - TITLE_CENTER);

    Vibend::setforeground(Vibend::Color::WHITE);
    for (const char* cur = TITLE; *cur != '\0'; cur++)
    {
        // Skip spaces
        if (*cur != ' ')
            Vibend::getch();

        Vibend::print(*cur);
    }
#endif // !_DEBUG

    #pragma region Items
    constexpr uint8_t INDEX_EXAMPLE_TEXT = 0;
    constexpr uint8_t INDEX_AI_TEXT      = 1;
    constexpr uint8_t INDEX_WORDS        = 2;
    constexpr uint8_t INDEX_CREDITS_TEXT = 3;
    constexpr uint8_t INDEX_EXIT         = 4;

    std::vector<const char*> items(5);
    items[INDEX_EXAMPLE_TEXT]   = "Example text";
    items[INDEX_AI_TEXT]        = "Type AI text";
    items[INDEX_WORDS]          = "Words";
    items[INDEX_CREDITS_TEXT]   = "Credits";
    items[INDEX_EXIT]           = "Exit";
    #pragma endregion

    std::vector<const char*> difficulties = { "Piece of cake", "Tricky", "BEAST" };
    constexpr const char* DIFFICULTY_SUFFIX[3] = {
        ". About 100 words with only lowercase letters and no symbols just simple words",
        ". About 300 words",
        ". About 600 words and some hard, long and advanced words and also symbols"
    };

    constexpr const char* EXAMPLE_TEXTS[3] = {
        // Difficulty 0
        "the sun was warm and bright in the clear blue sky "
        "as the gentle wind moved across the green field of flowers "
        "the birds sang sweet songs in the morning air "
        "while the trees swayed softly in the breeze "
        "the river flowed calmly past the tall hills "
        "and the grass danced under the golden light of the afternoon "
        "the world felt peaceful and still "
        "as nature embraced the beauty of the day "
        "the animals played in the forest "
        "and the butterflies drifted above the meadows "
        "it was a perfect moment of harmony and joy in the soft embrace of the earth",

        // Difficulty 1
        "The golden Sun cast shimmering reflections upon the restless waves, "
        "dancing like flames on the water's surface. A cool breeze whispered "
        "through the towering trees, stirring the emerald leaves into a soft "
        "rustling symphony. Birds soared high, tracing elegant arcs against "
        "the endless sky, their songs mingling with the distant laughter of "
        "children playing by the shore. Time seemed to slow as the rhythmic "
        "ebb and flow of the tide carried thoughts far beyond the horizon.\n"
        "In the heart of the forest, an ancient oak stood resolute, its gnarle d"
        "branches reaching for the heavens like a silent guardian of secrets untold. "
        "Beneath its watchful gaze, a small stream meandered through moss-covered "
        "stones, its melody soothing the weary traveler who paused to drink in "
        "the tranquility. A lone deer stepped cautiously from the shadows, its "
        "gaze locked upon the fleeting light filtering through the canopy, painting "
        "the earth in soft hues of amber and gold.\n"
        "As twilight descended, the sky ignited in a spectacle of vibrant colors�"
        "crimson, violet, and fiery orange merging in breathtaking harmony. Stars "
        "emerged, timid at first, before scattering across the heavens in a brilliant "
        "display of cosmic wonder. The world held its breath, caught between the "
        "fading warmth of day and the quiet embrace of the night. And in that moment, "
        "as the last glimmer of sunlight surrendered to the abyss, all was still, "
        "yet infinitely alive.",

        // Difficulty 2
        "The resplendent Sun, a celestial beacon of incandescent brilliance, "
        "cast its effulgent radiance upon the undulating aqueous expanse, "
        "where capricious waves ebbed and surged in a rhythmic, perpetual dance. "
        "Zephyrs meandered through the sylvan canopy, coaxing verdant foliage "
        "into an elaborate ballet of shimmering oscillations, their susurrus "
        "interwoven with the dulcet warbling of avian denizens soaring in the ether. "
        "Below, juveniles cavorted along the sun-drenched shore, their mirthful "
        "laughter an ephemeral symphony harmonizing with the elemental cadence "
        "of the tide. Time, an inexorable yet abstract construct, seemed momentarily "
        "suspended within this idyllic tableau.\n"
        "Deep within the heart of the emerald labyrinth, an ancient arboreal sentinel "
        "stood in august repose, its gnarled extremities extending toward the firmament "
        "like an erudite scholar imparting wisdom to the cosmos. Beneath its expansive "
        "umbrage, a serpentine brook traversed a mosaic of moss-cloaked stones, its "
        "gurgling cascade imbued with a tranquil melody that allayed the weary sojourner�s "
        "burdens. A solitary cervine figure materialized from the umbrage, its luminous "
        "gaze fixed upon the ephemeral interplay of photons filtering through the "
        "verdant ceiling, suffusing the forest floor with an amber-hued resplendence.\n"
        "As the diurnal spectacle waned, twilight unfurled its sumptuous chromatic tapestry "
        "across the celestial dome vermilion, amethystine, and aureate hues amalgamating "
        "into an opulent chiaroscuro of incandescent splendor. The firmament, initially "
        "tenebrous, awakened with an astral effulgence as constellations emerged from the "
        "abyss, festooning the nocturnal heavens with celestial cartography. The world, "
        "enveloped in an ephemeral serenity, oscillated between the lingering warmth of "
        "day and the somnolent embrace of night. And in that transcendental moment�"
        "as the last vestiges of solar brilliance capitulated to the cosmic void�"
        "existence was paradoxically immutable yet vibrantly alive."
    };

    Vibend::ItemSelect options(
        Vibend::Box(),
        &items
    );

    Vibend::ItemSelect difficulty(
        Vibend::Box(),
        &difficulties
    );

    constexpr const char* AI_PROMPT_PREFIX = 
        "Generate a simple text based on the following subject "
        "using only characters available on a standard keyboard, "
        "avoid emojis, or non-standard characters, only give me the text. Subject: ";

    do {
        options.current = 0;

        uint16_t lastwidth = 0;
        uint16_t lastheight = 0;

        while (options.refresh(false) != Vibend::ItemSelect::SELECT) {
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

            options.box = Vibend::Box(25_vw, 50_vh - (uint16_t)items.size() / 2, 50_vw, (uint16_t)items.size());
            options.drawall();
            options.hover(options.current);
            Vibend::Panel(options.box.outer()).drawborder();
        }

        // Exit if the current options is last one (Exit)
        // Making the while condition useless
        if (options.current == INDEX_EXIT)
            break;

        Vibend::resetstyle();
        Vibend::clear();
        Vibend::checksize();

        difficulty.box = Vibend::Box(25_vw, 50_vh - (uint16_t)difficulties.size() / 2, 50_vw, (uint16_t)difficulties.size());
        Vibend::Panel(difficulty.box.outer()).drawborder();
        difficulty.drawall();
        difficulty.hover(0);
        difficulty.current = 0;

        difficulty.refresh(false);

        // Get the difficulty
        if (options.current != INDEX_CREDITS_TEXT)
            while (difficulty.refresh(false) != Vibend::ItemSelect::SELECT)
                if (difficulty.pressedch() == '\x1b') break;

        if (difficulty.pressedch() == '\x1b') continue;

        Vibend::resetstyle();
        Vibend::clear();
        Vibend::checksize();

        switch (options.current)
        {
        case INDEX_EXAMPLE_TEXT:
            takeTest(EXAMPLE_TEXTS[difficulty.current]);
            break;

        case INDEX_AI_TEXT: {
            Vibend::teleport(10_vh, 25_vw);
            Vibend::print("Enter your subject:");

            Vibend::teleport(10_vh +1, 25_vw);
            auto input = inputLine();

            if (input.empty()) break;

            Vibend::teleport(10_vh +2, 25_vw);
            Vibend::print("Your text is being generated please wait...");
            
            auto response = sendRequest(AI_PROMPT_PREFIX + input + DIFFICULTY_SUFFIX[difficulty.current]);
            auto text = extractText(response);

            Vibend::clear();
            takeTest(text.c_str());
            break;
        }

        case INDEX_WORDS:
            takeWords(wordslvls[difficulty.current]);
            break;
        
        case INDEX_CREDITS_TEXT:
            takeTest(
                "NullType, an application about typing and console. "
                "Inspired by typing.com and curses lib. Using vibend! "
                "Example texts are generated by AI! But are used in offline mode.\n"
                "Developed by Sina Kasesaz"
            );
            break;
        
        default:
            break;
        }

    } while (options.current != INDEX_EXIT);

    Vibend::setcursor(true);

    return 0;
}