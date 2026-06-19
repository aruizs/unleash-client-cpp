#ifndef UNLEASH_VARIANTS_H
#define UNLEASH_VARIANTS_H
#include <string>
#include <vector>

namespace unleash {

struct Context;

struct Override {
    std::string contextName;
    std::vector<std::string> values;
};

/// @brief Result of a UnleashClient::variant() query.
struct variant_t {
    std::string name;       ///< Variant name (@c "disabled" when no variant applies).
    unsigned int weight;    ///< Configured weight of the variant.
    bool enabled;           ///< Whether a real variant was selected.
    bool feature_enabled;   ///< Whether the underlying feature flag is enabled.
    std::string payload;    ///< Variant payload, serialized as JSON.
};

class Variant {
public:
    Variant(std::string name, unsigned int weight, std::string_view payload, std::string_view overrides);
    unsigned int getWeight() const { return m_weight; }
    const std::string &getName() const { return m_name; }
    const std::vector<Override> &getOverrides() const { return m_overrides; }
    const std::string &getPayload() const { return m_payload; }

private:
    std::string m_name;
    unsigned int m_weight;
    std::string m_payload;
    std::vector<Override> m_overrides;
};
}  // namespace unleash


#endif  //UNLEASH_VARIANTS_H