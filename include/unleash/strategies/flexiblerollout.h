#ifndef UNLEASH_FLEXIBLEROLLOUT_H
#define UNLEASH_FLEXIBLEROLLOUT_H
#include "strategy.h"
#include <string>

namespace unleash {
class FlexibleRollout : public Strategy {
public:
    explicit FlexibleRollout(std::string_view parameters);
    bool isEnabled(const Context &context) override;

private:
    std::string m_stickiness;
    std::string m_groupId;
    uint32_t m_rollout;
};
}  // namespace unleash
#endif  //UNLEASH_FLEXIBLEROLLOUT_H
