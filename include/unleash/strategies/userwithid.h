#ifndef UNLEASH_USERWITHID_H
#define UNLEASH_USERWITHID_H
#include "strategy.h"
#include <string>
#include <vector>

namespace unleash {
class UserWithId : public Strategy {
public:
    explicit UserWithId(std::string_view parameters, std::string_view constraints = {});
    bool isEnabled(const Context &context) override;

private:
    std::vector<std::string> m_userIds;
};
}  // namespace unleash
#endif  //UNLEASH_USERWITHID_H
