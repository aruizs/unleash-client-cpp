#ifndef UNLEASH_GRADUALROLLOUTSESSIONID_H
#define UNLEASH_GRADUALROLLOUTSESSIONID_H
#include "strategy.h"
#include <string>

namespace unleash {
class GradualRolloutSessionId : public Strategy {
public:
    GradualRolloutSessionId(const std::string &parameters);
    bool isEnabled(const Context &context);

private:
    std::string m_groupId;
    uint32_t m_percentage;
};
}  // namespace unleash
#endif  //UNLEASH_GRADUALROLLOUTSESSIONID_H
