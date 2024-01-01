#ifndef UNLEASH_VARIANTS_H
#define UNLEASH_VARIANTS_H
#include <string>
#include <vector>

namespace unleash {

class Context;

struct Override {
    std::string contextName;
    std::vector<std::string> values;
};

struct variant_t {
    std::string name;
    unsigned int weight;
    bool enabled;
    bool featureEnabled;
    std::string payload;
};

class Variant {
public:
    Variant(std::string name, unsigned int weight, std::string_view payload, std::string_view overrides);
    unsigned int getWeight() const { return m_weight; }
    const std::string &getName() const { return m_name; }
    const std::vector<Override> &getOverrides() const { return m_overrides; }
    const std::string &getPayload() const { return m_payload; }

private:
    std::string m_name;
    unsigned int m_weight;
    std::string m_payload;
    std::vector<Override> m_overrides;
};
}  // namespace unleash


#endif  //UNLEASH_VARIANTS_H