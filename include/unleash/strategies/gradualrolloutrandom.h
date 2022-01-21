#ifndef UNLEASH_GRADUALROLLOUTRANDOM_H
#define UNLEASH_GRADUALROLLOUTRANDOM_H
#include "strategy.h"
#include <string>

namespace unleash {
class GradualRolloutRandom : public Strategy {
public:
    explicit GradualRolloutRandom(std::string_view parameters);
    bool isEnabled(const Context &context) override;

private:
    uint32_t m_percentage;
};
}  // namespace unleash
#endif  //UNLEASH_GRADUALROLLOUTRANDOM_H
