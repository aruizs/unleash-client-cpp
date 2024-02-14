#include "unleash/api/cprclient.h"
#include <chrono>
#include <cpr/cpr.h>
#include <cpr/ssl_options.h>
#include <iostream>
#include <nlohmann/json.hpp>

namespace unleash {
CprClient::CprClient(std::string url, std::string name, std::string instanceId, std::string authentication,
                     std::string caInfo)
    : m_url(std::move(url)), m_instanceId(std::move(instanceId)), m_name(std::move(name)),
      m_authentication(std::move(authentication)), m_caInfo(std::move(caInfo)) {}

std::string CprClient::features() {
    cpr::SslOptions sslOptions;


    if (!m_caInfo.empty()) {
        // since CaInfo takes an rvalue reference and we don't want to move the m_caInfo in
        // we create a copy of it and move it in
        std::string copyOfCaInfo = m_caInfo;
        sslOptions = cpr::Ssl(cpr::ssl::CaInfo{std::move(copyOfCaInfo)});
    }
    auto response = cpr::Get(cpr::Url{m_url + "/client/features"},
                             cpr::Header{{"UNLEASH-INSTANCEID", m_instanceId},
                                         {"UNLEASH-APPNAME", m_name},
                                         {"Authorization", m_authentication}},
                             sslOptions);
    if (response.status_code == 0) {
        std::cerr << response.error.message << std::endl;
        return std::string{};
    } else if (response.status_code >= 400) {
        std::cerr << "Error [" << response.status_code << "] making request" << std::endl;
        return std::string{};
    }
    return response.text;
}

bool CprClient::registration(unsigned int refreshInterval) {
    cpr::SslOptions sslOptions;


    if (!m_caInfo.empty()) {
        // since CaInfo takes an rvalue reference and we don't want to move the m_caInfo in
        // we create a copy of it and move it in
        std::string copyOfCaInfo = m_caInfo;
        sslOptions = cpr::Ssl(cpr::ssl::CaInfo{std::move(copyOfCaInfo)});
    }
    nlohmann::json payload;
    payload["appName"] = m_name;
    payload["interval"] = refreshInterval;
    payload["started"] = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    payload["strategies"] = {"default", "userWithId", "flexibleRollout", "remoteAddress", "applicationHostname"};

    if (auto response = cpr::Post(
                cpr::Url{m_url + "/client/register"}, cpr::Body{payload.dump()},
                cpr::Header{{"Authorization", m_authentication}, {"Content-Type", "application/json"}}, sslOptions);
        response.status_code == 0) {
        std::cerr << response.error.message << std::endl;
    } else if (response.status_code >= 400) {
        std::cerr << "Error [" << response.status_code << "] making request" << std::endl;
    } else if (response.status_code == 202) {
        std::cout << "Unleash client has been registered" << std::endl;
        return true;
    }

    return false;
}
}  // namespace unleash