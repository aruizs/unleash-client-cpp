#include "unleash/unleashclient.h"
#include "unleash/api/cprclient.h"
#include "unleash/strategies/strategy.h"
#include <nlohmann/json.hpp>

namespace unleash {

UnleashClientBuilder UnleashClient::create(std::string name, std::string url) { return UnleashClientBuilder{name, url}; }

std::ostream &operator<<(std::ostream &os, const UnleashClient &obj) {
    return os << obj.m_name
              << std::endl
              << "with url:" << obj.m_url
              << std::endl
              << "instance id: " << obj.m_instanceId
              << " in environment: " << obj.m_environment;
}

UnleashClientBuilder &UnleashClientBuilder::instanceId(std::string instanceId) {
    unleashClient.m_instanceId = instanceId;
    return *this;
}

UnleashClientBuilder &UnleashClientBuilder::environment(std::string environment) {
    unleashClient.m_environment = environment;
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

void UnleashClient::initializeClient() {
    if (!m_isInitialized) {
        if (m_apiClient == nullptr) {
            m_apiClient = std::make_unique<CprClient>(m_url, m_name, m_instanceId);
        }
        m_features = loadFeatures(m_apiClient->features());
        m_thread = std::thread(&UnleashClient::periodicTask, this);
        m_isInitialized = true;
    } else {
        std::cout << "Attempted to initialize an Unleash Client instance that has already been initialized." << std::endl;
    }
}

UnleashClient::UnleashClient(std::string name, std::string url) : m_name(name), m_url(url), m_thread() {}

void UnleashClient::periodicTask() {
    unsigned long globalTimer = 0;
    while (!m_stopThread) {
        std::this_thread::sleep_for(std::chrono::milliseconds(k_pollInterval));
        globalTimer += k_pollInterval;
        if (globalTimer >= m_refreshInterval) {
            globalTimer = 0;
            auto features_response = m_apiClient->features();
            auto featuresMap = loadFeatures(features_response);
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

bool UnleashClient::isEnabled(const std::string &flag, Context &context) {
    if (auto search = m_features.find(flag); search != m_features.end()) {
        return m_features.at(flag).isEnabled(context);
    }
    return false;
}

UnleashClient::featuresMap_t UnleashClient::loadFeatures(const std::string &features) {
    const auto featuresJson = nlohmann::json::parse(features);
    featuresMap_t featuresMap;
    for (const auto &[key, value] : featuresJson["features"].items()) {
        std::vector<std::unique_ptr<Strategy>> m_strategies;
        for (const auto &[strategyKey, strategyValue] : value["strategies"].items()) {
            m_strategies.push_back(std::move(Strategy::createStrategy(strategyValue["name"], strategyValue.value("parameters", ""))));
        }
        featuresMap.insert(std::pair(value["name"], Feature(value["name"], std::move(m_strategies), value["enabled"])));
    }
    return featuresMap;
}
}  // namespace unleash
