#ifndef UNLEASH_STRATEGY_H
#define UNLEASH_STRATEGY_H

#include "unleash/context.h"
#include "unleash/export.h"
#include "unleash/variants/variant.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace unleash {

/// @brief A single activation-strategy constraint evaluated against the Context.
struct Constraint {
    std::string contextName;         ///< Context field the constraint applies to.
    std::string constraintOperator;  ///< Operator (e.g. IN, STR_CONTAINS, SEMVER_GT, IN_CIDR).
    std::vector<std::string> values; ///< Candidate values for multi-value operators.
    std::string value;               ///< Single value for single-value operators.
    bool inverted = false;           ///< Negates the result when set.
    bool caseInsensitive = false;    ///< Case-insensitive comparison for string operators.
};

/// @brief Base class for activation strategies; createStrategy() builds the built-in ones.
///
/// Custom strategies subclass this, pass their constraints to the base constructor, and call
/// meetConstraints() from isEnabled(). Register them with UnleashClientBuilder::registerStrategy().
class UNLEASH_EXPORT Strategy {
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

/// @brief Builds a custom Strategy from a feature's @p parameters and @p constraints (both JSON).
/// Register one with UnleashClientBuilder::registerStrategy().
using StrategyFactory =
        std::function<std::unique_ptr<Strategy>(std::string_view parameters, std::string_view constraints)>;
}  // namespace unleash

#endif  //UNLEASH_STRATEGY_H
