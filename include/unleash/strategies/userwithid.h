#ifndef UNLEASH_USERWITHID_H
#define UNLEASH_USERWITHID_H
#include "strategy.h"
#include <string>
#include <vector>

namespace unleash {
class UserWithId : public Strategy {
public:
    UserWithId(const std::string &parameters);
    bool isEnabled(const Context &context);

private:
    std::vector<std::string> m_userIds;
};
}  // namespace unleash
#endif  //UNLEASH_USERWITHID_H
