#ifndef UNLEASH_H
#define UNLEASH_H

#include "unleash/api/apiclient.h"
#include "unleash/export.h"
#include "unleash/feature.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

namespace unleash {

class UnleashClientBuilder;
struct Context;
struct variant_t;

struct ToggleCount {
    unsigned long yes = 0;
    unsigned long no = 0;
    std::map<std::string, unsigned long> variants;
};

class UNLEASH_EXPORT UnleashClient {
public:
    using featuresMap_t = std::map<std::string, Feature>;

    friend class UnleashClientBuilder;
    ~UnleashClient();
    UnleashClient(UnleashClient &&other) noexcept;
    friend UNLEASH_EXPORT std::ostream &operator<<(std::ostream &os, const UnleashClient &obj) {
        return os << obj.m_name << std::endl
                  << "with url:" << obj.m_url << std::endl
                  << "instance id: " << obj.m_instanceId << " in environment: " << obj.m_environment;
    }
    static UnleashClientBuilder create(std::string name, std::string url);
    void initializeClient();
    bool isEnabled(const std::string &flag);
    bool isEnabled(const std::string &flag, bool defaultValue);
    bool isEnabled(const std::string &flag, const Context &context);
    bool isEnabled(const std::string &flag, const Context &context, bool defaultValue);
    variant_t variant(const std::string &flag, const Context &context);


private:
    UnleashClient(std::string name, std::string url);
    void periodicTask();
    bool dependenciesSatisfied(const Feature &feature, const Context &context) const;
    featuresMap_t loadFeatures(std::string_view features) const;
    bool loadFeaturesFromCache();
    bool saveFeaturestoCache(const std::string &features) const;
    void countToggle(const std::string &flag, bool enabled);
    void countVariant(const std::string &flag, const std::string &variantName);
    std::string buildMetricsPayload(const std::map<std::string, ToggleCount> &bucket,
                                    std::chrono::system_clock::time_point start) const;
    void flushMetrics();

    std::string m_name;
    std::string m_url;
    std::string m_instanceId;
    std::string m_environment;
    std::string m_authentication;
    bool m_registration = false;
    std::string m_cacheFilePath;
    unsigned int m_refreshInterval = 15000;
    std::thread m_thread;
    std::atomic<bool> m_stopThread{false};
    bool m_isInitialized = false;
    featuresMap_t m_features;
    mutable std::mutex m_featuresMutex;
    std::shared_ptr<ApiClient> m_apiClient;
    bool m_metrics = false;
    unsigned int m_metricsInterval = 60000;
    std::map<std::string, ToggleCount> m_metricsBucket;
    std::chrono::system_clock::time_point m_metricsBucketStart;
    mutable std::mutex m_metricsMutex;
    static constexpr unsigned int k_pollInterval = 500;
};

// Unleash Client Builder

class UNLEASH_EXPORT UnleashClientBuilder {
public:
    UnleashClientBuilder(std::string appName, std::string url) : unleashClient(std::move(appName), std::move(url)) {}

    // Implicit conversion is kept for backward compatibility with 1.3.0; prefer build().
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    operator UnleashClient() { return std::move(unleashClient); }

    UnleashClient build() { return std::move(unleashClient); }

    UnleashClientBuilder &instanceId(std::string instanceId);
    UnleashClientBuilder &environment(std::string environment);
    UnleashClientBuilder &refreshInterval(unsigned int refreshInterval);
    UnleashClientBuilder &apiClient(std::shared_ptr<ApiClient> apiClient);
    UnleashClientBuilder &authentication(std::string authentication);
    UnleashClientBuilder &registration(bool registration);
    UnleashClientBuilder &cacheFilePath(std::string cacheFilePath);
    UnleashClientBuilder &metrics(bool metrics);
    UnleashClientBuilder &metricsInterval(unsigned int metricsInterval);

private:
    UnleashClient unleashClient;
};

}  // namespace unleash

#endif  //UNLEASH_H
