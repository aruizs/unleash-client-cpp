#ifndef UNLEASH_DEFAULT_H
#define UNLEASH_DEFAULT_H

#include "strategy.h"
#include <string>

namespace unleash {
class Default : public Strategy {
public:
    Default(const std::string &parameters);
    bool isEnabled(const Context &context);
};
}

#endif  //UNLEASH_DEFAULT_H
