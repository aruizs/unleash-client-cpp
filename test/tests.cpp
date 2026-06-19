#include "unleash/api/apiclient.h"
#include "unleash/context.h"
#include "unleash/strategies/strategy.h"
#include "unleash/unleashclient.h"
#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <thread>

using ::testing::Return;


class ApiClientMock : public unleash::ApiClient {
public:
    MOCK_METHOD(std::string, features, (), (override));
    MOCK_METHOD(bool, registration, (unsigned int), (override));
    MOCK_METHOD(bool, metrics, (const std::string &), (override));
};

// A custom strategy enabled when the numeric userId is at least the "min" parameter. Exercises
// parameter parsing and constraint inheritance from the Strategy base class.
class MinUserIdStrategy : public unleash::Strategy {
public:
    MinUserIdStrategy(std::string_view parameters, std::string_view constraints)
        : unleash::Strategy("minUserId", constraints) {
        if (!parameters.empty()) {
            auto json = nlohmann::json::parse(parameters);
            if (json.contains("min")) m_min = std::stoi(json["min"].get<std::string>());
        }
    }
    bool isEnabled(const unleash::Context &context) override {
        if (!meetConstraints(context)) return false;
        return !context.userId.empty() && std::stoi(context.userId) >= m_min;
    }

private:
    int m_min = 0;
};

using TestParam = std::tuple<std::string, std::string, bool>;

std::vector<TestParam> readSpecificationTestFromDisk(const std::string &testPath) {
    std::vector<TestParam> values;
    if (testPath.empty()) {
        std::cout << "No test path found. Current path: " << std::filesystem::current_path() << std::endl;
        return values;
    }
    // Read index file
    auto testIndexPath = testPath + "index.json";
    std::ifstream i(testIndexPath);
    nlohmann::json j;
    i >> j;

    // range-based to read each test
    for (auto &element : j) {
        std::cout << testPath + element.get<std::string>() << std::endl;
        std::ifstream testFile(testPath + element.get<std::string>());
        nlohmann::json testJson;
        testFile >> testJson;
        if (testJson.contains("tests"))
            values.push_back(std::make_tuple(testJson["state"].dump(), testJson["tests"].dump(), false));
        else if (testJson.contains("variantTests"))
            values.push_back(std::make_tuple(testJson["state"].dump(), testJson["variantTests"].dump(), true));
    }
    return values;
}

std::string getTestPath() {
    auto currentPath = std::filesystem::current_path();
    std::string currentPathStr{currentPath.u8string()};
    auto testPath = currentPathStr + "/specification/";
    return testPath;
}

class UnleashSpecificationTest : public testing::TestWithParam<TestParam> {};

TEST(UnleashTest, InicializationBadServerUrl) {
    auto unleashClient = static_cast<unleash::UnleashClient>(unleash::UnleashClient::create("production", "urlMock"));
    std::cout << unleashClient << std::endl;
    unleashClient.initializeClient();
    EXPECT_FALSE(unleashClient.isEnabled("feature.toogle"));
}

TEST(UnleashTest, InicializationErrorServerResponse) {
    auto unleashClient = static_cast<unleash::UnleashClient>(unleash::UnleashClient::create("production", "https://www.apple.com/%"));
    unleashClient.initializeClient();
    EXPECT_FALSE(unleashClient.isEnabled("feature.toogle"));
}

TEST(UnleashTest, RegistrationBadServerUrl) {
    auto unleashClient = static_cast<unleash::UnleashClient>(unleash::UnleashClient::create("production", "urlMock").registration(true));
    std::cout << unleashClient << std::endl;
    unleashClient.initializeClient();
    EXPECT_FALSE(unleashClient.isEnabled("feature.toogle"));
}

TEST(UnleashTest, RegistrationErrorServerResponse) {
    auto unleashClient = static_cast<unleash::UnleashClient>(
            unleash::UnleashClient::create("production", "https://www.apple.com/%").registration(true));
    unleashClient.initializeClient();
    EXPECT_FALSE(unleashClient.isEnabled("feature.toogle"));
}

TEST(UnleashTest, ApplicationHostname) {
    const std::string parameters = "{\"hostNames\": \"testHostname\"}";
    auto appHost = unleash::Strategy::createStrategy("applicationHostname", parameters);
    unleash::Context context;
    EXPECT_FALSE(appHost->isEnabled(context));
}

TEST_P(UnleashSpecificationTest, TestSet) {
    auto testData = GetParam();
    auto apiMock = std::make_shared<ApiClientMock>();
    constexpr unsigned int refreshInterval = 500;
    auto unleashClient = static_cast<unleash::UnleashClient>(unleash::UnleashClient::create("production", "urlMock")
                                                   .instanceId("intanceId")
                                                   .environment("production")
                                                   .apiClient(apiMock)
                                                   .refreshInterval(refreshInterval)
                                                   .authentication("clientToken")
                                                   .registration(true));
    EXPECT_CALL(*apiMock, features()).WillRepeatedly(Return(std::get<0>(testData)));
    EXPECT_CALL(*apiMock, registration(refreshInterval)).WillRepeatedly(Return(true));
    unleashClient.initializeClient();
    nlohmann::json testSet = nlohmann::json::parse(std::get<1>(testData));
    unleashClient.initializeClient();  // Retry initialization to check nothing happens
    for (const auto &[key, value] : testSet.items()) {
        auto contextJson = value["context"];
        unleash::Context testContext{contextJson.value("userId", ""), contextJson.value("sessionId", ""),
                                     contextJson.value("remoteAddress", ""), contextJson.value("environment", ""),
                                     contextJson.value("appName", "")};
        testContext.currentTime = contextJson.value("currentTime", "");
        if (contextJson.contains("properties")) {
            for (auto &[propertyKey, propertyValue] : contextJson["properties"].items()) {
                if (!propertyValue.is_null()) {
                    testContext.properties.try_emplace(propertyKey, propertyValue.get<std::string>());
                }
            }
        }
        if (!std::get<2>(testData)) {
            EXPECT_EQ(unleashClient.isEnabled(value["toggleName"], testContext), value["expectedResult"].get<bool>());
        } else {
            std::cout << value["toggleName"] << std::endl;
            nlohmann::json expectedResult = value["expectedResult"];
            auto variant = unleashClient.variant(value["toggleName"], testContext);
            EXPECT_EQ(expectedResult["feature_enabled"], variant.feature_enabled);
            EXPECT_EQ(expectedResult["enabled"], variant.enabled);
            EXPECT_EQ(expectedResult["name"], variant.name);
            if (expectedResult.contains("payload")) EXPECT_EQ(expectedResult["payload"].dump(), variant.payload);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(AllSpecificationFiles, UnleashSpecificationTest,
                         testing::ValuesIn(readSpecificationTestFromDisk(getTestPath())));

// Bootstrap/Cache tests
class BootstrapTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_cacheFilePath = std::filesystem::temp_directory_path() / "unleash_test_cache.json";
        // Clean up any existing cache file
        std::filesystem::remove(m_cacheFilePath);
    }

    void TearDown() override {
        // Clean up cache file after test
        std::filesystem::remove(m_cacheFilePath);
    }

    std::filesystem::path m_cacheFilePath;
    const std::string m_validFeatures = R"({"features":[{"name":"test.feature","enabled":true,"strategies":[{"name":"default"}]}]})";
};

TEST_F(BootstrapTest, InitializesFromCacheWhenApiFails) {
    // Write a valid cache file
    std::ofstream cacheFile(m_cacheFilePath);
    cacheFile << m_validFeatures;
    cacheFile.close();

    auto apiMock = std::make_shared<ApiClientMock>();
    EXPECT_CALL(*apiMock, features()).WillRepeatedly(Return(""));  // API returns empty

    auto unleashClient = static_cast<unleash::UnleashClient>(unleash::UnleashClient::create("production", "urlMock")
                                                   .apiClient(apiMock)
                                                   .cacheFilePath(m_cacheFilePath.string()));
    unleashClient.initializeClient();

    EXPECT_TRUE(unleashClient.isEnabled("test.feature"));
}

TEST_F(BootstrapTest, FailsInitializationWhenApiFailsAndNoCacheExists) {
    auto apiMock = std::make_shared<ApiClientMock>();
    EXPECT_CALL(*apiMock, features()).WillRepeatedly(Return(""));

    auto unleashClient = static_cast<unleash::UnleashClient>(unleash::UnleashClient::create("production", "urlMock")
                                                   .apiClient(apiMock)
                                                   .cacheFilePath(m_cacheFilePath.string()));
    unleashClient.initializeClient();

    EXPECT_FALSE(unleashClient.isEnabled("test.feature"));
}

TEST_F(BootstrapTest, FailsInitializationWhenApiFailsAndNoCachePathSet) {
    auto apiMock = std::make_shared<ApiClientMock>();
    EXPECT_CALL(*apiMock, features()).WillRepeatedly(Return(""));

    auto unleashClient = static_cast<unleash::UnleashClient>(unleash::UnleashClient::create("production", "urlMock")
                                                   .apiClient(apiMock));
    unleashClient.initializeClient();

    EXPECT_FALSE(unleashClient.isEnabled("test.feature"));
}

TEST_F(BootstrapTest, HandlesInvalidJsonInCacheFile) {
    // Write invalid JSON to cache
    std::ofstream cacheFile(m_cacheFilePath);
    cacheFile << "{ invalid json }";
    cacheFile.close();

    auto apiMock = std::make_shared<ApiClientMock>();
    EXPECT_CALL(*apiMock, features()).WillRepeatedly(Return(""));

    auto unleashClient = static_cast<unleash::UnleashClient>(unleash::UnleashClient::create("production", "urlMock")
                                                   .apiClient(apiMock)
                                                   .cacheFilePath(m_cacheFilePath.string()));
    unleashClient.initializeClient();

    // Should not crash, and feature should be disabled
    EXPECT_FALSE(unleashClient.isEnabled("test.feature"));
}

TEST_F(BootstrapTest, WritesCacheOnSuccessfulApiResponse) {
    auto apiMock = std::make_shared<ApiClientMock>();
    EXPECT_CALL(*apiMock, features()).WillRepeatedly(Return(m_validFeatures));

    auto unleashClient = static_cast<unleash::UnleashClient>(unleash::UnleashClient::create("production", "urlMock")
                                                   .apiClient(apiMock)
                                                   .refreshInterval(100)
                                                   .cacheFilePath(m_cacheFilePath.string()));
    unleashClient.initializeClient();

    // Wait for periodic task to write cache (poll interval is 500ms + refresh interval 100ms + buffer)
    std::this_thread::sleep_for(std::chrono::milliseconds(700));

    // Verify cache file was created
    EXPECT_TRUE(std::filesystem::exists(m_cacheFilePath));

    // Verify cache contains valid JSON
    std::ifstream cacheFile(m_cacheFilePath);
    std::stringstream buffer;
    buffer << cacheFile.rdbuf();
    EXPECT_NO_THROW(nlohmann::json::parse(buffer.str()));
}

TEST(CustomStrategyTest, RegisteredStrategyIsUsedWithParameters) {
    auto apiMock = std::make_shared<ApiClientMock>();
    const std::string state =
            R"({"version":1,"features":[{"name":"custom.flag","enabled":true,)"
            R"("strategies":[{"name":"minUserId","parameters":{"min":"10"}}]}]})";
    EXPECT_CALL(*apiMock, features()).WillRepeatedly(Return(state));

    auto unleashClient = unleash::UnleashClient::create("appName", "urlMock")
                                 .apiClient(apiMock)
                                 .registerStrategy("minUserId",
                                                   [](std::string_view parameters, std::string_view constraints) {
                                                       return std::make_unique<MinUserIdStrategy>(parameters,
                                                                                                  constraints);
                                                   })
                                 .build();
    unleashClient.initializeClient();

    unleash::Context above;
    above.userId = "15";
    unleash::Context below;
    below.userId = "5";
    EXPECT_TRUE(unleashClient.isEnabled("custom.flag", above));
    EXPECT_FALSE(unleashClient.isEnabled("custom.flag", below));
}

TEST(CustomStrategyTest, OverridesBuiltInStrategyOfTheSameName) {
    auto apiMock = std::make_shared<ApiClientMock>();
    // "default" would normally always be enabled; the custom registration takes precedence.
    const std::string state =
            R"({"version":1,"features":[{"name":"custom.flag","enabled":true,"strategies":[{"name":"default"}]}]})";
    EXPECT_CALL(*apiMock, features()).WillRepeatedly(Return(state));

    auto unleashClient = unleash::UnleashClient::create("appName", "urlMock")
                                 .apiClient(apiMock)
                                 .registerStrategy("default",
                                                   [](std::string_view parameters, std::string_view constraints) {
                                                       return std::make_unique<MinUserIdStrategy>(parameters,
                                                                                                  constraints);
                                                   })
                                 .build();
    unleashClient.initializeClient();

    unleash::Context context;
    context.userId = "7";
    // With min defaulting to 0, userId 7 satisfies the override.
    EXPECT_TRUE(unleashClient.isEnabled("custom.flag", context));
    // Empty userId fails the custom strategy, proving the built-in default was replaced.
    EXPECT_FALSE(unleashClient.isEnabled("custom.flag"));
}

TEST(MetricsTest, ReportsToggleAndVariantCounts) {
    auto apiMock = std::make_shared<ApiClientMock>();
    const std::string state =
            R"({"version":1,"features":[{"name":"metrics.flag","enabled":true,"strategies":[{"name":"default"}],)"
            R"("variants":[{"name":"v1","weight":100,"payload":{"type":"string","value":"a"}}]}]})";
    EXPECT_CALL(*apiMock, features()).WillRepeatedly(Return(state));
    EXPECT_CALL(*apiMock, registration(testing::_)).WillRepeatedly(Return(true));

    // The payload is captured on the periodic thread and inspected on the main thread, so guard it
    // with a mutex to establish a happens-before relationship (and keep ThreadSanitizer happy).
    std::mutex payloadMutex;
    std::string capturedPayload;
    EXPECT_CALL(*apiMock, metrics(testing::_))
            .WillRepeatedly(testing::Invoke([&](const std::string &payload) {
                std::scoped_lock lock(payloadMutex);
                capturedPayload = payload;
                return true;
            }));

    auto unleashClient = static_cast<unleash::UnleashClient>(unleash::UnleashClient::create("appName", "urlMock")
                                                                     .apiClient(apiMock)
                                                                     .metrics(true)
                                                                     .metricsInterval(500));
    unleashClient.initializeClient();

    EXPECT_TRUE(unleashClient.isEnabled("metrics.flag"));
    EXPECT_FALSE(unleashClient.isEnabled("unknown.flag"));
    unleashClient.variant("metrics.flag", unleash::Context{});

    // Wait for at least one metrics flush from the periodic thread.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::string payloadCopy;
    {
        std::scoped_lock lock(payloadMutex);
        payloadCopy = capturedPayload;
    }
    ASSERT_FALSE(payloadCopy.empty());
    auto payload = nlohmann::json::parse(payloadCopy);
    EXPECT_EQ(payload["appName"], "appName");
    const auto &toggles = payload["bucket"]["toggles"];
    ASSERT_TRUE(toggles.contains("metrics.flag"));
    // isEnabled + variant both register an enabled evaluation.
    EXPECT_EQ(toggles["metrics.flag"]["yes"], 2);
    EXPECT_EQ(toggles["metrics.flag"]["variants"]["v1"], 1);
    EXPECT_EQ(toggles["unknown.flag"]["no"], 1);
}

TEST(ApiErgonomicsTest, BuildReturnsUsableClient) {
    auto apiMock = std::make_shared<ApiClientMock>();
    const std::string state = R"({"version":1,"features":[{"name":"build.flag","enabled":true,"strategies":[{"name":"default"}]}]})";
    EXPECT_CALL(*apiMock, features()).WillRepeatedly(Return(state));

    auto unleashClient = unleash::UnleashClient::create("appName", "urlMock").apiClient(apiMock).build();
    unleashClient.initializeClient();

    EXPECT_TRUE(unleashClient.isEnabled("build.flag"));
}

TEST(ApiErgonomicsTest, DefaultValueUsedOnlyForUnknownFlags) {
    auto apiMock = std::make_shared<ApiClientMock>();
    const std::string state =
            R"({"version":1,"features":[{"name":"on.flag","enabled":true,"strategies":[{"name":"default"}]},)"
            R"({"name":"off.flag","enabled":false,"strategies":[{"name":"default"}]}]})";
    EXPECT_CALL(*apiMock, features()).WillRepeatedly(Return(state));

    auto unleashClient = unleash::UnleashClient::create("appName", "urlMock").apiClient(apiMock).build();
    unleashClient.initializeClient();

    // Unknown flag falls back to the supplied default.
    EXPECT_TRUE(unleashClient.isEnabled("missing.flag", true));
    EXPECT_FALSE(unleashClient.isEnabled("missing.flag", false));

    // Known flags always evaluate on their own merits, ignoring the default.
    EXPECT_TRUE(unleashClient.isEnabled("on.flag", false));
    EXPECT_FALSE(unleashClient.isEnabled("off.flag", true));

    // The context-aware overload behaves the same way.
    unleash::Context context{"user-1"};
    EXPECT_TRUE(unleashClient.isEnabled("missing.flag", context, true));
    EXPECT_TRUE(unleashClient.isEnabled("on.flag", context, false));
}

TEST(ApiErgonomicsTest, DefaultValueReturnedBeforeInitialization) {
    auto unleashClient = unleash::UnleashClient::create("appName", "urlMock").build();
    // Without initialization there are no features, so the default is always returned.
    EXPECT_TRUE(unleashClient.isEnabled("any.flag", true));
    EXPECT_FALSE(unleashClient.isEnabled("any.flag", false));
}

TEST(MetricsTest, DisabledByDefaultDoesNotReport) {
    auto apiMock = std::make_shared<ApiClientMock>();
    const std::string state = R"({"version":1,"features":[{"name":"metrics.flag","enabled":true,"strategies":[{"name":"default"}]}]})";
    EXPECT_CALL(*apiMock, features()).WillRepeatedly(Return(state));
    EXPECT_CALL(*apiMock, metrics(testing::_)).Times(0);

    auto unleashClient = static_cast<unleash::UnleashClient>(unleash::UnleashClient::create("appName", "urlMock")
                                                                     .apiClient(apiMock)
                                                                     .metricsInterval(500));
    unleashClient.initializeClient();
    unleashClient.isEnabled("metrics.flag");

    std::this_thread::sleep_for(std::chrono::milliseconds(800));
}