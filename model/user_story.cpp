#include "user_story.hpp"
#include "id_generator.hpp"

UserStory::UserStory(const std::string& /*id*/, const std::string& title) : storyId_(idGen_.generateId(title)) {}
UserStory::~UserStory() {}

std::string UserStory::getId() const {
    return storyId_;
}