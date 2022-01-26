# Unleash Client SDK for C++

![Build Status](https://github.com/aruizs/unleash-client-cpp/actions/workflows/ubuntu.yml/badge.svg)
![Build Status](https://github.com/aruizs/unleash-client-cpp/actions/workflows/windows.yml/badge.svg)
![Build Status](https://github.com/aruizs/unleash-client-cpp/actions/workflows/macos.yml/badge.svg)
[![codecov](https://codecov.io/gh/aruizs/unleash-client-cpp/branch/main/graph/badge.svg?token=SFWVJY808A)](https://codecov.io/gh/aruizs/unleash-client-cpp)
[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=aruizs_unleash-client-cpp&metric=sqale_rating)](https://sonarcloud.io/summary/new_code?id=aruizs_unleash-client-cpp)
[![Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=aruizs_unleash-client-cpp&metric=reliability_rating)](https://sonarcloud.io/summary/new_code?id=aruizs_unleash-client-cpp)
[![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=aruizs_unleash-client-cpp&metric=security_rating)](https://sonarcloud.io/summary/new_code?id=aruizs_unleash-client-cpp)

This repository provides a C++ client SDK for Unleash that meets the [Unleash Client Specifications](https://github.com/Unleash/client-specification).

## Features

The below table shows what features the SDKs support or plan to support.

- [x] Feature toggles
- [x] Built-in strategies
- [x] Unleash context
- [ ] Custom strategies
- [ ] Strategy constrains
- [ ] Variants
- [ ] Usage Metrics


## Requirements

- Compatible C++17 compiler such as Clang or GCC. The minimum required versions are Clang 4 and g++7.

## Usage

### Include Unleash library

    #include <unleash/unleashclient.h>

### Initialization

The `unleashClient` can be initialized with the following parameters but only `appName` and `unleashServerUrl` are mandatories.



| Config      | Required? | Type | Default value |
|-------------|-----------| ---- |---------------|
| Unleash URL | Yes | String | N/A |
| App. Name   | Yes | String | N/A |
| Instance ID. | No        | String | N/A |
| Environment | No        | String | N/A |
| Authentication | No        | String | N/A |
| Refresh Interval (ms) | No        | Int | 15000 |



    unleash::UnleashClient unleashClient = unleash::UnleashClient::create("appName", "unleashServerUrl").instanceId("intanceId").environment("environment").authentication("token).refreshInterval("pollingTime");
    unleashClient.initializeClient();






### Feature Flag is enabled?


- Simple toggle:


```
unleashClient.isEnabled("feature.toogle");
```


- Toggle with context:


```
    #include "unleash/context.h"
    ...
    unleash::Context context{"userId", "sessionId", "remoteAddress"}
    unleashClient.isEnabled("feature.toogle", context);
```

## Integration
### CMake

*To be documented*

### Conan

*To be documented. This package is pending to be published in Conan Center as unleash-client-cpp/1.0.0*


## Used third-party tools

Thanks a lot to the following tools for your contribution:

- [Building a Dual Shared and Static Library with CMake](https://github.com/alexreinking/SharedStaticStarter) for the CMake library template.
- [C++ Requests: Curl for People](https://github.com/libcpr/cpr) for the API client library.
- [JSON for Modern C++](https://github.com/nlohmann/json) for the JSON handling library.
- [Codecov](https://about.codecov.io/) for code coverage solution.
- [Sonarcloud](https://sonarcloud.io/) for the static code analysis.
- [CMake](https://cmake.org/) for the C++ build system.
- [Conan.io](https://conan.io/) for the C++ package manager.
- [GitHub](https://github.com/) for the repository and CI/CD services.