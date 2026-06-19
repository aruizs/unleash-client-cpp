# Unleash Client SDK for C++

![Build Status](https://github.com/aruizs/unleash-client-cpp/actions/workflows/ubuntu.yml/badge.svg)
![Build Status](https://github.com/aruizs/unleash-client-cpp/actions/workflows/windows.yml/badge.svg)
![Build Status](https://github.com/aruizs/unleash-client-cpp/actions/workflows/macos.yml/badge.svg)
[![codecov](https://codecov.io/gh/aruizs/unleash-client-cpp/branch/main/graph/badge.svg?token=SFWVJY808A)](https://codecov.io/gh/aruizs/unleash-client-cpp)
[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=aruizs_unleash-client-cpp&metric=sqale_rating)](https://sonarcloud.io/summary/new_code?id=aruizs_unleash-client-cpp)
[![Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=aruizs_unleash-client-cpp&metric=reliability_rating)](https://sonarcloud.io/summary/new_code?id=aruizs_unleash-client-cpp)
[![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=aruizs_unleash-client-cpp&metric=security_rating)](https://sonarcloud.io/summary/new_code?id=aruizs_unleash-client-cpp)

This repository provides a C++ client SDK for Unleash that meets
the [Unleash Client Specifications](https://github.com/Unleash/client-specification).

📖 API reference: https://aruizs.github.io/unleash-client-cpp/

## Features

The below table shows what features the SDK supports or plans to support.

- [x] Feature toggles
- [x] Built-in strategies
- [x] Unleash context
- [x] Strategy constraints
- [x] Constraint operators (IN/NOT_IN, string, numeric, date, SemVer, regex, CIDR)
- [x] Segments (global constraints)
- [x] Application registration
- [x] Variants
- [x] Strategy variants
- [x] Custom strategies
- [x] Dependent features
- [x] Custom stickiness
- [x] Bootstrapping (local cache fallback)
- [x] Delta API (hydration and event stream)
- [x] Usage Metrics

The client passes the full [Unleash Client Specification](https://github.com/Unleash/client-specification) test suite.

## Requirements

- Compatible C++17 compiler such as Clang or GCC. The minimum required versions are Clang 4 and g++7.

## Usage

### Include Unleash library

    #include <unleash/unleashclient.h>

### Initialization

The `unleashClient` can be initialized with the following parameters but only `appName` and `unleashServerUrl` are
mandatories.

| Config                | Required? | Type   | Default value |
|-----------------------|-----------|--------|---------------|
| Unleash URL           | Yes       | String | N/A           |
| App. Name             | Yes       | String | N/A           |
| Instance ID.          | No        | String | N/A           |
| Environment           | No        | String | N/A           |
| Authentication        | No        | String | N/A           |
| Refresh Interval (ms) | No        | Int    | 15000         |
| Registration          | No        | Bool   | False         |
| Cache File Path       | No        | String | N/A           |
| Metrics               | No        | Bool   | False         |
| Metrics Interval (ms) | No        | Int    | 60000         |

    auto unleashClient = unleash::UnleashClient::create("appName", "unleashServerUrl")
            .instanceId("instanceId")
            .environment("environment")
            .authentication("token")
            .refreshInterval(pollingTime)
            .registration(boolValue)
            .metrics(boolValue)
            .metricsInterval(metricsTime)
            .build();
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

- Toggle with a fallback value for unknown flags:

```
    // Returns the default value only when the flag is not known to the server.
    // A flag that exists is always evaluated on its own merits.
    unleashClient.isEnabled("feature.toogle", true);
    unleashClient.isEnabled("feature.toogle", context, true);
```

### Getting a Variant

``` 
    #include "unleash/context.h"
    ...
    unleash::Context context{"userId"};
    auto variant = unleashClient.variant("feature.toogle", context);
    ...
    /*
    The variant response is an instance of the following structure:
    {
      std::string name;
      unsigned int weight;
      bool enabled;
      bool feature_enabled;
      std::string payload;
    }
    */
```

For more information about variants, see the [Variant documentation](https://docs.getunleash.io/advanced/toggle_variants).

### Custom Strategies

You can register your own activation strategy by subclassing `unleash::Strategy` and registering a
factory on the builder. A custom strategy receives the feature's `parameters` and `constraints`
(both as JSON strings), and calls `meetConstraints()` to honour any configured constraints. A
registered name overrides the built-in strategy of the same name.

```cpp
#include "unleash/strategies/strategy.h"

class MinUserIdStrategy : public unleash::Strategy {
public:
    MinUserIdStrategy(std::string_view parameters, std::string_view constraints)
        : unleash::Strategy("minUserId", constraints) {
        // parse `parameters` (JSON) here
    }
    bool isEnabled(const unleash::Context &context) override {
        return meetConstraints(context) && /* your logic */ true;
    }
};

auto client = unleash::UnleashClient::create("appName", "unleashServerUrl")
        .registerStrategy("minUserId",
                          [](std::string_view parameters, std::string_view constraints) {
                              return std::make_unique<MinUserIdStrategy>(parameters, constraints);
                          })
        .build();
client.initializeClient();
```

### Bootstrapping (Local Cache)

The client supports bootstrapping from a local cache file. This provides offline resilience when the Unleash server is unavailable.

```cpp
auto unleashClient = unleash::UnleashClient::create("appName", "unleashServerUrl")
        .cacheFilePath("/path/to/cache.json")
        .build();
unleashClient.initializeClient();
```

**How it works:**
- On successful API response, the client saves the feature configuration to the cache file
- If the API fails during initialization, the client loads features from the cache file
- During periodic refresh, if the API fails, the client falls back to the cached configuration

This ensures your application can start and operate with the last known feature flag state even when the Unleash server is temporarily unavailable.

### Usage Metrics

When enabled, the client tracks how often each feature flag is evaluated (and which variants are served) and periodically reports these counts to the Unleash server's `/client/metrics` endpoint. This powers the usage and "last seen" data shown in the Unleash UI. Metrics are **disabled by default**.

```cpp
auto unleashClient = unleash::UnleashClient::create("appName", "unleashServerUrl")
        .metrics(true)
        .metricsInterval(60000)
        .build();
unleashClient.initializeClient();
```

**How it works:**
- Each `isEnabled` call records a `yes`/`no` count for the flag, and each `variant` call additionally records the served variant
- Counts are bucketed in memory and flushed to the server every `metricsInterval` milliseconds (default 60000)
- Counting uses a dedicated lock kept off the evaluation path, so it adds minimal overhead

## Integration

### Building with CMake

The following requirements need to be installed to build the library using CMake:

- CMake 3.19+
- Conan 2

By default, it provides the static library. The shared version shall be available using the CMake
option `BUILD_SHARED_LIB=YES`.

The installation files include the `UnleashConfig.cmake` to integrate this library using the target `unleash::unleash`.

To build unleash client with Conan and CMake run the following commands:

```bash
conan install . -s build_type=Debug -s compiler.cppstd=17 --build=missing
cmake --preset conan-debug
cmake --build --preset conan-debug
```

For a Release build:

```bash
conan install . -s build_type=Release -s compiler.cppstd=17 --build=missing
cmake --preset conan-release
cmake --build --preset conan-release
```

The project uses CMake presets generated by Conan for simplified configuration.

A runnable example lives in [`examples/`](examples/quickstart.cpp). Build it with
`-DUNLEASH_BUILD_EXAMPLES=ON`.

To build with a sanitizer (used by CI), configure with `-DUNLEASH_SANITIZER=address`
(or `thread` / `undefined`).

### Conan

This package is published in Conan Center as [unleash-client-cpp/1.5.1](https://conan.io/center/unleash-client-cpp).

### vcpkg

A vcpkg port is maintained in [`vcpkg/ports/unleash-client-cpp`](vcpkg/ports/unleash-client-cpp).
Until it is merged into the upstream registry, consume it as an overlay port:

```bash
vcpkg install unleash-client-cpp --overlay-ports=vcpkg/ports
```

Then, in your CMake project:

```cmake
find_package(unleash CONFIG REQUIRED)
target_link_libraries(main PRIVATE unleash::unleash)
```

## Tested services

- *Gitlab* using `application name` and `instance id` parameters for authentication.
- *Self-hosted unleash* using `client token` for authentication.

## Used third-party tools

Thanks a lot to the following tools for your contribution:

- [Building a Dual Shared and Static Library with CMake](https://github.com/alexreinking/SharedStaticStarter) for the
  CMake library template.
- [C++ Requests: Curl for People](https://github.com/libcpr/cpr) for the API client library.
- [JSON for Modern C++](https://github.com/nlohmann/json) for the JSON handling library.
- [Codecov](https://about.codecov.io/) for code coverage solution.
- [Sonarcloud](https://sonarcloud.io/) for the static code analysis.
- [CMake](https://cmake.org/) for the C++ build system.
- [Conan.io](https://conan.io/) for the C++ package manager.
- [GitHub](https://github.com/) for the repository and CI/CD services.
- [GoogleTest](https://github.com/google/googletest) for C++ testing.
