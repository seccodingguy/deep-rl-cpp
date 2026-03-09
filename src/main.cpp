#include "dqn_agent.hpp"
#include "code_analyzer.hpp"
#include "state_encoder.hpp"
#include "prompt_actions.hpp"
#include "prompt_builder.hpp"
#include "reward_evaluator.hpp"
#include "llm_client.hpp"
#include "user_story_analyzer_service.hpp"
#include "model/user_story.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <cmath>

// ── Toy environment: balance a value near 0 ───────────────────────────────────
// State:  [position, velocity]
// Actions: 0 = push left (-1), 1 = push right (+1)
struct ToyEnv {
    double pos = 0.0, vel = 0.0;
    int steps = 0;

    Vector reset() {
        pos = ((double)rand() / RAND_MAX) * 0.1 - 0.05;
        vel = 0.0;
        steps = 0;
        return {pos, vel};
    }

    struct StepResult { Vector state; double reward; bool done; };

    StepResult step(int action) {
        double force = (action == 1) ? 1.0 : -1.0;
        vel = 0.9 * vel + 0.1 * force;
        pos += vel;
        steps++;

        double reward = -std::abs(pos);
        bool done = std::abs(pos) > 1.5 || steps >= 200;
        return {{pos, vel}, reward, done};
    }
};

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    const int STATE_SIZE  = 20;
    const int ACTION_SIZE = 2;    // toy env: push left / push right
    const int EPISODES    = 500;

    // ── 1. Code-aware pipeline (optional – requires real codebase path) ───────
    // Usage:
    //   Anthropic: ./deep_rl_agent <codebase> <api_key> [story]
    //   Ollama:    ./deep_rl_agent <codebase> <ollama_base_url> <model> [story]
    //              (Ollama detected when arg[2] starts with "http")
    if (argc > 1) {
        std::string codebase_path = argv[1];

        bool        use_ollama   = (argc > 2 &&
                                    std::string(argv[2]).rfind("http", 0) == 0);
        std::string ollama_url, ollama_model, api_key, user_story;

        if (use_ollama) {
            std::string base = argv[2];
            if (!base.empty() && base.back() == '/') base.pop_back();
            ollama_url   = base + "/api/chat";
            ollama_model = (argc > 3) ? argv[3] : "llama3";
            user_story   = (argc > 4) ? argv[4]
                                      : "As a user I want to upload a profile picture";
        } else {
            api_key    = (argc > 2) ? argv[2] : "";
            user_story = (argc > 3) ? argv[3]
                                    : "As a user I want to upload a profile picture";
        }

        CodeAnalyzer  analyzer;
        StateEncoder  encoder;
        PromptBuilder builder;
        [[maybe_unused]] RewardEvaluator evaluator;

        auto graph = analyzer.analyze(codebase_path);
        std::cout << "Analysed " << graph.files.size() << " files. "
                  << "Pattern: " << graph.dominant_pattern << "\n";

        const int CODE_STATE  = 20;
        const int CODE_ACTION = (int)PromptAction::ACTION_COUNT;

        DQNAgent code_agent(CODE_STATE, CODE_ACTION,
            /*lr=*/1e-3, /*gamma=*/0.95,
            /*eps_start=*/1.0, /*eps_min=*/0.05, /*eps_decay=*/0.99,
            /*batch=*/16, /*buf=*/5000, /*tgt_upd=*/50);

        std::vector<std::string> training_stories = {
            "As a user I want to create a new account with email and password",
            "As an admin I want to list all users with pagination",
            "As a user I want to update my profile information",
            "As a system I want to send email notifications on signup"
        };

        for (int ep = 0; ep < 200; ++ep) {
            auto& story = training_stories[ep % training_stories.size()];
            auto  state = encoder.encode(graph, story);
            int   action = code_agent.select_action(state);

            PromptDecisions decisions;
            decisions.selected_actions.push_back(static_cast<PromptAction>(action));
            decisions.naming_convention    = "camelCase";
            decisions.error_handling_style = "exceptions";

            // Mock reward for demo (replace with real LLM call + evaluator)
            double reward = (action < 6) ? 1.0 : -0.5;

            code_agent.remember(state, action, reward, state, true);
            double loss = code_agent.train_step();

            if (ep % 50 == 0)
                std::cout << "Code Agent Ep " << ep
                          << " | Reward: " << reward
                          << " | Loss: "   << loss
                          << " | Eps: "    << code_agent.epsilon << "\n";
        }

        // Inference
        code_agent.epsilon = 0.0;
        auto inf_state   = encoder.encode(graph, user_story);
        int  best_action = code_agent.select_action(inf_state);

        PromptDecisions final_decisions;
        final_decisions.selected_actions.push_back(
            static_cast<PromptAction>(best_action));
        final_decisions.naming_convention    = "camelCase";
        final_decisions.error_handling_style = "exceptions";

        std::string prompt = builder.build(user_story, final_decisions, graph);
        std::cout << "\n=== Generated Prompt ===\n" << prompt << "\n";

        if (use_ollama) {
            std::cout << "Calling Ollama (" << ollama_model
                      << " @ " << ollama_url << ")...\n";
            OllamaClient llm(ollama_model, ollama_url);
            auto code = llm.generate(prompt);
            std::cout << "\n=== Generated Code ===\n" << code << "\n";
        } else if (!api_key.empty()) {
            LLMClient llm(api_key);
            auto code = llm.generate(prompt);
            std::cout << "\n=== Generated Code ===\n" << code << "\n";
        }

        // ── User Story Analyzer demo ──────────────────────────────────────────
        std::cout << "\n=== User Story Analysis ===\n";
        UserStoryAnalyzerService svc;
        std::vector<UserStory> stories = {
            UserStory("", user_story),
            UserStory("", "As an admin I want to see system metrics"),
            UserStory("", "As a developer I want CI to run tests automatically"),
        };
        auto story_scores = svc.analyze(stories, /*isCode=*/false);
        for (auto& [id, score] : story_scores)
            std::cout << "  story[" << id.substr(0, 8) << "...] -> score: " << score << "\n";

        std::cout << "\n=== Code Analysis ===\n";
        std::cout << "  Language : " << graph.dominant_language << "\n";
        std::cout << "  Pattern  : " << graph.dominant_pattern  << "\n";
        std::cout << "  Files    : " << graph.files.size()      << "\n";

        return 0;
    }

    // ── 2. Standalone DQN demo (toy balancing environment) ────────────────────
    std::cout << "Running standalone DQN demo (toy balancing env)...\n\n";

    DQNAgent agent(STATE_SIZE, ACTION_SIZE,
        /*lr=*/1e-3, /*gamma=*/0.99,
        /*eps_start=*/1.0, /*eps_min=*/0.01, /*eps_decay=*/0.995,
        /*batch=*/32, /*buf=*/5000, /*tgt_upd=*/50);

    ToyEnv env;

    for (int ep = 0; ep < EPISODES; ++ep) {
        Vector state = env.reset();
        double total_reward = 0.0;
        double avg_loss     = 0.0;
        int    num_steps    = 0;
        bool   done         = false;

        while (!done) {
            int action = agent.select_action(state);
            auto [next_state, reward, is_done] = env.step(action);
            agent.remember(state, action, reward, next_state, is_done);
            double loss = agent.train_step();

            total_reward += reward;
            avg_loss     += loss;
            state         = next_state;
            done          = is_done;
            ++num_steps;
        }

        if (ep % 50 == 0) {
            auto q_vals = agent.get_q_values(state);
            std::cout << "Episode " << ep
                      << " | Steps: "    << num_steps
                      << " | Reward: "   << total_reward
                      << " | AvgLoss: "  << avg_loss / num_steps
                      << " | Eps: "      << agent.epsilon
                      << " | Q[L]: "     << q_vals[0]
                      << " | Q[R]: "     << q_vals[1]
                      << "\n";
        }
    }

    return 0;
}
