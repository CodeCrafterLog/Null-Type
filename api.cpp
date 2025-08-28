#include "api.h"

#pragma warning(push)
#pragma warning(disable : 26495)
#pragma warning(disable : 28020)
#include <nlohmann/json.hpp>
#pragma warning(pop)
#include <fstream>

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
    // This API key is only used for this app.
    // It has no personal usage
    headers = curl_slist_append(headers, "Authorization: Bearer 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");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!headers) {
        curl_easy_cleanup(curl);
        return "Error: Failed to set headers";
    }

    // Construct JSON request body
    nlohmann::json requestData = {
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
        nlohmann::json parsedData = nlohmann::json::parse(jsonResponse); // Parse the JSON string
        return parsedData["choices"][0]["text"]; // Extract the main response text
    }
    catch (const std::exception& e) {
        return "Error parsing JSON: " + std::string(e.what());
    }
}
