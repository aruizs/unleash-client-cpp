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
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>


namespace unleash {

namespace {

std::string toLower(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

bool parseDouble(const std::string &str, double &result) {
    try {
        size_t pos;
        result = std::stod(str, &pos);
        return pos == str.size();
    } catch (...) {
        return false;
    }
}

struct SemVer {
    int major = 0;
    int minor = 0;
    int patch = 0;
    std::vector<std::string> prerelease;
    bool valid = false;
};

bool isAllDigits(const std::string &s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}

SemVer parseSemVer(const std::string &version) {
    SemVer sv;
    if (version.empty() || version[0] == 'v' || version[0] == 'V') {
        return sv;
    }

    std::string versionPart = version;
    std::string prereleaseStr;

    if (auto hyphenPos = version.find('-'); hyphenPos != std::string::npos) {
        versionPart = version.substr(0, hyphenPos);
        prereleaseStr = version.substr(hyphenPos + 1);
        if (auto plusPos = prereleaseStr.find('+'); plusPos != std::string::npos) {
            prereleaseStr = prereleaseStr.substr(0, plusPos);
        }
    } else if (auto plusPos = version.find('+'); plusPos != std::string::npos) {
        versionPart = version.substr(0, plusPos);
    }

    int major, minor, patch;
    char dot1, dot2;
    std::istringstream iss(versionPart);
    if (!(iss >> major >> dot1 >> minor >> dot2 >> patch) || dot1 != '.' || dot2 != '.') {
        return sv;
    }
    std::string remaining;
    if (iss >> remaining) {
        return sv;
    }

    sv.major = major;
    sv.minor = minor;
    sv.patch = patch;
    sv.valid = true;

    if (!prereleaseStr.empty()) {
        std::istringstream prss(prereleaseStr);
        std::string token;
        while (std::getline(prss, token, '.')) {
            if (token.empty()) {
                sv.valid = false;
                return sv;
            }
            sv.prerelease.push_back(token);
        }
    }

    return sv;
}

int compareSemVer(const SemVer &a, const SemVer &b) {
    if (a.major != b.major) return a.major < b.major ? -1 : 1;
    if (a.minor != b.minor) return a.minor < b.minor ? -1 : 1;
    if (a.patch != b.patch) return a.patch < b.patch ? -1 : 1;

    if (a.prerelease.empty() && b.prerelease.empty()) return 0;
    if (a.prerelease.empty()) return 1;
    if (b.prerelease.empty()) return -1;

    size_t maxLen = std::max(a.prerelease.size(), b.prerelease.size());
    for (size_t i = 0; i < maxLen; ++i) {
        if (i >= a.prerelease.size()) return -1;
        if (i >= b.prerelease.size()) return 1;

        const std::string &pa = a.prerelease[i];
        const std::string &pb = b.prerelease[i];

        bool aIsNum = isAllDigits(pa);
        bool bIsNum = isAllDigits(pb);

        if (aIsNum && bIsNum) {
            int na = std::stoi(pa);
            int nb = std::stoi(pb);
            if (na != nb) return na < nb ? -1 : 1;
        } else if (aIsNum) {
            return -1;
        } else if (bIsNum) {
            return 1;
        } else {
            int cmp = pa.compare(pb);
            if (cmp != 0) return cmp < 0 ? -1 : 1;
        }
    }
    return 0;
}

std::chrono::system_clock::time_point parseIso8601(const std::string &dateStr) {
    std::tm tm = {};
    std::istringstream ss(dateStr);

    int tzOffsetMinutes = 0;
    std::string datePart;

    auto plusPos = dateStr.find('+');
    auto minusPos = dateStr.rfind('-');
    bool hasTimezone = false;

    if (plusPos != std::string::npos && plusPos > 10) {
        datePart = dateStr.substr(0, plusPos);
        std::string tzPart = dateStr.substr(plusPos + 1);
        int hours = 0, minutes = 0;
        if (std::sscanf(tzPart.c_str(), "%d:%d", &hours, &minutes) >= 1) {
            tzOffsetMinutes = hours * 60 + minutes;
        }
        hasTimezone = true;
    } else if (minusPos != std::string::npos && minusPos > 10) {
        datePart = dateStr.substr(0, minusPos);
        std::string tzPart = dateStr.substr(minusPos + 1);
        int hours = 0, minutes = 0;
        if (std::sscanf(tzPart.c_str(), "%d:%d", &hours, &minutes) >= 1) {
            tzOffsetMinutes = -(hours * 60 + minutes);
        }
        hasTimezone = true;
    } else {
        datePart = dateStr;
        if (datePart.back() == 'Z') {
            datePart.pop_back();
        }
    }

    if (auto dotPos = datePart.find('.'); dotPos != std::string::npos) {
        datePart = datePart.substr(0, dotPos);
    }

    std::istringstream dateStream(datePart);
    dateStream >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

    auto tp = std::chrono::system_clock::from_time_t(
#ifdef _WIN32
        _mkgmtime(&tm)
#else
        timegm(&tm)
#endif
    );

    if (hasTimezone) {
        tp -= std::chrono::minutes(tzOffsetMinutes);
    }

    return tp;
}

bool isValidOperator(const std::string &op) {
    static const std::vector<std::string> validOperators = {
        "IN", "NOT_IN",
        "STR_STARTS_WITH", "STR_ENDS_WITH", "STR_CONTAINS",
        "NUM_EQ", "NUM_GT", "NUM_GTE", "NUM_LT", "NUM_LTE",
        "DATE_AFTER", "DATE_BEFORE",
        "SEMVER_EQ", "SEMVER_GT", "SEMVER_GTE", "SEMVER_LT", "SEMVER_LTE"
    };
    return std::find(validOperators.begin(), validOperators.end(), op) != validOperators.end();
}

}  // namespace

Strategy::Strategy(std::string name, std::string_view constraints) : m_name(std::move(name)) {
    if (constraints.empty()) {
        return;
    }
    auto constraint_json = nlohmann::json::parse(constraints);
    for (const auto &[key, value] : constraint_json.items()) {
        if (!value.contains("contextName") || !value.contains("operator")) {
            continue;
        }

        std::string op = value["operator"];
        if (!isValidOperator(op)) {
            Constraint invalidConstraint;
            invalidConstraint.contextName = value["contextName"];
            invalidConstraint.constraintOperator = op;
            m_constraints.push_back(invalidConstraint);
            continue;
        }

        Constraint strategyConstraint;
        strategyConstraint.contextName = value["contextName"];
        strategyConstraint.constraintOperator = op;
        strategyConstraint.inverted = value.value("inverted", false);
        strategyConstraint.caseInsensitive = value.value("caseInsensitive", false);

        if (value.contains("values")) {
            for (const auto &[valuesKey, valuesValue] : value["values"].items()) {
                strategyConstraint.values.push_back(valuesValue);
            }
        }
        if (value.contains("value")) {
            strategyConstraint.value = value["value"];
        }

        m_constraints.push_back(strategyConstraint);
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
    const std::string &op = constraint.constraintOperator;

    if (!isValidOperator(op)) {
        return false;
    }

    std::string contextValue;
    bool contextValueExists = false;

    if (constraint.contextName == "environment") {
        contextValue = context.environment;
        contextValueExists = !contextValue.empty();
    } else if (constraint.contextName == "appName") {
        contextValue = context.appName;
        contextValueExists = !contextValue.empty();
    } else if (constraint.contextName == "userId") {
        contextValue = context.userId;
        contextValueExists = !contextValue.empty();
    } else if (constraint.contextName == "currentTime") {
        contextValue = context.currentTime;
        contextValueExists = !contextValue.empty();
    } else if (auto it = context.properties.find(constraint.contextName); it != context.properties.end()) {
        contextValue = it->second;
        contextValueExists = true;
    }

    bool result = false;

    if (op == "IN") {
        if (!contextValueExists) {
            result = false;
        } else {
            result = std::find(constraint.values.begin(), constraint.values.end(), contextValue) != constraint.values.end();
        }
    } else if (op == "NOT_IN") {
        if (!contextValueExists) {
            result = true;
        } else {
            result = std::find(constraint.values.begin(), constraint.values.end(), contextValue) == constraint.values.end();
        }
    } else if (op == "STR_STARTS_WITH") {
        if (!contextValueExists) {
            result = false;
        } else {
            std::string ctxVal = constraint.caseInsensitive ? toLower(contextValue) : contextValue;
            result = std::any_of(constraint.values.begin(), constraint.values.end(), [&](const std::string &v) {
                std::string val = constraint.caseInsensitive ? toLower(v) : v;
                return ctxVal.size() >= val.size() && ctxVal.compare(0, val.size(), val) == 0;
            });
        }
    } else if (op == "STR_ENDS_WITH") {
        if (!contextValueExists) {
            result = false;
        } else {
            std::string ctxVal = constraint.caseInsensitive ? toLower(contextValue) : contextValue;
            result = std::any_of(constraint.values.begin(), constraint.values.end(), [&](const std::string &v) {
                std::string val = constraint.caseInsensitive ? toLower(v) : v;
                return ctxVal.size() >= val.size() && ctxVal.compare(ctxVal.size() - val.size(), val.size(), val) == 0;
            });
        }
    } else if (op == "STR_CONTAINS") {
        if (!contextValueExists) {
            result = false;
        } else {
            std::string ctxVal = constraint.caseInsensitive ? toLower(contextValue) : contextValue;
            result = std::any_of(constraint.values.begin(), constraint.values.end(), [&](const std::string &v) {
                std::string val = constraint.caseInsensitive ? toLower(v) : v;
                return ctxVal.find(val) != std::string::npos;
            });
        }
    } else if (op == "NUM_EQ" || op == "NUM_GT" || op == "NUM_GTE" || op == "NUM_LT" || op == "NUM_LTE") {
        if (!contextValueExists) {
            result = false;
        } else {
            double contextNum, constraintNum;
            if (!parseDouble(contextValue, contextNum) || !parseDouble(constraint.value, constraintNum)) {
                result = false;
            } else if (op == "NUM_EQ") {
                result = contextNum == constraintNum;
            } else if (op == "NUM_GT") {
                result = contextNum > constraintNum;
            } else if (op == "NUM_GTE") {
                result = contextNum >= constraintNum;
            } else if (op == "NUM_LT") {
                result = contextNum < constraintNum;
            } else if (op == "NUM_LTE") {
                result = contextNum <= constraintNum;
            }
        }
    } else if (op == "DATE_AFTER" || op == "DATE_BEFORE") {
        if (!contextValueExists) {
            result = false;
        } else {
            auto contextTime = parseIso8601(contextValue);
            auto constraintTime = parseIso8601(constraint.value);
            if (op == "DATE_AFTER") {
                result = contextTime > constraintTime;
            } else {
                result = contextTime < constraintTime;
            }
        }
    } else if (op == "SEMVER_EQ" || op == "SEMVER_GT" || op == "SEMVER_GTE" || op == "SEMVER_LT" || op == "SEMVER_LTE") {
        if (!contextValueExists) {
            result = false;
        } else {
            SemVer contextSv = parseSemVer(contextValue);
            SemVer constraintSv = parseSemVer(constraint.value);
            if (!contextSv.valid || !constraintSv.valid) {
                result = false;
            } else {
                int cmp = compareSemVer(contextSv, constraintSv);
                if (op == "SEMVER_EQ") {
                    result = cmp == 0;
                } else if (op == "SEMVER_GT") {
                    result = cmp > 0;
                } else if (op == "SEMVER_GTE") {
                    result = cmp >= 0;
                } else if (op == "SEMVER_LT") {
                    result = cmp < 0;
                } else if (op == "SEMVER_LTE") {
                    result = cmp <= 0;
                }
            }
        }
    }

    return constraint.inverted ? !result : result;
}


}  // namespace unleash
