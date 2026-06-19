#include "unleash/strategies/applicationhostname.h"
#include <nlohmann/json.hpp>
#ifdef WIN32
#include <Windows.h>
#include <tchar.h>
#else
#include <unistd.h>
#endif
#include <sstream>


namespace unleash {

std::string getHostname() {
#ifdef WIN32
    TCHAR infoBuf[150];
    DWORD bufCharCount = 150;
    if (GetComputerName(infoBuf, &bufCharCount)) {
        return std::string(infoBuf, bufCharCount);
    }
    return "Unknown_Host_Name";
#else
    char name[150] = {0};
    if (gethostname(name, sizeof(name) - 1) == 0) {
        return std::string(name);
    }
    return "Unknown_Host_Name";
#endif
}

ApplicationHostname::ApplicationHostname(std::string_view parameters, std::string_view constraints)
    : Strategy("applicationHostname", constraints) {
    auto applicationHostname_json = nlohmann::json::parse(parameters);
    std::stringstream sstream(applicationHostname_json["hostNames"].get<std::string>());
    std::string hostname;
    while (std::getline(sstream, hostname, ',')) {
        hostname.erase(remove(hostname.begin(), hostname.end(), ' '), hostname.end());
        m_applicationHostnames.push_back(hostname);
    }
}

bool ApplicationHostname::isEnabled(const Context &context) {
    std::string hostname = getHostname();
    if (std::find(m_applicationHostnames.begin(), m_applicationHostnames.end(), hostname) == m_applicationHostnames.end())
        return false;
    return meetConstraints(context);
}
}  // namespace unleash