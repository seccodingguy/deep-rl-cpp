#include "user_story_analyzer_service.hpp"
#include "llm_client.hpp"

UserStoryAnalyzerService::UserStoryAnalyzerService() {}
UserStoryAnalyzerService::~UserStoryAnalyzerService() {}

std::map<std::string, double> UserStoryAnalyzerService::analyze(const std::vector<UserStory>& userStories, const bool isCode) const {
    // For now, we'll just return some dummy data. In the future, this function can be implemented to actually analyze the code/user stories
    std::map<std::string, double> results;

    if (isCode) {
        // Analyze code here
        for (const UserStory& story : userStories) {
            results[story.getId()] = 0.5; // Replace with actual analysis logic
        }
    } else {
        // Analyze user stories here
        for (const UserStory& story : userStories) {
            results[story.getId()] = 0.8; // Replace with actual analysis logic
        }
    }

    return results;
}