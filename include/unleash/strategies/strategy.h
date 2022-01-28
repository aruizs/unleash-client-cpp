#ifndef UNLEASH_STRATEGY_H
#define UNLEASH_STRATEGY_H

#include "unleash/context.h"
#include <memory>
#include <string>
#include <vector>

namespace unleash {

struct Constraint {
    std::string contextName;
    std::string constraintOperator;
    std::vector<std::string> values;
};

class Strategy {
public:
    Strategy(std::string name, std::string_view parameters, std::string_view constraints = {});
    virtual ~Strategy() = default;
    virtual bool isEnabled(const Context &context) = 0;
    static std::unique_ptr<Strategy> createStrategy(std::string_view strategy, std::string_view parameters, std::string_view constraints = {});

protected:
    bool meetConstraints(const Context &context) const;

private:
    bool checkContextConstraint(const Context &context, const Constraint &constraint) const;

    const std::string m_name;
    const std::string m_parameters;
    std::vector<Constraint> m_constraints;
};
}  // namespace unleash

#endif  //UNLEASH_STRATEGY_H
