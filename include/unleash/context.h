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
    std::map<std::string, std::string> properties;
    // Appended after `properties` to preserve the 1.3.0 aggregate-initialization order.
    std::string currentTime;
};
}  // namespace unleash
#endif  //UNLEASH_CONTEXT_H
