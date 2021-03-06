#ifndef UNLEASH_GRADUALROLLOUTSESSIONID_H
#define UNLEASH_GRADUALROLLOUTSESSIONID_H
#include "strategy.h"
#include <string>

namespace unleash {
class GradualRolloutSessionId : public Strategy {
public:
    explicit GradualRolloutSessionId(std::string_view parameters, std::string_view constraints = {});
    bool isEnabled(const Context &context) override;

private:
    std::string m_groupId;
    uint32_t m_percentage;
};
}  // namespace unleash
#endif  //UNLEASH_GRADUALROLLOUTSESSIONID_H
