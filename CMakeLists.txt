cmake_minimum_required(VERSION 3.16)
project(DeepRLAgent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler flags ────────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

# ── Find optional dependencies ────────────────────────────────────────────────
find_package(CURL QUIET)
find_package(nlohmann_json QUIET)

# ── Source files ──────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/llm_client.cpp
    src/user_story_analyzer_service.cpp
    controller/controller.cpp
    model/user_story.cpp
)

set(HEADERS
    src/neural_network.hpp
    src/replay_buffer.hpp
    src/dqn_agent.hpp
    src/code_analyzer.hpp
    src/state_encoder.hpp
    src/prompt_actions.hpp
    src/prompt_builder.hpp
    src/reward_evaluator.hpp
    src/llm_client.hpp
)

# ── Main executable ───────────────────────────────────────────────────────────
add_executable(deep_rl_agent ${SOURCES} ${HEADERS})
target_include_directories(deep_rl_agent PRIVATE src ${CMAKE_SOURCE_DIR})

# Link CURL if found (required for LLM API calls)
if(CURL_FOUND)
    target_link_libraries(deep_rl_agent PRIVATE CURL::libcurl)
    target_compile_definitions(deep_rl_agent PRIVATE HAS_CURL=1)
    message(STATUS "CURL found — LLM client enabled")
else()
    message(WARNING "CURL not found — LLM client will not compile. "
                    "Install libcurl4-openssl-dev to enable API calls.")
endif()

# Link nlohmann/json if found
if(nlohmann_json_FOUND)
    target_link_libraries(deep_rl_agent PRIVATE nlohmann_json::nlohmann_json)
    target_compile_definitions(deep_rl_agent PRIVATE HAS_JSON=1)
    message(STATUS "nlohmann/json found")
else()
    message(WARNING "nlohmann/json not found — LLM JSON parsing disabled. "
                    "Install with: vcpkg install nlohmann-json  OR  "
                    "apt-get install nlohmann-json3-dev")
endif()

# ── Install target ────────────────────────────────────────────────────────────
install(TARGETS deep_rl_agent DESTINATION bin)
install(FILES ${HEADERS} DESTINATION include/deep_rl_agent)
