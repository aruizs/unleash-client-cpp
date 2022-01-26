#include "unleash/api/cprclient.h"
#include <cpr/cpr.h>
#include <iostream>

namespace unleash {
CprClient::CprClient(std::string url, std::string name, std::string instanceId, std::string authentication) : m_url(std::move(url)), m_instanceId(std::move(instanceId)), m_name(std::move(name)), m_authentication(std::move(authentication)) {}

std::string CprClient::features() {
    cpr::Response response = cpr::Get(cpr::Url{m_url + "/client/features"},
                                      cpr::Header{{"UNLEASH-INSTANCEID", m_instanceId}, {"UNLEASH-APPNAME", m_name}, {"Authorization", m_authentication}});
    if (response.status_code == 0) {
        std::cerr << response.error.message << std::endl;
        return std::string{};
    } else if (response.status_code >= 400) {
        std::cerr << "Error [" << response.status_code << "] making request" << std::endl;
        return std::string{};
    }
    return response.text;
}
}  // namespace unleash