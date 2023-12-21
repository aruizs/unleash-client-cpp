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
              << "instance id: " << obj.m_instanceId
              << " in environment: " << obj.m_environment;
}

UnleashClientBuilder &UnleashClientBuilder::instanceId(std::string instanceId) {
    unleashClient.m_instanceId = std::move(instanceId);
    return *this;
}

UnleashClientBuilder &
UnleashClientBuilder::environment(std::string environment) {
    unleashClient.m_environment = std::move(environment);
    return *this;
}

UnleashClientBuilder &
UnleashClientBuilder::refreshInterval(unsigned int refreshInterval) {
    unleashClient.m_refreshInterval = refreshInterval;
    return *this;
}

UnleashClientBuilder &
UnleashClientBuilder::apiClient(std::shared_ptr<ApiClient> apiClient) {
    unleashClient.m_apiClient = apiClient;
    return *this;
}

UnleashClientBuilder &
UnleashClientBuilder::authentication(std::string authentication) {
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
            m_apiClient = std::make_unique<CprClient>(
                    m_url, m_name, m_instanceId, m_authentication);
        }

        // Register the Client
        if (m_registration && !m_apiClient->registration(m_refreshInterval)) {
            std::cerr << "Unable to register an Unleash Client instance."
                      << std::endl;
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

UnleashClient::UnleashClient(std::string name, std::string url)
    : m_name(std::move(name)), m_url(std::move(url)) {}

void UnleashClient::periodicTask() {
    unsigned long globalTimer = 0;
    while (!m_stopThread) {
        std::this_thread::sleep_for(std::chrono::milliseconds(k_pollInterval));
        globalTimer += k_pollInterval;
        if (globalTimer >= m_refreshInterval) {
            globalTimer = 0;
            auto features_response = m_apiClient->features();
            if (!features_response.empty())
                m_features = loadFeatures(features_response);
        }
    }
}

UnleashClient::~UnleashClient() {
    m_stopThread = true;
    if (m_thread.joinable()) m_thread.join();
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

UnleashClient::featuresMap_t
UnleashClient::loadFeatures(std::string_view features) const {
    const auto featuresJson = nlohmann::json::parse(features);
    featuresMap_t featuresMap;
    for (const auto &[key, value] : featuresJson["features"].items()) {
        std::vector<std::unique_ptr<Strategy>> m_strategies;
        for (const auto &[strategyKey, strategyValue] :
             value["strategies"].items()) {
            std::string strategyParameters;
            if (strategyValue.contains("parameters"))
                strategyParameters = strategyValue["parameters"].dump();
            std::string strategyConstraints;
            if (strategyValue.contains("constraints")) {
                strategyConstraints = strategyValue["constraints"].dump();
            }
            m_strategies.push_back(Strategy::createStrategy(
                    strategyValue["name"].get<std::string>(),
                    strategyParameters, strategyConstraints));
        }
        featuresMap.try_emplace(value["name"], value["name"],
                                std::move(m_strategies), value["enabled"]);
    }
    return featuresMap;
}
}  // namespace unleash
