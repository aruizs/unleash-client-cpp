// Minimal end-to-end example of the Unleash C++ client.
//
// Build (from the repository root):
//   conan install . -s build_type=Release -s compiler.cppstd=17 --build=missing
//   cmake --preset conan-release -DUNLEASH_BUILD_EXAMPLES=ON
//   cmake --build --preset conan-release
//
// Run:
//   ./quickstart <unleash-url> <api-token> [app-name]

#include <unleash/context.h>
#include <unleash/unleashclient.h>
#include <iostream>

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << "Usage: quickstart <unleash-url> <api-token> [app-name]\n";
        return 0;
    }
    const std::string url = argv[1];
    const std::string token = argv[2];
    const std::string appName = argc > 3 ? argv[3] : "quickstart-app";

    // build() is the recommended construction path: it works with `auto` and
    // makes it obvious where configuration ends and the client begins.
    auto client = unleash::UnleashClient::create(appName, url)
                          .authentication(token)
                          .refreshInterval(15000)
                          .build();
    client.initializeClient();

    // Simple toggle. The second argument is the fallback returned only when the
    // flag is unknown to the server (a known flag is always evaluated normally).
    const bool enabled = client.isEnabled("my.feature", /*defaultValue=*/false);
    std::cout << "my.feature enabled: " << std::boolalpha << enabled << "\n";

    // Toggle evaluated against a user context.
    unleash::Context context;
    context.userId = "user-123";
    std::cout << "my.feature for user-123: " << client.isEnabled("my.feature", context, false) << "\n";

    // Fetch a variant.
    const auto variant = client.variant("my.feature", context);
    std::cout << "variant: " << variant.name << " (enabled=" << variant.enabled << ")\n";

    return 0;
}
