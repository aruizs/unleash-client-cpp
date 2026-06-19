#include "unleash/feature.h"
#include "unleash/utils/murmur3hash.h"
#include <algorithm>
#include <nlohmann/json.hpp>
#include <numeric>

namespace unleash {
Feature::Feature(std::string name, std::vector<std::unique_ptr<Strategy>> strategies, bool enable)
    : m_name(std::move(name)), m_enabled(enable), m_strategies(std::move(strategies)) {}

bool Feature::isEnabled(const Context &context) const {
    if (m_enabled) {
        if (m_strategies.empty()) return true;
        for (const auto &strategy : m_strategies) {
            if (strategy->isEnabled(context)) { return true; }
        }
    }
    return false;
}

void Feature::setVariants(std::pair<std::vector<std::unique_ptr<Variant>>, unsigned int> variants) {
    m_variants = std::move(variants);
}

void Feature::setDependencies(std::string_view dependencies) {
    if (dependencies.empty()) {
        return;
    }
    auto dependenciesJson = nlohmann::json::parse(dependencies);
    for (const auto &[key, value] : dependenciesJson.items()) {
        Dependency dependency;
        dependency.feature = value["feature"].get<std::string>();
        dependency.enabled = value.value("enabled", true);
        if (value.contains("variants")) {
            dependency.hasVariants = true;
            for (const auto &[variantKey, variantValue] : value["variants"].items()) {
                dependency.variants.push_back(variantValue.get<std::string>());
            }
        }
        m_dependencies.push_back(std::move(dependency));
    }
}

variant_t Feature::getVariant(const unleash::Context &context) const {
    variant_t variant{"disabled", 0, false, false};
    if (!isEnabled(context)) { return variant; }

    variant.feature_enabled = true;

    // Strategy variants take precedence over feature variants: use the variants of the first
    // satisfied strategy that defines any.
    for (const auto &strategy : m_strategies) {
        if (strategy->hasVariants() && strategy->isEnabled(context)) {
            variant_t strategyVariant = strategy->resolveVariant(context);
            strategyVariant.feature_enabled = true;
            return strategyVariant;
        }
    }

    if (m_variants.first.empty()) { return variant; }

    variant.enabled = true;
    constexpr uint32_t seed = 86028157;
    auto normalizedValue = normalizedMurmur3(m_name + ":" + context.userId, m_variants.second, seed);
    unsigned int weight = 0;
    for (auto &eachVariant : m_variants.first) {
        if (!eachVariant->getOverrides().empty() && checkVariant(*eachVariant, variant, context)) {
            return variant;
        }
        weight += eachVariant->getWeight();
        if (normalizedValue <= weight) {
            variant.name = eachVariant->getName();
            variant.payload = eachVariant->getPayload();
            return variant;
        }
    }

    return variant;
}

bool Feature::checkVariant(const unleash::Variant &variantInput, variant_t &variantResponse,
                           const unleash::Context &context) const {
    if (auto contextIt = std::find_if(variantInput.getOverrides().begin(), variantInput.getOverrides().end(),
                                      [](const Override &o) { return o.contextName == "userId"; });
        contextIt != variantInput.getOverrides().end()) {
        auto valuesIt = std::find((*contextIt).values.begin(), (*contextIt).values.end(), context.userId);

        if (valuesIt != (*contextIt).values.end()) {
            variantResponse.name = variantInput.getName();
            variantResponse.payload = variantInput.getPayload();
            return true;
        }
    }
    return false;
};
}  // namespace unleash