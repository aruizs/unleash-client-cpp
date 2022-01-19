#include "unleash/api/apiclient.h"
#include "unleash/context.h"
#include "unleash/unleashclient.h"
#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>
#include <thread>

using ::testing::Return;


class ApiClientMock : public unleash::ApiClient {
public:
    MOCK_METHOD(std::string, features, (), (override));
};

using TestParam = std::pair<std::string, std::string>;

std::vector<TestParam> readSpecificationTestFromDisk(const std::string &testPath) {
    std::vector<TestParam> values;
    // Read index file
    auto testIndexPath = testPath + "index.json";
    std::ifstream i(testIndexPath);
    nlohmann::json j;
    i >> j;

    // range-based to read each test
    for (auto &element : j) {  // Only features implemented for now
        if (std::stoi(element.get<std::string>().substr(0, 2)) <= 7) {
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
    std::string currentPath = std::filesystem::current_path();
    const std::string stringToken = "unleash-client-cpp";
    auto pos = currentPath.find(stringToken);
    auto projectPath = currentPath.substr(0, pos + stringToken.length());
    auto testPath = projectPath + "/client-specification/specifications/";
    return testPath;
}

class UnleashSpecificationTest : public testing::TestWithParam<TestParam> {};

TEST_P(UnleashSpecificationTest, TestSet) {
    auto testData = GetParam();
    auto apiMock = std::make_shared<ApiClientMock>();
    unleash::UnleashClient unleashClient = unleash::UnleashClient::create("production", "urlMock").instanceId("intanceId").environment("production").apiClient(apiMock);
    EXPECT_CALL(*apiMock, features()).WillRepeatedly(Return(testData.first));
    unleashClient.initializeClient();
    nlohmann::json testSet = nlohmann::json::parse(testData.second);
    for (const auto &[key, value] : testSet.items()) {
        auto contextJson = value["context"];
        unleash::Context testContext{
                contextJson.value("userId", ""), contextJson.value("sessionId", ""), contextJson.value("remoteAddress", "")};
        EXPECT_EQ(unleashClient.isEnabled(value["toggleName"], testContext), value["expectedResult"].get<bool>());
    }
}

INSTANTIATE_TEST_SUITE_P(AllSpecificationFiles, UnleashSpecificationTest, testing::ValuesIn(readSpecificationTestFromDisk(getTestPath())));