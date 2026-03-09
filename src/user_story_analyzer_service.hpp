#ifndef USER_STORY_ANALYZER_SERVICE_HPP
#define USER_STORY_ANALYZER_SERVICE_HPP

#include "service/service.hpp"
#include "model/user_story.hpp"

class UserStoryAnalyzerService : public Service {
public:
    UserStoryAnalyzerService();
    ~UserStoryAnalyzerService();

    std::map<std::string, double> analyze(const std::vector<UserStory>& userStories, const bool isCode = false) const;
};

#endif // USER_STORY_ANALYZER_SERVICE_HPP
