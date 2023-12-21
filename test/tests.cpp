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

using TestParam = std::pair<std::string, std::string>;

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
        if (testNumber <= 11) {
            std::cout << testPath + element.get<std::string>() << std::endl;
            std::ifstream testFile(testPath + element.get<std::string>());
            nlohmann::json testJson;
            testFile >> testJson;
            values.push_back(std::pair(testJson["state"].dump(), testJson["tests"].dump()));
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
    unleash::UnleashClient unleashClient = unleash::UnleashClient::create("production", "urlMock");
    std::cout << unleashClient << std::endl;
    unleashClient.initializeClient();
    EXPECT_FALSE(unleashClient.isEnabled("feature.toogle"));
}

TEST(UnleashTest, InicializationErrorServerResponse) {
    unleash::UnleashClient unleashClient = unleash::UnleashClient::create("production", "https://www.apple.com/%");
    unleashClient.initializeClient();
    EXPECT_FALSE(unleashClient.isEnabled("feature.toogle"));
}

TEST(UnleashTest, RegistrationBadServerUrl) {
    unleash::UnleashClient unleashClient = unleash::UnleashClient::create("production", "urlMock").registration(true);
    std::cout << unleashClient << std::endl;
    unleashClient.initializeClient();
    EXPECT_FALSE(unleashClient.isEnabled("feature.toogle"));
}

TEST(UnleashTest, RegistrationErrorServerResponse) {
    unleash::UnleashClient unleashClient =
            unleash::UnleashClient::create("production", "https://www.apple.com/%").registration(true);
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
    unleash::UnleashClient unleashClient = unleash::UnleashClient::create("production", "urlMock")
                                                   .instanceId("intanceId")
                                                   .environment("production")
                                                   .apiClient(apiMock)
                                                   .refreshInterval(refreshInterval)
                                                   .authentication("clientToken")
                                                   .registration(true);
    EXPECT_CALL(*apiMock, features()).WillRepeatedly(Return(testData.first));
    EXPECT_CALL(*apiMock, registration(refreshInterval)).WillRepeatedly(Return(true));
    unleashClient.initializeClient();
    nlohmann::json testSet = nlohmann::json::parse(testData.second);
    unleashClient.initializeClient();  // Retry initialization to check nothing happens
    for (const auto &[key, value] : testSet.items()) {
        auto contextJson = value["context"];
        unleash::Context testContext{contextJson.value("userId", ""), contextJson.value("sessionId", ""),
                                     contextJson.value("remoteAddress", ""), contextJson.value("environment", ""),
                                     contextJson.value("appName", "")};
        if (contextJson.contains("properties")) {
            for (auto &[propertyKey, propertyValue] : contextJson["properties"].items()) {
                testContext.properties.try_emplace(propertyKey, propertyValue);
            }
        }
        EXPECT_EQ(unleashClient.isEnabled(value["toggleName"], testContext), value["expectedResult"].get<bool>());
    }
}

INSTANTIATE_TEST_SUITE_P(AllSpecificationFiles, UnleashSpecificationTest,
                         testing::ValuesIn(readSpecificationTestFromDisk(getTestPath())));