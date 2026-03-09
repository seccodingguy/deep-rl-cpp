#pragma once
#include <string>
#include <stdexcept>

// Note: Requires libcurl and nlohmann/json
// Install: apt-get install libcurl4-openssl-dev
//          vcpkg install nlohmann-json
//
// Link flags: -lcurl

// Forward declarations to avoid header dependency in this file
// Include <curl/curl.h> and <nlohmann/json.hpp> in your .cpp

class LLMClient {
    std::string api_key;
    std::string endpoint;
    std::string model_name;

public:
    LLMClient(const std::string& key,
              const std::string& url   = "https://api.anthropic.com/v1/messages",
              const std::string& model = "claude-sonnet-4-20250514")
        : api_key(key), endpoint(url), model_name(model)
    {
        if (key.empty())
            throw std::invalid_argument("An Anthropic API key is required.");
    }

    // Send prompt to Claude, return generated text.
    // Implementation lives in llm_client.cpp to avoid pulling curl into every TU.
    std::string generate(const std::string& prompt);

    const std::string& get_model()    const { return model_name; }
    const std::string& get_endpoint() const { return endpoint;   }
};

// Ollama client — no API key required; runs against a local Ollama server.
// Default endpoint: http://localhost:11434/api/chat
class OllamaClient {
    std::string endpoint;
    std::string model_name;

public:
    OllamaClient(const std::string& model = "llama3",
                 const std::string& url   = "http://localhost:11434/api/chat")
        : endpoint(url), model_name(model) {}

    // Send prompt to Ollama, return generated text.
    std::string generate(const std::string& prompt);

    const std::string& get_model()    const { return model_name; }
    const std::string& get_endpoint() const { return endpoint;   }
};
