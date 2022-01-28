#ifndef UNLEASH_REMOTEADDRESS_H
#define UNLEASH_REMOTEADDRESS_H
#include "strategy.h"
#include <string>
#include <vector>

namespace unleash {
class RemoteAddress : public Strategy {
public:
    explicit RemoteAddress(std::string_view parameters, std::string_view constraints = {});
    bool isEnabled(const Context &context) override;

private:
    std::vector<std::string> m_ips;
};
}  // namespace unleash
#endif  //UNLEASH_REMOTEADDRESS_H
