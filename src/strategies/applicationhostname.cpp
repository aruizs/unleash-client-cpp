#include "unleash/strategies/applicationhostname.h"
#include <nlohmann/json.hpp>
#ifdef WIN32
#include <Windows.h>
#include <tchar.h>
#else
#include <unistd.h>
#endif
#include <iostream>


namespace unleash {

void getHostname(char machineName[150]) {
    char Name[150];

#ifdef WIN32
    TCHAR infoBuf[150];
    DWORD bufCharCount = 150;
    memset(Name, 0, 150);
    if (GetComputerName(infoBuf, &bufCharCount)) {
        for (i = 0; i < 150; i++) {
            Name[i] = infoBuf[i];
        }
    } else {
        strcpy(Name, "Unknown_Host_Name");
    }
#else
    memset(Name, 0, 150);
    gethostname(Name, 150);
#endif
    strncpy(machineName, Name, 150);
}

ApplicationHostname::ApplicationHostname(std::string_view parameters, std::string_view constraints) : Strategy("applicationHostname", constraints) {
    auto applicationHostname_json = nlohmann::json::parse(parameters);
    const std::string delimiter = ",";
    std::stringstream sstream(applicationHostname_json["hostNames"].get<std::string>());
    std::string hostname;
    while (std::getline(sstream, hostname, ',')) {
        hostname.erase(remove(hostname.begin(), hostname.end(), ' '), hostname.end());
        m_applicationHostnames.push_back(hostname);
    }
}

bool ApplicationHostname::isEnabled(const Context &context) {
    char hostnameC[150];
    getHostname(hostnameC);
    std::string hostname{hostnameC};
    if (std::find(m_applicationHostnames.begin(), m_applicationHostnames.end(), hostname) != m_applicationHostnames.end())
        return true;
    return false;
}
}  // namespace unleash