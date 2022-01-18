#ifndef UNLEASH_STRATEGY_H
#define UNLEASH_STRATEGY_H

#include "unleash/context.h"
#include <memory>
#include <string>

namespace unleash {
class Strategy {
public:
    Strategy(std::string name, const std::string &parameters);
    virtual ~Strategy() = default;
    virtual bool isEnabled(const Context &context);
    static std::unique_ptr<Strategy> createStrategy(const std::string &strategy, const std::string &parameters);

private:
    const std::string m_name;
    const std::string m_parameters;
};
}  // namespace unleash

#endif  //UNLEASH_STRATEGY_H
