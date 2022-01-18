#ifndef UNLEASH_REMOTEADDRESS_H
#define UNLEASH_REMOTEADDRESS_H
#include "strategy.h"
#include <string>
#include <vector>

namespace unleash {
class RemoteAddress : public Strategy {
public:
    RemoteAddress(const std::string &parameters);
    bool isEnabled(const Context &context);

private:
    std::vector<std::string> m_ips;
};
}  // namespace unleash
#endif  //UNLEASH_REMOTEADDRESS_H
