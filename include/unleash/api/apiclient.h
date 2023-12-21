#ifndef UNLEASH_APICLIENT_H
#define UNLEASH_APICLIENT_H

#include <string>

namespace unleash {
class ApiClient {
public:
    virtual ~ApiClient() = default;
    virtual std::string features() = 0;
    virtual bool registration(unsigned int refreshInterval) = 0;
};
}  // namespace unleash

#endif  //UNLEASH_APICLIENT_H
