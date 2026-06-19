#ifndef UNLEASH_APICLIENT_H
#define UNLEASH_APICLIENT_H

#include <string>

namespace unleash {
class ApiClient {
public:
    virtual ~ApiClient() = default;
    virtual std::string features() = 0;
    virtual bool registration(unsigned int refreshInterval) = 0;
    // Non-pure with a default so existing custom ApiClient implementations keep compiling.
    // Override it to enable usage-metrics reporting.
    virtual bool metrics(const std::string & /*payload*/) { return false; }
};
}  // namespace unleash

#endif  //UNLEASH_APICLIENT_H
