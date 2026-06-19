#include "unleash/strategies/strategy.h"
#include "unleash/strategies/applicationhostname.h"
#include "unleash/strategies/default.h"
#include "unleash/strategies/flexiblerollout.h"
#include "unleash/strategies/gradualrolloutrandom.h"
#include "unleash/strategies/gradualrolloutsessionid.h"
#include "unleash/strategies/gradualrolloutuserid.h"
#include "unleash/strategies/remoteaddress.h"
#include "unleash/strategies/userwithid.h"
#include "unleash/utils/murmur3hash.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif


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
        "SEMVER_EQ", "SEMVER_GT", "SEMVER_GTE", "SEMVER_LT", "SEMVER_LTE",
        "REGEX", "IN_CIDR"
    };
    return std::find(validOperators.begin(), validOperators.end(), op) != validOperators.end();
}

// Returns true when the address falls within the given CIDR (or matches an exact IP when no prefix
// is present). Supports IPv4 and IPv6; an invalid address or CIDR yields false.
bool ipInCidr(const std::string &address, const std::string &cidr) {
    std::string network = cidr;
    int prefix = -1;
    if (auto slash = cidr.find('/'); slash != std::string::npos) {
        network = cidr.substr(0, slash);
        try {
            size_t pos;
            prefix = std::stoi(cidr.substr(slash + 1), &pos);
            if (pos != cidr.size() - slash - 1) return false;
        } catch (...) {
            return false;
        }
    }

    in_addr address4{};
    in_addr network4{};
    if (inet_pton(AF_INET, address.c_str(), &address4) == 1 && inet_pton(AF_INET, network.c_str(), &network4) == 1) {
        int bits = (prefix < 0) ? 32 : prefix;
        if (bits < 0 || bits > 32) return false;
        uint32_t addressBits = ntohl(address4.s_addr);
        uint32_t networkBits = ntohl(network4.s_addr);
        uint32_t mask = (bits == 0) ? 0 : (0xFFFFFFFFu << (32 - bits));
        return (addressBits & mask) == (networkBits & mask);
    }

    in6_addr address6{};
    in6_addr network6{};
    if (inet_pton(AF_INET6, address.c_str(), &address6) == 1 && inet_pton(AF_INET6, network.c_str(), &network6) == 1) {
        int bits = (prefix < 0) ? 128 : prefix;
        if (bits < 0 || bits > 128) return false;
        int fullBytes = bits / 8;
        int remainingBits = bits % 8;
        for (int i = 0; i < fullBytes; ++i) {
            if (address6.s6_addr[i] != network6.s6_addr[i]) return false;
        }
        if (remainingBits > 0) {
            auto mask = static_cast<uint8_t>(0xFF << (8 - remainingBits));
            if ((address6.s6_addr[fullBytes] & mask) != (network6.s6_addr[fullBytes] & mask)) return false;
        }
        return true;
    }

    return false;
}

// RE2 (used by the Unleash server) rejects backreferences, while std::regex would happily evaluate
// them. Detect "\1".."\9" so such patterns are treated as invalid and disable the toggle.
bool hasBackreference(const std::string &pattern) {
    for (size_t i = 0; i + 1 < pattern.size(); ++i) {
        if (pattern[i] == '\\') {
            char next = pattern[i + 1];
            if (next >= '1' && next <= '9') return true;
            ++i;  // skip the escaped character
        }
    }
    return false;
}

// std::regex has no syntax for inline flags such as (?i), (?m) or (?s). Extract any leading/standalone
// flag groups, translate them to std::regex flags, and emulate dot-all by widening '.' to [\s\S].
std::string processInlineFlags(std::string pattern, std::regex::flag_type &flags) {
    static const std::regex flagGroup(R"(\(\?([ims]+)\))");
    bool dotAll = false;
    for (std::sregex_iterator it(pattern.begin(), pattern.end(), flagGroup), end; it != end; ++it) {
        const std::string letters = (*it)[1].str();
        if (letters.find('i') != std::string::npos) flags |= std::regex::icase;
        if (letters.find('m') != std::string::npos) flags |= std::regex::multiline;
        if (letters.find('s') != std::string::npos) dotAll = true;
    }
    pattern = std::regex_replace(pattern, flagGroup, "");

    if (!dotAll) {
        return pattern;
    }
    std::string widened;
    bool inClass = false;
    for (size_t i = 0; i < pattern.size(); ++i) {
        char c = pattern[i];
        if (c == '\\' && i + 1 < pattern.size()) {
            widened += c;
            widened += pattern[++i];
        } else if (c == '[') {
            inClass = true;
            widened += c;
        } else if (c == ']') {
            inClass = false;
            widened += c;
        } else if (c == '.' && !inClass) {
            widened += R"([\s\S])";
        } else {
            widened += c;
        }
    }
    return widened;
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

void Strategy::setVariants(std::string_view variants) {
    if (variants.empty()) {
        return;
    }
    auto variantsJson = nlohmann::json::parse(variants);
    for (const auto &[key, value] : variantsJson.items()) {
        std::string payload = value.contains("payload") ? value["payload"].dump() : "";
        std::string overrides = value.contains("overrides") ? value["overrides"].dump() : "";
        m_variants.push_back(std::make_unique<Variant>(value["name"], value["weight"], payload, overrides));
        m_totalVariantWeight += value["weight"].get<unsigned int>();
    }
}

std::string Strategy::stickinessValue(const Context &context) const {
    std::string stickiness = variantStickiness();
    if (stickiness == "default") {
        if (!context.userId.empty()) return context.userId;
        if (!context.sessionId.empty()) return context.sessionId;
        return "";
    }
    if (stickiness == "userId") return context.userId;
    if (stickiness == "sessionId") return context.sessionId;
    if (auto it = context.properties.find(stickiness); it != context.properties.end()) return it->second;
    return "";
}

variant_t Strategy::resolveVariant(const Context &context) const {
    variant_t variant{"disabled", 0, false, false};
    if (m_variants.empty()) {
        return variant;
    }
    variant.enabled = true;
    constexpr uint32_t seed = 86028157;
    auto normalizedValue = normalizedMurmur3(variantGroupId() + ":" + stickinessValue(context), m_totalVariantWeight, seed);
    unsigned int weight = 0;
    for (const auto &eachVariant : m_variants) {
        weight += eachVariant->getWeight();
        if (normalizedValue <= weight) {
            variant.name = eachVariant->getName();
            variant.payload = eachVariant->getPayload();
            return variant;
        }
    }
    return variant;
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
    } else if (constraint.contextName == "sessionId") {
        contextValue = context.sessionId;
        contextValueExists = !contextValue.empty();
    } else if (constraint.contextName == "remoteAddress") {
        contextValue = context.remoteAddress;
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
    } else if (op == "IN_CIDR") {
        if (!contextValueExists) {
            result = false;
        } else {
            result = std::any_of(constraint.values.begin(), constraint.values.end(),
                                 [&](const std::string &cidr) { return ipInCidr(contextValue, cidr); });
        }
    } else if (op == "REGEX") {
        if (!contextValueExists || hasBackreference(constraint.value)) {
            return false;
        }
        auto flags = std::regex::ECMAScript;
        if (constraint.caseInsensitive) flags |= std::regex::icase;
        // processInlineFlags may add flags, so resolve the pattern before constructing the regex.
        const std::string processedPattern = processInlineFlags(constraint.value, flags);
        try {
            std::regex pattern(processedPattern, flags);
            result = std::regex_search(contextValue, pattern);
        } catch (const std::regex_error &) {
            return false;  // invalid regex disables the toggle, regardless of inversion
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
