#include "unleash/strategies/strategy.h"
#include "unleash/strategies/applicationhostname.h"
#include "unleash/strategies/default.h"
#include "unleash/strategies/flexiblerollout.h"
#include "unleash/strategies/gradualrolloutrandom.h"
#include "unleash/strategies/gradualrolloutsessionid.h"
#include "unleash/strategies/gradualrolloutuserid.h"
#include "unleash/strategies/remoteaddress.h"
#include "unleash/strategies/userwithid.h"

namespace unleash {
Strategy::Strategy(std::string name, std::string_view parameters) : m_name(std::move(name)), m_parameters(parameters) {
}

std::unique_ptr<Strategy> Strategy::createStrategy(std::string_view strategy, std::string_view parameters) {
    if (strategy == "default")
        return std::make_unique<Default>(parameters);
    else if (strategy == "userWithId")
        return std::make_unique<UserWithId>(parameters);
    else if (strategy == "applicationHostname")
        return std::make_unique<ApplicationHostname>(parameters);
    else if (strategy == "flexibleRollout")
        return std::make_unique<FlexibleRollout>(parameters);
    else if (strategy == "gradualRolloutUserId")
        return std::make_unique<GradualRolloutUserId>(parameters);
    else if (strategy == "gradualRolloutSessionId")
        return std::make_unique<GradualRolloutSessionId>(parameters);
    else if (strategy == "gradualRolloutRandom")
        return std::make_unique<GradualRolloutRandom>(parameters);
    else if (strategy == "remoteAddress")
        return std::make_unique<RemoteAddress>(parameters);
    return nullptr;
}


}  // namespace unleash
