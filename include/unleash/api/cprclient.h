#ifndef UNLEASH_CPRCLIENT_H
#define UNLEASH_CPRCLIENT_H

#include "apiclient.h"

namespace unleash {
class CprClient : public ApiClient {
public:
    CprClient(const std::string &url, const std::string &name, const std::string &instanceId);
    std::string features() override;

private:
    std::string m_url;
    std::string m_instanceId;
    std::string m_name;
};
}  // namespace unleash
#endif  //UNLEASH_CPRCLIENT_H
