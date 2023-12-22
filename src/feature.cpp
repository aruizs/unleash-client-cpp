#include "unleash/feature.h"
#include "unleash/utils/murmur3hash.h"
#include <algorithm>
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

variant_t Feature::getVariant(const unleash::Context &context) const {
    variant_t variant{"disabled", 0, false, false};
    if (!isEnabled(context)) { return variant; }

    variant.feature_enabled = true;
    if (m_variants.first.empty()) { return variant; }

    variant.enabled = true;
    constexpr uint32_t seed = 86028157;
    auto normalizedValue = normalizedMurmur3(m_name + ":" + context.userId, m_variants.second, seed);
    unsigned int weight = 0;
    for (auto &eachVariant : m_variants.first) {
        if (!eachVariant->getOverrides().empty() && checkVariant(*eachVariant, variant, context)) break;

        weight += eachVariant->getWeight();
        if (normalizedValue <= weight) {
            variant.name = eachVariant->getName();
            variant.payload = eachVariant->getPayload();
            break;
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