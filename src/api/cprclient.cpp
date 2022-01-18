#include "unleash/api/cprclient.h"
#include <cpr/cpr.h>

namespace unleash {
CprClient::CprClient(const std::string &url, const std::string &name, const std::string &instanceId) : m_url(std::move(url)), m_name(std::move(name)), m_instanceId(std::move(instanceId)) {}

std::string CprClient::features() {
    cpr::Response response = cpr::Get(cpr::Url{m_url + "/client/features"},
                                      cpr::Header{{"UNLEASH-INSTANCEID", m_instanceId}, {"UNLEASH-APPNAME", m_name}});
    // TODO: check status code before returning value
    return response.text;
}
}  // namespace unleash