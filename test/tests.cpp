#include "unleash/api/apiclient.h"
#include "unleash/context.h"
#include "unleash/strategies/strategy.h"
#include "unleash/unleashclient.h"
#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

using ::testing::Return;


class ApiClientMock : public unleash::ApiClient {
public:
    MOCK_METHOD(std::string, features, (), (override));
    MOCK_METHOD(bool, registration, (unsigned int), (override));
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
    for (auto &element : j) {  // Only features implemented for now
        auto testNumber = std::stoi(element.get<std::string>().substr(0, 2));
        if (testNumber <= 14) {
            std::cout << testPath + element.get<std::string>() << std::endl;
            std::ifstream testFile(testPath + element.get<std::string>());
            nlohmann::json testJson;
            testFile >> testJson;
            if (testJson.contains("tests"))
                values.push_back(std::make_tuple(testJson["state"].dump(), testJson["tests"].dump(), false));
            else if (testJson.contains("variantTests"))
                values.push_back(std::make_tuple(testJson["state"].dump(), testJson["variantTests"].dump(), true));
        }
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
                                     contextJson.value("appName", ""), contextJson.value("currentTime", "")};
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
            EXPECT_EQ(expectedResult["feature_enabled"], variant.featureEnabled);
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