#ifndef UNLEASH_APPLICATIONHOSTNAME_H
#define UNLEASH_APPLICATIONHOSTNAME_H
#include "strategy.h"
#include <string>
#include <vector>

namespace unleash {
class ApplicationHostname : public Strategy {
public:
    ApplicationHostname(const std::string &parameters);
    bool isEnabled(const Context &context);

private:
    std::vector<std::string> m_applicationHostnames;
};
}  // namespace unleash
#endif  //UNLEASH_APPLICATIONHOSTNAME_H
