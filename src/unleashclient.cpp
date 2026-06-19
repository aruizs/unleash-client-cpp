#include "unleash/unleashclient.h"
#include "unleash/api/cprclient.h"
#include "unleash/strategies/strategy.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
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

UnleashClientBuilder &UnleashClientBuilder::metrics(bool metrics) {
    unleashClient.m_metrics = metrics;
    return *this;
}

UnleashClientBuilder &UnleashClientBuilder::metricsInterval(unsigned int metricsInterval) {
    unleashClient.m_metricsInterval = metricsInterval;
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
        m_metricsBucketStart = std::chrono::system_clock::now();
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
      m_stopThread(other.m_stopThread.load()),
      m_isInitialized(other.m_isInitialized),
      m_features(std::move(other.m_features)),
      m_apiClient(std::move(other.m_apiClient)),
      m_metrics(other.m_metrics),
      m_metricsInterval(other.m_metricsInterval),
      m_metricsBucket(std::move(other.m_metricsBucket)),
      m_metricsBucketStart(other.m_metricsBucketStart) {}

void UnleashClient::periodicTask() {
    unsigned long globalTimer = 0;
    unsigned long metricsTimer = 0;
    while (!m_stopThread) {
        std::this_thread::sleep_for(std::chrono::milliseconds(k_pollInterval));
        globalTimer += k_pollInterval;
        metricsTimer += k_pollInterval;
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
        if (m_metrics && metricsTimer >= m_metricsInterval) {
            metricsTimer = 0;
            flushMetrics();
        }
    }
}

UnleashClient::~UnleashClient() {
    m_stopThread = true;
    if (m_thread.joinable()) m_thread.join();
}

bool UnleashClient::isEnabled(const std::string &flag) {
    Context context;
    return isEnabled(flag, context, false);
}

bool UnleashClient::isEnabled(const std::string &flag, bool defaultValue) {
    Context context;
    return isEnabled(flag, context, defaultValue);
}

bool UnleashClient::isEnabled(const std::string &flag, const Context &context) {
    return isEnabled(flag, context, false);
}

bool UnleashClient::dependenciesSatisfied(const Feature &feature, const Context &context) const {
    for (const auto &dependency : feature.getDependencies()) {
        auto parentIt = m_features.find(dependency.feature);
        if (parentIt == m_features.end()) {
            return false;
        }
        const Feature &parent = parentIt->second;
        // Unleash does not allow transitive (or cyclic) dependencies: a parent that is itself a
        // child cannot satisfy a dependency.
        if (parent.hasDependencies()) {
            return false;
        }
        if (parent.isEnabled(context) != dependency.enabled) {
            return false;
        }
        if (dependency.enabled && dependency.hasVariants && !dependency.variants.empty()) {
            auto parentVariant = parent.getVariant(context);
            if (std::find(dependency.variants.begin(), dependency.variants.end(), parentVariant.name) ==
                dependency.variants.end()) {
                return false;
            }
        }
    }
    return true;
}

bool UnleashClient::isEnabled(const std::string &flag, const Context &context, bool defaultValue) {
    // The default value is returned only for unknown flags; a flag that exists is always evaluated
    // on its own merits. This matches the fallback semantics of the official Unleash SDKs.
    bool enabled = defaultValue;
    if (m_isInitialized) {
        std::scoped_lock lock(m_featuresMutex);
        if (auto search = m_features.find(flag); search != m_features.end()) {
            enabled = dependenciesSatisfied(search->second, context) && search->second.isEnabled(context);
        }
    }
    countToggle(flag, enabled);
    return enabled;
}

variant_t UnleashClient::variant(const std::string &flag, const unleash::Context &context) {
    variant_t variant{"disabled", 0, false, false};
    if (m_isInitialized) {
        std::scoped_lock lock(m_featuresMutex);
        if (auto search = m_features.find(flag); search != m_features.end()) {
            if (dependenciesSatisfied(search->second, context)) {
                variant = search->second.getVariant(context);
            }
        }
    }
    countToggle(flag, variant.feature_enabled);
    countVariant(flag, variant.name);
    return variant;
}

void UnleashClient::countToggle(const std::string &flag, bool enabled) {
    if (!m_metrics) {
        return;
    }
    std::scoped_lock lock(m_metricsMutex);
    auto &count = m_metricsBucket[flag];
    enabled ? ++count.yes : ++count.no;
}

void UnleashClient::countVariant(const std::string &flag, const std::string &variantName) {
    if (!m_metrics) {
        return;
    }
    std::scoped_lock lock(m_metricsMutex);
    ++m_metricsBucket[flag].variants[variantName];
}

void UnleashClient::flushMetrics() {
    std::map<std::string, ToggleCount> bucket;
    std::chrono::system_clock::time_point start;
    {
        std::scoped_lock lock(m_metricsMutex);
        bucket.swap(m_metricsBucket);
        start = m_metricsBucketStart;
        m_metricsBucketStart = std::chrono::system_clock::now();
    }
    if (!bucket.empty()) {
        m_apiClient->metrics(buildMetricsPayload(bucket, start));
    }
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

namespace {
std::string toIso8601(std::chrono::system_clock::time_point timePoint) {
    auto time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif
    std::ostringstream stream;
    stream << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}
}  // namespace

std::string UnleashClient::buildMetricsPayload(const std::map<std::string, ToggleCount> &bucket,
                                               std::chrono::system_clock::time_point start) const {
    nlohmann::json payload;
    payload["appName"] = m_name;
    payload["instanceId"] = m_instanceId;
    payload["bucket"]["start"] = toIso8601(start);
    payload["bucket"]["stop"] = toIso8601(std::chrono::system_clock::now());
    auto &toggles = payload["bucket"]["toggles"] = nlohmann::json::object();
    for (const auto &[flag, count] : bucket) {
        nlohmann::json toggle;
        toggle["yes"] = count.yes;
        toggle["no"] = count.no;
        if (!count.variants.empty()) {
            for (const auto &[variantName, variantCount] : count.variants) {
                toggle["variants"][variantName] = variantCount;
            }
        }
        toggles[flag] = toggle;
    }
    return payload.dump();
}

UnleashClient::featuresMap_t UnleashClient::loadFeatures(std::string_view features) const {
    auto parsedJson = nlohmann::json::parse(features);
    featuresMap_t featuresMap;

    // Delta API format: features and segments arrive as a stream of events rather than sitting at
    // the top level. Replay the events in order to build the effective state, then hand it to the
    // rest of the loading logic so that it stays format-agnostic.
    nlohmann::json featuresJson = parsedJson;
    if (parsedJson.contains("events")) {
        std::map<std::string, nlohmann::json> featureByName;
        std::map<int, nlohmann::json> segmentById;
        for (const auto &event : parsedJson["events"]) {
            const std::string type = event.value("type", "");
            if (type == "hydration") {
                for (const auto &feature : event.value("features", nlohmann::json::array())) {
                    featureByName[feature["name"].get<std::string>()] = feature;
                }
                for (const auto &segment : event.value("segments", nlohmann::json::array())) {
                    segmentById[segment["id"].get<int>()] = segment;
                }
            } else if (type == "feature-updated") {
                featureByName[event["feature"]["name"].get<std::string>()] = event["feature"];
            } else if (type == "feature-removed") {
                featureByName.erase(event["featureName"].get<std::string>());
            } else if (type == "segment-updated") {
                segmentById[event["segment"]["id"].get<int>()] = event["segment"];
            }
        }
        featuresJson = nlohmann::json::object();
        featuresJson["features"] = nlohmann::json::array();
        for (auto &[name, feature] : featureByName) { featuresJson["features"].push_back(feature); }
        featuresJson["segments"] = nlohmann::json::array();
        for (auto &[id, segment] : segmentById) { featuresJson["segments"].push_back(segment); }
    }

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

    // Append the constraints of the referenced segments. Returns false if any segment is missing.
    auto appendSegmentConstraints = [&segmentsMap](const nlohmann::json &segmentIds, nlohmann::json &constraints) {
        for (const auto &segmentId : segmentIds) {
            auto segmentIt = segmentsMap.find(segmentId.get<int>());
            if (segmentIt == segmentsMap.end()) { return false; }
            for (const auto &constraint : segmentIt->second) { constraints.push_back(constraint); }
        }
        return true;
    };

    // Merge a strategy's own constraints with the constraints of the segments referenced by both the
    // strategy and its feature. A reference to a missing segment forces the strategy to stay disabled.
    auto resolveConstraints = [&](const nlohmann::json &strategyValue,
                                  const nlohmann::json &featureSegments) -> std::string {
        nlohmann::json constraints = nlohmann::json::array();
        if (strategyValue.contains("constraints")) {
            for (const auto &constraint : strategyValue["constraints"]) { constraints.push_back(constraint); }
        }
        bool segmentsResolved = true;
        if (strategyValue.contains("segments")) {
            segmentsResolved = appendSegmentConstraints(strategyValue["segments"], constraints);
        }
        if (segmentsResolved && !featureSegments.empty()) {
            segmentsResolved = appendSegmentConstraints(featureSegments, constraints);
        }
        if (!segmentsResolved) {
            return nlohmann::json::array({{{"contextName", ""}, {"operator", "MISSING_SEGMENT"}}}).dump();
        }
        return constraints.empty() ? std::string{} : constraints.dump();
    };

    for (const auto &[key, value] : featuresJson["features"].items()) {
        const nlohmann::json featureSegments =
                value.contains("segments") ? value["segments"] : nlohmann::json::array();
        // Load strategies
        std::vector<std::unique_ptr<Strategy>> strategies;
        for (const auto &[strategyKey, strategyValue] : value["strategies"].items()) {
            std::string strategyParameters;
            if (strategyValue.contains("parameters")) strategyParameters = strategyValue["parameters"].dump();
            std::string strategyConstraints = resolveConstraints(strategyValue, featureSegments);
            auto strategy = Strategy::createStrategy(strategyValue["name"].get<std::string>(), strategyParameters,
                                                     strategyConstraints);
            if (strategy && strategyValue.contains("variants")) {
                strategy->setVariants(strategyValue["variants"].dump());
            }
            strategies.push_back(std::move(strategy));
        }
        Feature newFeature(value["name"], std::move(strategies), value["enabled"]);
        // Load variants
        if (value.contains("variants")) {
            newFeature.setVariants(parseVariants(value["variants"]));
        }
        // Load dependencies on other features
        if (value.contains("dependencies")) {
            newFeature.setDependencies(value["dependencies"].dump());
        }
        featuresMap.try_emplace(value["name"], std::move(newFeature));
    }
    return featuresMap;
}
}  // namespace unleash
