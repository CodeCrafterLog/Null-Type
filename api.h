#pragma once

#include <string>
#include <curl/curl.h>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* output);
static int LogCallback(CURL* handle, curl_infotype type, char* data, size_t size, void* userptr);
std::string sendRequest(const std::string& prompt);
std::string extractText(const std::string& jsonResponse);