#ifndef UNLEASH_CONTEXT_H
#define UNLEASH_CONTEXT_H
#include <map>
#include <string>

namespace unleash {
/**
 * @brief Evaluation context passed to isEnabled() and variant().
 *
 * All fields are optional; populate the ones relevant to the strategies and constraints
 * configured for your flags.
 */
struct Context {
    std::string userId;          ///< Unique user identifier.
    std::string sessionId;       ///< Session identifier.
    std::string remoteAddress;   ///< Client IP address (used by IP/CIDR strategies).
    std::string environment;     ///< Environment name.
    std::string appName;         ///< Application name.
    std::map<std::string, std::string> properties;  ///< Custom fields referenced by constraints.
    /// Reference time (ISO-8601) for date constraints. Appended last to preserve the 1.3.0
    /// aggregate-initialization order.
    std::string currentTime;
};
}  // namespace unleash
#endif  //UNLEASH_CONTEXT_H
