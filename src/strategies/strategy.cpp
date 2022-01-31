#include "unleash/strategies/strategy.h"
#include "unleash/strategies/applicationhostname.h"
#include "unleash/strategies/default.h"
#include "unleash/strategies/flexiblerollout.h"
#include "unleash/strategies/gradualrolloutrandom.h"
#include "unleash/strategies/gradualrolloutsessionid.h"
#include "unleash/strategies/gradualrolloutuserid.h"
#include "unleash/strategies/remoteaddress.h"
#include "unleash/strategies/userwithid.h"
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>


namespace unleash {
Strategy::Strategy(std::string name, std::string_view parameters, std::string_view constraints) : m_name(std::move(name)) {
    if (!constraints.empty()) {
        auto constraint_json = nlohmann::json::parse(constraints);
        for (const auto &[key, value] : constraint_json.items()) {
            if ((value.contains("contextName") && value.contains("operator") && value.contains("values")) && (value["operator"] == "IN" || value["operator"] == "NOT_IN")) {
                Constraint strategyConstraint{value["contextName"], value["operator"]};
                for (const auto &[valuesKey, valuesValue] : value["values"].items()) {
                    strategyConstraint.values.push_back(valuesValue);
                }
                m_constraints.push_back(strategyConstraint);
            }
        }
    }
}

std::unique_ptr<Strategy> Strategy::createStrategy(std::string_view strategy, std::string_view parameters, std::string_view constraints) {
    if (strategy == "default")
        return std::make_unique<Default>(parameters, constraints);
    else if (strategy == "userWithId")
        return std::make_unique<UserWithId>(parameters, constraints);
    else if (strategy == "applicationHostname")
        return std::make_unique<ApplicationHostname>(parameters, constraints);
    else if (strategy == "flexibleRollout")
        return std::make_unique<FlexibleRollout>(parameters, constraints);
    else if (strategy == "gradualRolloutUserId")
        return std::make_unique<GradualRolloutUserId>(parameters, constraints);
    else if (strategy == "gradualRolloutSessionId")
        return std::make_unique<GradualRolloutSessionId>(parameters, constraints);
    else if (strategy == "gradualRolloutRandom")
        return std::make_unique<GradualRolloutRandom>(parameters, constraints);
    else if (strategy == "remoteAddress")
        return std::make_unique<RemoteAddress>(parameters, constraints);
    return nullptr;
}

bool Strategy::meetConstraints(const Context &context) const {
    if (m_constraints.empty())
        return true;
    return (std::all_of(m_constraints.cbegin(), m_constraints.cend(), [&](const auto &constraintItem) { return checkContextConstraint(context, constraintItem); }));
}

bool Strategy::checkContextConstraint(const Context &context, const Constraint &constraint) const {
    bool inCondition = (constraint.constraintOperator == "IN");
    bool contextIn = false;
    if (constraint.contextName == "environment") {
        contextIn = (std::find(constraint.values.begin(), constraint.values.end(), context.environment) != constraint.values.end());
    } else if (constraint.contextName == "appName") {
        contextIn = (std::find(constraint.values.begin(), constraint.values.end(), context.appName) != constraint.values.end());
    } else if (constraint.contextName == "userId") {
        contextIn = (std::find(constraint.values.begin(), constraint.values.end(), context.userId) != constraint.values.end());
    } else if (context.properties.find(constraint.contextName) != context.properties.end()) {
        contextIn = (std::find(constraint.values.begin(), constraint.values.end(), context.properties.at(constraint.contextName)) != constraint.values.end());
    }

    if (contextIn == inCondition)
        return true;

    return false;
}


}  // namespace unleash
