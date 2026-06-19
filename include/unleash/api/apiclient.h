#ifndef UNLEASH_APICLIENT_H
#define UNLEASH_APICLIENT_H

#include <string>

namespace unleash {
/**
 * @brief Transport abstraction between UnleashClient and the Unleash server.
 *
 * The default implementation (CprClient) speaks HTTP. Provide a custom implementation to
 * change the transport or to stub the server in tests.
 */
class ApiClient {
public:
    virtual ~ApiClient() = default;
    /// @brief Fetches the feature configuration document.
    virtual std::string features() = 0;
    /// @brief Registers this client instance with the server.
    virtual bool registration(unsigned int refreshInterval) = 0;
    /**
     * @brief Reports a usage-metrics payload to the server.
     *
     * Non-pure with a default so existing custom ApiClient implementations keep compiling;
     * override it to enable usage-metrics reporting.
     */
    virtual bool metrics(const std::string & /*payload*/) { return false; }
};
}  // namespace unleash

#endif  //UNLEASH_APICLIENT_H
