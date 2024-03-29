#include "unleash/strategies/gradualrolloutrandom.h"
#include <nlohmann/json.hpp>
#include <random>


namespace unleash {
GradualRolloutRandom::GradualRolloutRandom(std::string_view parameters, std::string_view constraints)
    : Strategy("flexibleRollout", constraints) {
    auto gradualRollout_json = nlohmann::json::parse(parameters);
    m_percentage = (gradualRollout_json["percentage"].type() == nlohmann::json::value_t::string)
                           ? std::stoi(gradualRollout_json["percentage"].get<std::string>())
                           : uint32_t(gradualRollout_json["percentage"]);
}

bool GradualRolloutRandom::isEnabled(const Context &context) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist6(1, 100);
    return dist6(rng) <= m_percentage;
}
}  // namespace unleash