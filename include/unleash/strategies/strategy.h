#ifndef UNLEASH_STRATEGY_H
#define UNLEASH_STRATEGY_H

#include "unleash/context.h"
#include <memory>
#include <string>

namespace unleash {
class Strategy {
public:
    Strategy(std::string name, std::string_view parameters);
    virtual ~Strategy() = default;
    virtual bool isEnabled(const Context &context) = 0;
    static std::unique_ptr<Strategy> createStrategy(std::string_view strategy, std::string_view parameters);

private:
    const std::string m_name;
    const std::string m_parameters;
};
}  // namespace unleash

#endif  //UNLEASH_STRATEGY_H
