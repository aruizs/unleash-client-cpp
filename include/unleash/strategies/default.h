#ifndef UNLEASH_DEFAULT_H
#define UNLEASH_DEFAULT_H

#include "strategy.h"
#include <string>

namespace unleash {
class Default : public Strategy {
public:
    explicit Default(std::string_view parameters);
    bool isEnabled(const Context &context) override;
};
}

#endif  //UNLEASH_DEFAULT_H
