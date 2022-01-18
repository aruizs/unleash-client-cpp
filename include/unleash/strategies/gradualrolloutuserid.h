#ifndef UNLEASH_GRADUALROLLOUTUSERID_H
#define UNLEASH_GRADUALROLLOUTUSERID_H
#include "strategy.h"
#include <string>

namespace unleash {
class GradualRolloutUserId : public Strategy {
public:
    GradualRolloutUserId(const std::string &parameters);
    bool isEnabled(const Context &context);

private:
    std::string m_groupId;
    uint32_t m_percentage;
};
}  // namespace unleash
#endif  //UNLEASH_GRADUALROLLOUTUSERID_H
