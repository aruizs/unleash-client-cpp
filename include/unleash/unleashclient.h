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

/**
 * @brief Evaluates Unleash feature flags for an application.
 *
 * An instance is created through UnleashClientBuilder (see create()), configured
 * with the builder's fluent setters, and finalised with UnleashClientBuilder::build().
 * After initializeClient() the client polls the Unleash server in the background and
 * answers isEnabled() / variant() queries from the cached configuration.
 *
 * Query methods are safe to call from multiple threads.
 */
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

    /**
     * @brief Starts building a client for the given application and server.
     * @param name Application name reported to the Unleash server.
     * @param url  Base URL of the Unleash API.
     * @return A builder used to configure and then build() the client.
     */
    static UnleashClientBuilder create(std::string name, std::string url);

    /**
     * @brief Fetches the initial feature configuration and starts the background refresh thread.
     *
     * Must be called once before any isEnabled() / variant() query. Calling it more than once
     * has no effect. If the initial fetch fails and a cache file is configured, the client loads
     * the last known configuration from it.
     */
    void initializeClient();

    /**
     * @brief Checks whether a feature flag is enabled, using an empty context.
     * @param flag Feature toggle name.
     * @return @c true if enabled; @c false if disabled or unknown.
     */
    bool isEnabled(const std::string &flag);

    /**
     * @brief Checks whether a feature flag is enabled, with a fallback for unknown flags.
     * @param flag Feature toggle name.
     * @param defaultValue Returned only when the flag is unknown to the server; a known flag is
     *        always evaluated on its own merits.
     * @return The flag's evaluation, or @p defaultValue when the flag is unknown.
     */
    bool isEnabled(const std::string &flag, bool defaultValue);

    /**
     * @brief Checks whether a feature flag is enabled for the given context.
     * @param flag Feature toggle name.
     * @param context Evaluation context (user, session, properties, ...).
     * @return @c true if enabled; @c false if disabled or unknown.
     */
    bool isEnabled(const std::string &flag, const Context &context);

    /**
     * @brief Checks whether a feature flag is enabled for the given context, with a fallback.
     * @param flag Feature toggle name.
     * @param context Evaluation context.
     * @param defaultValue Returned only when the flag is unknown to the server.
     * @return The flag's evaluation, or @p defaultValue when the flag is unknown.
     */
    bool isEnabled(const std::string &flag, const Context &context, bool defaultValue);

    /**
     * @brief Resolves the variant served for a feature flag in the given context.
     * @param flag Feature toggle name.
     * @param context Evaluation context.
     * @return The selected variant, or the disabled variant when the flag is off or unknown.
     */
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

/**
 * @brief Fluent builder for UnleashClient.
 *
 * Obtain one from UnleashClient::create(), chain the optional setters, then call build():
 * @code
 * auto client = unleash::UnleashClient::create("my-app", "https://unleash.example.com/api")
 *                   .authentication("token")
 *                   .build();
 * client.initializeClient();
 * @endcode
 */
class UNLEASH_EXPORT UnleashClientBuilder {
public:
    UnleashClientBuilder(std::string appName, std::string url) : unleashClient(std::move(appName), std::move(url)) {}

    /// Implicit conversion kept for backward compatibility with 1.3.0; prefer build().
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    operator UnleashClient() { return std::move(unleashClient); }

    /// @brief Finalises configuration and returns the configured client.
    UnleashClient build() { return std::move(unleashClient); }

    /// @brief Sets the instance id reported to the server.
    UnleashClientBuilder &instanceId(std::string instanceId);
    /// @brief Sets the environment used during flag evaluation.
    UnleashClientBuilder &environment(std::string environment);
    /// @brief Sets the background refresh interval, in milliseconds (default 15000).
    UnleashClientBuilder &refreshInterval(unsigned int refreshInterval);
    /// @brief Overrides the API transport (mainly for testing).
    UnleashClientBuilder &apiClient(std::shared_ptr<ApiClient> apiClient);
    /// @brief Sets the authentication token sent to the server.
    UnleashClientBuilder &authentication(std::string authentication);
    /// @brief Enables client registration on initialization (default false).
    UnleashClientBuilder &registration(bool registration);
    /// @brief Sets a local cache file used for offline bootstrapping.
    UnleashClientBuilder &cacheFilePath(std::string cacheFilePath);
    /// @brief Enables usage-metrics reporting to the server (default false).
    UnleashClientBuilder &metrics(bool metrics);
    /// @brief Sets the metrics flush interval, in milliseconds (default 60000).
    UnleashClientBuilder &metricsInterval(unsigned int metricsInterval);

private:
    UnleashClient unleashClient;
};

}  // namespace unleash

#endif  //UNLEASH_H
