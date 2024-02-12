#ifndef UNLEASH_CPRCLIENT_H
#define UNLEASH_CPRCLIENT_H

#include "apiclient.h"

namespace unleash {
class CprClient : public ApiClient {
public:
    CprClient(std::string url, std::string name, std::string instanceId, std::string authentication = std::string(),
              std::string caInfo = std::string());
    std::string features() override;
    bool registration(unsigned int refreshInterval) override;

private:
    std::string m_url;
    std::string m_instanceId;
    std::string m_name;
    std::string m_authentication;
    std::string m_caInfo;
};
}  // namespace unleash
#endif  //UNLEASH_CPRCLIENT_H
