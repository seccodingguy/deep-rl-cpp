# Deep RL Agent — C++ Implementation

A self-learning Deep Q-Network (DQN) agent that:
1. Analyses a codebase's structure and conventions
2. Learns which contextual information to include in prompts
3. Dynamically generates LLM prompts from user stories
4. Uses the LLM to produce idiomatic, architecture-conforming code

---

## Project Structure

```
deep_rl_cpp/
├── CMakeLists.txt
├── README.md
└── src/
    ├── neural_network.hpp    # Pure C++ neural network (He init, ReLU, SGD)
    ├── replay_buffer.hpp     # Fixed-capacity experience replay buffer
    ├── dqn_agent.hpp         # DQN agent (online + target networks, ε-greedy)
    ├── code_analyzer.hpp     # Codebase walker — extracts structural features
    ├── state_encoder.hpp     # CodebaseGraph → 20-dim state vector
    ├── prompt_actions.hpp    # Discrete action space enum + PromptDecisions
    ├── prompt_builder.hpp    # Assembles LLM prompt from agent decisions
    ├── reward_evaluator.hpp  # Scores generated code → scalar reward
    ├── llm_client.hpp        # Anthropic + Ollama client interfaces
    ├── llm_client.cpp        # libcurl implementations (Anthropic & Ollama)
    └── main.cpp              # Training loop + inference pipeline
```

---

## Dependencies

| Dependency | Required | Purpose | Install |
| :----------- | :--------- | :-------- | :-------- |
| C++17 compiler | Yes | Structured bindings, `fs::` | GCC 9+, Clang 9+, MSVC 2019+ |
| CMake 3.16+ | Yes | Build system | `apt-get install cmake` |
| libcurl | Optional | HTTP calls to Anthropic/Ollama API | `apt-get install libcurl4-openssl-dev` |
| nlohmann/json | Optional | JSON serialisation | `apt-get install nlohmann-json3-dev` |
| Ollama | Optional | Local LLM inference server | [ollama.com](https://ollama.com) |

The agent runs **without** libcurl/json in demo mode (toy balancing environment).

---

## Build

### Linux / macOS

```bash
# 1. Install optional dependencies (for LLM support)
sudo apt-get install libcurl4-openssl-dev nlohmann-json3-dev   # Ubuntu/Debian
brew install curl nlohmann-json                                # macOS

# 2. Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 3. Run demo (no LLM needed)
./deep_rl_agent

# 4. Run with real codebase + Anthropic Claude
./deep_rl_agent /path/to/your/project sk-ant-your-api-key "your user story"

# 5. Run with real codebase + Ollama (no API key needed)
#    Start Ollama first: ollama serve  (or point to a remote host)
./deep_rl_agent /path/to/your/project http://<ollama-host>:11434 <model> "your user story"

# Example: remote Ollama host, llama3.1:8b model
./deep_rl_agent /path/to/your/project http://192.168.50.153:11434 llama3.1:8b \
    "Add a new feature to include user story analyzer"
```

### Windows (MSVC + vcpkg)

```powershell
vcpkg install curl nlohmann-json
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
.\Release\deep_rl_agent.exe
```

---

## How It Works

### Q-Value Flow

```
State [codebase features + story keywords]  (20-dim)
       │
       ▼
  Online Network  [20 → 128 → 128 → 12]
       │
       ▼
  Q-values: [Q(s,a₀), Q(s,a₁), …, Q(s,a₁₁)]
       │
       ├─ Action selection: argmax → chosen PromptAction
       │
       └─ Training:
             TD Target  = r + γ · max Q_target(s', a')
             TD Error   = TD Target − Q_online(s, a)
             Loss       = TD Error²
             Backprop   → update online network weights
```

### Architecture

```
┌──────────────────────────────────────────────────────┐
│ OFFLINE (once per codebase)                          │
│  Codebase → CodeAnalyzer → CodebaseGraph             │
│                 ↓                                    │
│           StateEncoder → 20-dim state vector         │
└──────────────────────────────────────────────────────┘
                       ↓
┌──────────────────────────────────────────────────────┐
│ TRAINING (many episodes)                             │
│  Story + State → DQNAgent.select_action()            │
│       ↓                                              │
│  PromptDecisions → PromptBuilder → Prompt string     │
│       ↓                                              │
│  LLM → Generated Code → RewardEvaluator → Reward    │
│       ↓                                              │
│  DQNAgent.train_step() → better Q-values             │
└──────────────────────────────────────────────────────┘
                       ↓
┌──────────────────────────────────────────────────────┐
│ INFERENCE                                            │
│  New Story → Encode → Agent picks best actions       │
│                  ↓                                   │
│           LLM → Production Code                      │
└──────────────────────────────────────────────────────┘
```

---

## Key Design Decisions

| Decision | Rationale |
|:---------|:----------|
| Header-only core (except LLM client) | Zero-dependency compilation for demo mode |
| He initialisation | Prevents vanishing gradients with ReLU activations |
| Separate online / target networks | Prevents reward instability (moving target problem) |
| ε-greedy with decay | Explores early, exploits learned policy later |
| Multi-signal reward | Compilation + style + architecture = holistic quality |

---

## Extending

- **New action**: Add entry to `PromptAction` enum, handle in `PromptBuilder::build()`
- **Richer state**: Extend `StateEncoder::encode()` — update `STATE_SIZE` in `main.cpp`
- **Different LLM**: Replace `LLMClient::generate()` body with your provider's API
- **Deeper network**: Change topology in `DQNAgent` constructor: `{20, 256, 256, 12}`
