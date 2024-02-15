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
#include <ctime>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>


namespace unleash {
Strategy::Strategy(std::string name, std::string_view constraints) : m_name(std::move(name)) {
    if (!constraints.empty()) {
        auto constraint_json = nlohmann::json::parse(constraints);
        for (const auto &[key, value] : constraint_json.items()) {
            if ((value.contains("contextName") && value.contains("operator"))) {
                Constraint strategyConstraint{value["contextName"], value["operator"]};
                if (value.contains("values")){
                    for (const auto &[valuesKey, valuesValue] : value["values"].items()) {
                        strategyConstraint.values.push_back(valuesValue);
                    }
                } else if (value.contains("value")){
                    strategyConstraint.values.push_back(value["value"]);
                }
                strategyConstraint.inverted = (value.contains("inverted") && value["inverted"] == true);
                strategyConstraint.caseInsensitive = (value.contains("caseInsensitive") && value["caseInsensitive"] == true);
                if (strategyConstraint.constraintOperator == "NOT_IN"){
                    strategyConstraint.constraintOperator = "IN";
                    strategyConstraint.inverted = !strategyConstraint.inverted;
                }
                m_constraints.push_back(strategyConstraint);
            }
        }
    }
}

std::unique_ptr<Strategy> Strategy::createStrategy(std::string_view strategy, std::string_view parameters,
                                                   std::string_view constraints) {
    if (strategy == "default") return std::make_unique<Default>(parameters, constraints);
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
    if (m_constraints.empty()) return true;
    return (std::all_of(m_constraints.cbegin(), m_constraints.cend(),
                        [&](const auto &constraintItem) { return checkContextConstraint(context, constraintItem); }));
}

bool Strategy::checkContextConstraint(const Context &context, const Constraint &constraint) const {
    bool match = false;
    if (constraint.contextName == "environment") {
        match = evalConstraintOperator(context.environment, constraint);
    } else if (constraint.contextName == "appName") {
        match = evalConstraintOperator(context.appName, constraint);
    } else if (constraint.contextName == "userId") {
        match = evalConstraintOperator(context.userId, constraint);
    } else if (constraint.contextName == "remoteAddress") {
        match = evalConstraintOperator(context.remoteAddress, constraint);
    } else if (constraint.contextName == "sessionId") {
        match = evalConstraintOperator(context.sessionId, constraint);
    } else if (constraint.contextName == "currentTime") {
        match = evalConstraintOperator(context.currentTime, constraint);
    }else if (context.properties.find(constraint.contextName) != context.properties.end()) {
        match = evalConstraintOperator(context.properties.at(constraint.contextName), constraint);
    }

    return match != constraint.inverted;
}

bool Strategy::evalConstraintOperator(const std::string &contextVariable, const Constraint &constraint) const {
    if (constraint.constraintOperator == "IN"){
        return (std::find(constraint.values.begin(), constraint.values.end(), contextVariable) != constraint.values.end());
    } else if (constraint.constraintOperator == "STR_CONTAINS") {
        for (auto word : constraint.values){
            const auto iterator = std::search(contextVariable.begin(), contextVariable.end(), word.begin(), word.end(),
                [&constraint](unsigned char first, unsigned char second){ return (constraint.caseInsensitive ? std::tolower(first) == std::tolower(second) : first == second);});
            if (iterator != contextVariable.end())
                return true;
        }
        return false;
    } else if (constraint.constraintOperator == "STR_STARTS_WITH") {
        for (const auto& start : constraint.values){
            const auto iterator = std::search(contextVariable.begin(), contextVariable.end(), start.begin(), start.end(),
                [&constraint](unsigned char first, unsigned char second){ return (constraint.caseInsensitive ? std::tolower(first) == std::tolower(second) : first == second);});
            if (iterator == contextVariable.begin())
                return true;
        }
        return false;
    } else if (constraint.constraintOperator == "STR_ENDS_WITH") {
        for (const auto& termination : constraint.values){
            const auto iterator = std::search(contextVariable.begin(), contextVariable.end(), termination.begin(), termination.end(),
                [&constraint](unsigned char first, unsigned char second){ return (constraint.caseInsensitive ? std::tolower(first) == std::tolower(second) : first == second);});
            if (iterator == contextVariable.end() - termination.length())
                return true;
        }
        return false;
    } else if (constraint.constraintOperator == "NUM_EQ") {
        return std::stod(contextVariable) == std::stod(constraint.values.front());
    } else if (constraint.constraintOperator == "NUM_GT") {
        return std::stod(contextVariable) > std::stod(constraint.values.front());
    } else if (constraint.constraintOperator == "NUM_GTE") {
        return std::stod(contextVariable) >= std::stod(constraint.values.front());
    } else if (constraint.constraintOperator == "NUM_LT") {
        return std::stod(contextVariable) < std::stod(constraint.values.front());
    } else if (constraint.constraintOperator == "NUM_LTE") {
        return std::stod(contextVariable) <= std::stod(constraint.values.front());
    } else if (constraint.constraintOperator == "DATE_AFTER"){
        if (constraint.values.front().length() >= 29)
            return contextVariable > parseDateWithTimezone(constraint.values.front());
        return contextVariable > constraint.values.front();
    } else if (constraint.constraintOperator == "DATE_BEFORE"){
        if (constraint.values.front().length() >= 29)
            return contextVariable < parseDateWithTimezone(constraint.values.front());
        return contextVariable < constraint.values.front();
    }
    return false;
}

std::string Strategy::parseDateWithTimezone(const std::string &dateWithTimezone) const {
    std::tm t = {};
    std::istringstream ss(dateWithTimezone);
    ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()){
        std::cerr << "Unable to parse date" << std::endl;
        return "";
    }
    const int hours = std::stoi(dateWithTimezone.substr(24,2));
    const int minutes = std::stoi(dateWithTimezone.substr(27,2));
    if (dateWithTimezone[23] == '-'){
        t.tm_hour += hours;
        t.tm_min += minutes;
    } else {
        t.tm_hour -= hours;
        t.tm_min -= minutes;
    }
    mktime(&t);
    std::stringstream out;
    out << std::put_time(&t, "%Y-%m-%dT%H:%M:%S.000Z");
    return out.str();
}

}  // namespace unleash
