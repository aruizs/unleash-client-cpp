#ifndef UNLEASH_FEATURE_H
#define UNLEASH_FEATURE_H
#include "unleash/strategies/strategy.h"
#include "unleash/variants/variant.h"
#include <memory>
#include <string>
#include <vector>

namespace unleash {
struct Context;
class Feature {
public:
    Feature(std::string name, std::vector<std::unique_ptr<Strategy>> strategies, bool enable);
    void setVariants(std::pair<std::vector<std::unique_ptr<Variant>>, unsigned int> variants);
    bool isEnabled(const Context &context) const;
    variant_t getVariant(const Context &context) const;

private:
    bool checkVariant(const unleash::Variant &variantInput, variant_t &variantResponse, const Context &context) const;

    std::string m_name;
    bool m_enabled;
    std::vector<std::unique_ptr<Strategy>> m_strategies;
    std::pair<std::vector<std::unique_ptr<Variant>>, unsigned int> m_variants;
};
}  // namespace unleash
#endif  //UNLEASH_FEATURE_H
