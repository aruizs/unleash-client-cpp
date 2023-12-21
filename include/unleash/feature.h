#ifndef UNLEASH_FEATURE_H
#define UNLEASH_FEATURE_H
#include "unleash/strategies/strategy.h"
#include <memory>
#include <string>
#include <vector>

namespace unleash {
class Context;
class Feature {
public:
    Feature(std::string name, std::vector<std::unique_ptr<Strategy>> strategies, bool enable);
    bool isEnabled(const Context &context) const;

private:
    std::string m_name;
    bool m_enabled;
    std::vector<std::unique_ptr<Strategy>> m_strategies;
};
}  // namespace unleash
#endif  //UNLEASH_FEATURE_H
