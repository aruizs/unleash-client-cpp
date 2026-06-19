#ifndef UNLEASH_STRATEGY_H
#define UNLEASH_STRATEGY_H

#include "unleash/context.h"
#include "unleash/variants/variant.h"
#include <memory>
#include <string>
#include <vector>

namespace unleash {

struct Constraint {
    std::string contextName;
    std::string constraintOperator;
    std::vector<std::string> values;
    std::string value;
    bool inverted = false;
    bool caseInsensitive = false;
};

class Strategy {
public:
    explicit Strategy(std::string name, std::string_view constraints = {});
    virtual ~Strategy() = default;
    virtual bool isEnabled(const Context &context) = 0;
    static std::unique_ptr<Strategy> createStrategy(std::string_view strategy, std::string_view parameters,
                                                    std::string_view constraints = {});

    void setVariants(std::string_view variants);
    bool hasVariants() const { return !m_variants.empty(); }
    variant_t resolveVariant(const Context &context) const;

protected:
    bool meetConstraints(const Context &context) const;
    virtual std::string variantStickiness() const { return "default"; }
    virtual std::string variantGroupId() const { return m_name; }

private:
    bool checkContextConstraint(const Context &context, const Constraint &constraint) const;
    std::string stickinessValue(const Context &context) const;

    const std::string m_name;
    std::vector<Constraint> m_constraints;
    std::vector<std::unique_ptr<Variant>> m_variants;
    unsigned int m_totalVariantWeight = 0;
};
}  // namespace unleash

#endif  //UNLEASH_STRATEGY_H
