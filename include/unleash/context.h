#ifndef UNLEASH_CONTEXT_H
#define UNLEASH_CONTEXT_H
#include <map>
#include <string>

namespace unleash {
struct Context {
    std::string userId;
    std::string sessionId;
    std::string remoteAddress;
    std::string environment;
    std::string appName;
    std::string currentTime;
    std::map<std::string, std::string> properties;
};
}  // namespace unleash
#endif  //UNLEASH_CONTEXT_H
