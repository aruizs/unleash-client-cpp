#ifndef UNLEASH_GRADUALROLLOUTUSERID_H
#define UNLEASH_GRADUALROLLOUTUSERID_H
#include "strategy.h"
#include <string>

namespace unleash {
class GradualRolloutUserId : public Strategy {
public:
    explicit GradualRolloutUserId(std::string_view parameters);
    bool isEnabled(const Context &context) override;

private:
    std::string m_groupId;
    uint32_t m_percentage;
};
}  // namespace unleash
#endif  //UNLEASH_GRADUALROLLOUTUSERID_H
