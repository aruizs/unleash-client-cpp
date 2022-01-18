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
    bool isEnabled(const std::string &flag, Context &context);

private:
    UnleashClient(std::string name, std::string url);
    void periodicTask();
    featuresMap_t loadFeatures(const std::string &features);

    std::string m_name, m_url;
    std::string m_instanceId, m_environment;
    unsigned int m_refreshInterval = 15;
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
    UnleashClientBuilder(std::string appName, std::string url) : unleashClient(appName, url) {}

    operator UnleashClient() { return std::move(unleashClient); }

    UnleashClientBuilder &instanceId(std::string instanceId);
    UnleashClientBuilder &environment(std::string environment);
    UnleashClientBuilder &refreshInterval(unsigned int refreshInterval);
    UnleashClientBuilder &apiClient(std::shared_ptr<ApiClient> apiClient);

private:
    UnleashClient unleashClient;
};

}  // namespace unleash

#endif  //UNLEASH_H
