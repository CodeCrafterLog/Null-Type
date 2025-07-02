#pragma once
#include <chrono>
#include <vector>
#include <string>

#pragma region Input
constexpr uint8_t NO_INPUT  = 0;
constexpr uint8_t CORRECT   = 1;
constexpr uint8_t WRONG     = 2;
constexpr uint8_t EXIT      = 3;
#pragma endregion

extern unsigned char enter;

void printSymbol(char current);
uint8_t getAndEcho(char correct);
uint8_t waitForInput(char totype, std::chrono::steady_clock::time_point& previostime, bool forceType);
std::chrono::duration<double> takeTest(const char* text);
std::chrono::duration<double> takeWords(std::vector<std::string>& words);