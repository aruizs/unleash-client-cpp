#ifndef UNLEASH_DEFAULT_H
#define UNLEASH_DEFAULT_H

#include "strategy.h"
#include <string>

namespace unleash {
class Default : public Strategy {
public:
    explicit Default(std::string_view parameters, std::string_view constraints = {});
    bool isEnabled(const Context &context) override;
};
}  // namespace unleash

#endif  //UNLEASH_DEFAULT_H
