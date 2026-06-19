#include "unleash/unleashclient.h"
#include "unleash/api/cprclient.h"
#include "unleash/strategies/strategy.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace unleash {

UnleashClientBuilder UnleashClient::create(std::string name, std::string url) {
    return UnleashClientBuilder{std::move(name), std::move(url)};
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

UnleashClientBuilder &UnleashClientBuilder::cacheFilePath(std::string cacheFilePath) {
    unleashClient.m_cacheFilePath = std::move(cacheFilePath);
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
        if (auto apiFeatures = m_apiClient->features(); apiFeatures.empty()) {
            std::cerr << "Attempted to initialize an Unleash Client instance without server response." << std::endl;
            if (!loadFeaturesFromCache()) {
                return;
            }
        } else {
            std::scoped_lock lock(m_featuresMutex);
            m_features = loadFeatures(apiFeatures);
        }
        m_thread = std::thread(&UnleashClient::periodicTask, this);
        m_isInitialized = true;
    } else {
        std::cout << "Attempted to initialize an Unleash Client instance that "
                     "has already been initialized."
                  << std::endl;
    }
}

UnleashClient::UnleashClient(std::string name, std::string url) : m_name(std::move(name)), m_url(std::move(url)) {}

UnleashClient::UnleashClient(UnleashClient &&other) noexcept
    : m_name(std::move(other.m_name)),
      m_url(std::move(other.m_url)),
      m_instanceId(std::move(other.m_instanceId)),
      m_environment(std::move(other.m_environment)),
      m_authentication(std::move(other.m_authentication)),
      m_registration(other.m_registration),
      m_cacheFilePath(std::move(other.m_cacheFilePath)),
      m_refreshInterval(other.m_refreshInterval),
      m_thread(std::move(other.m_thread)),
      m_stopThread(other.m_stopThread),
      m_isInitialized(other.m_isInitialized),
      m_features(std::move(other.m_features)),
      m_apiClient(std::move(other.m_apiClient)) {}

void UnleashClient::periodicTask() {
    unsigned long globalTimer = 0;
    while (!m_stopThread) {
        std::this_thread::sleep_for(std::chrono::milliseconds(k_pollInterval));
        globalTimer += k_pollInterval;
        if (globalTimer >= m_refreshInterval) {
            globalTimer = 0;
            auto features_response = m_apiClient->features();
            if (!features_response.empty()) {
                saveFeaturestoCache(features_response);
                std::scoped_lock lock(m_featuresMutex);
                m_features = loadFeatures(features_response);
            } else {
                loadFeaturesFromCache();
            }
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
        std::scoped_lock lock(m_featuresMutex);
        if (auto search = m_features.find(flag); search != m_features.end()) {
            return m_features.at(flag).isEnabled(context);
        }
    }
    return false;
}

variant_t UnleashClient::variant(const std::string &flag, const unleash::Context &context) {
    variant_t variant{"disabled", 0, false, false};
    if (m_isInitialized) {
        std::scoped_lock lock(m_featuresMutex);
        if (auto search = m_features.find(flag); search != m_features.end()) {
            variant.featureEnabled = m_features.at(flag).isEnabled(context);
            return m_features.at(flag).getVariant(context);
        }
    }
    return variant;
}

bool UnleashClient::loadFeaturesFromCache() {
    if (m_cacheFilePath.empty()) {
        return false;
    }
    std::ifstream cacheFile(m_cacheFilePath);
    if (!cacheFile.is_open()) {
        std::cerr << "Could not open cache file '" << m_cacheFilePath << "' for reading." << std::endl;
        return false;
    }
    try {
        std::stringstream features_buffer;
        features_buffer << cacheFile.rdbuf();
        std::scoped_lock lock(m_featuresMutex);
        m_features = loadFeatures(features_buffer.str());
        std::cout << "Loaded configuration from cache file " << m_cacheFilePath << std::endl;
        return true;
    } catch (const nlohmann::json::exception &e) {
        std::cerr << "Failed to parse cache file: " << e.what() << std::endl;
        return false;
    }
}

bool UnleashClient::saveFeaturestoCache(const std::string &features) const {
    if (m_cacheFilePath.empty()) {
        return false;
    }
    std::ofstream cacheFile(m_cacheFilePath);
    if (!cacheFile.is_open()) {
        std::cerr << "Could not open cache file '" << m_cacheFilePath << "' for writing." << std::endl;
        return false;
    }
    cacheFile << features;
    return true;
}

UnleashClient::featuresMap_t UnleashClient::loadFeatures(std::string_view features) const {
    const auto featuresJson = nlohmann::json::parse(features);
    featuresMap_t featuresMap;

    auto parseVariants = [](const nlohmann::json &variantsJson) {
        std::pair<std::vector<std::unique_ptr<Variant>>, unsigned int> variants;
        unsigned int totalWeight = 0;
        for (const auto &[variantKey, variantValue] : variantsJson.items()) {
            std::string variantPayload = variantValue.contains("payload") ? variantValue["payload"].dump() : "";
            std::string variantOverrides = variantValue.contains("overrides") ? variantValue["overrides"].dump() : "";
            variants.first.push_back(std::make_unique<Variant>(variantValue["name"], variantValue["weight"],
                                                               variantPayload, variantOverrides));
            totalWeight += variantValue["weight"].get<unsigned int>();
        }
        variants.second = totalWeight;
        return variants;
    };

    // Load global segments: id -> list of constraints
    std::map<int, nlohmann::json> segmentsMap;
    if (featuresJson.contains("segments")) {
        for (const auto &segment : featuresJson["segments"]) {
            segmentsMap[segment["id"].get<int>()] =
                    segment.contains("constraints") ? segment["constraints"] : nlohmann::json::array();
        }
    }

    // Merge a strategy's own constraints with the constraints of its referenced segments. A
    // reference to a missing segment forces the strategy to never be enabled.
    auto resolveConstraints = [&segmentsMap](const nlohmann::json &strategyValue) -> std::string {
        nlohmann::json constraints = nlohmann::json::array();
        if (strategyValue.contains("constraints")) {
            for (const auto &constraint : strategyValue["constraints"]) { constraints.push_back(constraint); }
        }
        if (strategyValue.contains("segments")) {
            for (const auto &segmentId : strategyValue["segments"]) {
                auto segmentIt = segmentsMap.find(segmentId.get<int>());
                if (segmentIt == segmentsMap.end()) {
                    // Missing segment: inject an unsatisfiable constraint so the strategy stays disabled.
                    return nlohmann::json::array({{{"contextName", ""}, {"operator", "MISSING_SEGMENT"}}}).dump();
                }
                for (const auto &constraint : segmentIt->second) { constraints.push_back(constraint); }
            }
        }
        return constraints.empty() ? std::string{} : constraints.dump();
    };

    for (const auto &[key, value] : featuresJson["features"].items()) {
        // Load strategies
        std::vector<std::unique_ptr<Strategy>> strategies;
        for (const auto &[strategyKey, strategyValue] : value["strategies"].items()) {
            std::string strategyParameters;
            if (strategyValue.contains("parameters")) strategyParameters = strategyValue["parameters"].dump();
            std::string strategyConstraints = resolveConstraints(strategyValue);
            strategies.push_back(Strategy::createStrategy(strategyValue["name"].get<std::string>(), strategyParameters,
                                                          strategyConstraints));
        }
        Feature newFeature(value["name"], std::move(strategies), value["enabled"]);
        // Load variants
        if (value.contains("variants")) {
            newFeature.setVariants(parseVariants(value["variants"]));
        }
        featuresMap.try_emplace(value["name"], std::move(newFeature));
    }
    return featuresMap;
}
}  // namespace unleash
