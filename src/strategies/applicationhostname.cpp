#include "unleash/strategies/applicationhostname.h"
#include <nlohmann/json.hpp>
#ifdef WIN32
#include <Windows.h>
#include <tchar.h>
#else
#include <unistd.h>
#endif
#include <cstring>
#include <sstream>


namespace unleash {

constexpr size_t kMaxHostnameLength = 150;

std::string getHostname() {
#ifdef WIN32
    TCHAR infoBuf[kMaxHostnameLength];
    DWORD bufCharCount = kMaxHostnameLength;
    if (GetComputerName(infoBuf, &bufCharCount)) {
        return std::string(infoBuf, bufCharCount);
    }
    return "Unknown_Host_Name";
#else
    std::string name(kMaxHostnameLength, '\0');
    if (gethostname(name.data(), kMaxHostnameLength - 1) == 0) {
        name.resize(std::strlen(name.c_str()));
        return name;
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