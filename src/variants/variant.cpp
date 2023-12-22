#include "unleash/variants/variant.h"
#include <nlohmann/json.hpp>


namespace unleash {
Variant::Variant(std::string name, unsigned int weight, std::string_view payload, std::string_view overrides)
    : m_name(std::move(name)), m_weight(weight), m_payload(payload) {
    if (overrides.empty()) return;

    auto overrides_json = nlohmann::json::parse(overrides);
    for (const auto &[key, value] : overrides_json.items()) {
        if (value.contains("contextName") && value.contains("values")) {
            Override variantOverride{value["contextName"]};
            for (const auto &[valuesKey, valuesValue] : value["values"].items()) {
                variantOverride.values.push_back(valuesValue);
            }
            m_overrides.push_back(variantOverride);
        }
    }
}


}  // namespace unleash
