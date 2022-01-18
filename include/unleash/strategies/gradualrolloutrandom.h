#ifndef UNLEASH_GRADUALROLLOUTRANDOM_H
#define UNLEASH_GRADUALROLLOUTRANDOM_H
#include "strategy.h"
#include <string>

namespace unleash {
class GradualRolloutRandom : public Strategy {
public:
    GradualRolloutRandom(const std::string &parameters);
    bool isEnabled(const Context &context);

private:
    uint32_t m_percentage;
};
}  // namespace unleash
#endif  //UNLEASH_GRADUALROLLOUTRANDOM_H
