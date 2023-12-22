#ifndef UNLEASH_H
#define UNLEASH_H

#include "unleash/api/apiclient.h"
#include "unleash/export.h"
#include "unleash/feature.h"
#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

namespace unleash {

class UnleashClientBuilder;
class Context;
struct variant_t;

class UNLEASH_EXPORT UnleashClient {
public:
    using featuresMap_t = std::map<std::string, Feature>;

    friend class UnleashClientBuilder;
    ~UnleashClient();
    UnleashClient(UnleashClient &&) = default;
    friend UNLEASH_EXPORT std::ostream &operator<<(std::ostream &os, const UnleashClient &obj);
    static UnleashClientBuilder create(std::string name, std::string url);
    void initializeClient();
    bool isEnabled(const std::string &flag);
    bool isEnabled(const std::string &flag, const Context &context);
    variant_t variant(const std::string &flag, const Context &context);


private:
    UnleashClient(std::string name, std::string url);
    void periodicTask();
    featuresMap_t loadFeatures(std::string_view features) const;

    std::string m_name;
    std::string m_url;
    std::string m_instanceId;
    std::string m_environment;
    std::string m_authentication;
    bool m_registration = false;
    unsigned int m_refreshInterval = 15000;
    std::thread m_thread;
    bool m_stopThread = false;
    bool m_isInitialized = false;
    featuresMap_t m_features;
    std::shared_ptr<ApiClient> m_apiClient;
    static constexpr unsigned int k_pollInterval = 500;
};

// Unleash Client Builder

class UNLEASH_EXPORT UnleashClientBuilder {
public:
    UnleashClientBuilder(std::string appName, std::string url) : unleashClient(std::move(appName), std::move(url)) {}

    operator UnleashClient() { return std::move(unleashClient); }

    UnleashClientBuilder &instanceId(std::string instanceId);
    UnleashClientBuilder &environment(std::string environment);
    UnleashClientBuilder &refreshInterval(unsigned int refreshInterval);
    UnleashClientBuilder &apiClient(std::shared_ptr<ApiClient> apiClient);
    UnleashClientBuilder &authentication(std::string authentication);
    UnleashClientBuilder &registration(bool registration);

private:
    UnleashClient unleashClient;
};

}  // namespace unleash

#endif  //UNLEASH_H
