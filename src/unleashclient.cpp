#include "unleash/unleashclient.h"
#include "unleash/api/cprclient.h"
#include "unleash/strategies/strategy.h"
#include <nlohmann/json.hpp>

namespace unleash {

UnleashClientBuilder UnleashClient::create(std::string name, std::string url) {
    return UnleashClientBuilder{std::move(name), std::move(url)};
}

std::ostream &operator<<(std::ostream &os, const UnleashClient &obj) {
    return os << obj.m_name << std::endl
              << "with url:" << obj.m_url << std::endl
              << "instance id: " << obj.m_instanceId << " in environment: " << obj.m_environment;
}

UnleashClientBuilder &UnleashClientBuilder::instanceId(std::string instanceId) {
    unleashClient.m_instanceId = std::move(instanceId);
    return *this;
}

UnleashClientBuilder &UnleashClientBuilder::environment(std::string environment) {
    unleashClient.m_environment = std::move(environment);
    return *this;
}

UnleashClientBuilder &UnleashClientBuilder::refreshInterval(unsigned int refreshInterval) {
    unleashClient.m_refreshInterval = refreshInterval;
    return *this;
}

UnleashClientBuilder &UnleashClientBuilder::apiClient(std::shared_ptr<ApiClient> apiClient) {
    unleashClient.m_apiClient = apiClient;
    return *this;
}

UnleashClientBuilder &UnleashClientBuilder::authentication(std::string authentication) {
    unleashClient.m_authentication = std::move(authentication);
    return *this;
}

UnleashClientBuilder &UnleashClientBuilder::registration(bool registration) {
    unleashClient.m_registration = registration;
    return *this;
}

void UnleashClient::initializeClient() {
    if (!m_isInitialized) {
        // Set-up Unleash API client
        if (m_apiClient == nullptr) {
            m_apiClient = std::make_unique<CprClient>(m_url, m_name, m_instanceId, m_authentication);
        }

        // Register the Client
        if (m_registration && !m_apiClient->registration(m_refreshInterval)) {
            std::cerr << "Unable to register an Unleash Client instance." << std::endl;
            return;
        }

        // Initial fetch of feature flags
        auto apiFeatures = m_apiClient->features();
        if (apiFeatures.empty()) {
            std::cerr << "Attempted to initialize an Unleash Client instance "
                         "without server response."
                      << std::endl;
            return;
        }
        m_features = loadFeatures(apiFeatures);
        m_thread = std::thread(&UnleashClient::periodicTask, this);
        m_isInitialized = true;
    } else {
        std::cout << "Attempted to initialize an Unleash Client instance that "
                     "has already been initialized."
                  << std::endl;
    }
}

UnleashClient::UnleashClient(std::string name, std::string url) : m_name(std::move(name)), m_url(std::move(url)) {}

void UnleashClient::periodicTask() {
    unsigned long globalTimer = 0;
    while (!m_stopThread) {
        std::this_thread::sleep_for(std::chrono::milliseconds(k_pollInterval));
        globalTimer += k_pollInterval;
        if (globalTimer >= m_refreshInterval) {
            globalTimer = 0;
            auto features_response = m_apiClient->features();
            if (!features_response.empty()) m_features = loadFeatures(features_response);
        }
    }
}

UnleashClient::~UnleashClient() {
    m_stopThread = true;
    if (m_thread.joinable()) m_thread.join();
}

std::vector<std::string> UnleashClient::featureFlags() const {
    std::vector<std::string> featureFlags;
    // test
    if (m_isInitialized) {
        for (auto it = m_features.begin(); it != m_features.end(); it++) {
            featureFlags.push_back(it->first);
        }
    }
    return featureFlags
}

bool UnleashClient::isEnabled(const std::string &flag) {
    Context context;
    return isEnabled(flag, context);
}

bool UnleashClient::isEnabled(const std::string &flag, const Context &context) {
    if (m_isInitialized) {
        if (auto search = m_features.find(flag); search != m_features.end()) {
            return m_features.at(flag).isEnabled(context);
        }
    }
    return false;
}

variant_t UnleashClient::variant(const std::string &flag, const unleash::Context &context) {
    variant_t variant{"disabled", 0, false, false};
    if (m_isInitialized) {
        variant.featureEnabled = isEnabled(flag, context);
        if (auto search = m_features.find(flag); search != m_features.end()) {
            return m_features.at(flag).getVariant(context);
        }
    }
    return variant;
}

UnleashClient::featuresMap_t UnleashClient::loadFeatures(std::string_view features) const {
    const auto featuresJson = nlohmann::json::parse(features);
    featuresMap_t featuresMap;
    for (const auto &[key, value] : featuresJson["features"].items()) {
        // Load strategies
        std::vector<std::unique_ptr<Strategy>> strategies;
        for (const auto &[strategyKey, strategyValue] : value["strategies"].items()) {
            std::string strategyParameters;
            if (strategyValue.contains("parameters")) strategyParameters = strategyValue["parameters"].dump();
            std::string strategyConstraints;
            if (strategyValue.contains("constraints")) { strategyConstraints = strategyValue["constraints"].dump(); }
            strategies.push_back(Strategy::createStrategy(strategyValue["name"].get<std::string>(), strategyParameters,
                                                          strategyConstraints));
        }
        Feature newFeature(value["name"], std::move(strategies), value["enabled"]);
        // Load variants
        std::pair<std::vector<std::unique_ptr<Variant>>, unsigned int> variants;
        if (value.contains("variants")) {
            unsigned int totalWeight = 0;
            for (const auto &[variantKey, variantValue] : value["variants"].items()) {
                std::string variantPayload;
                if (variantValue.contains("payload")) variantPayload = variantValue["payload"].dump();
                std::string variantOverrides;
                if (variantValue.contains("overrides")) variantOverrides = variantValue["overrides"].dump();
                variants.first.push_back(std::make_unique<Variant>(variantValue["name"], variantValue["weight"],
                                                                   variantPayload, variantOverrides));
                totalWeight += variantValue["weight"].get<unsigned int>();
            }
            variants.second = totalWeight;
            newFeature.setVariants(std::move(variants));
        }
        featuresMap.try_emplace(value["name"], std::move(newFeature));
    }
    return featuresMap;
}
}  // namespace unleash
