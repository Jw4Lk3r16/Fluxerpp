#include "fluxerpp/FluxerClient.h"

namespace fluxerpp {

FluxerClient::FluxerClient(const FluxerConfig& cfg)
    : rest(cfg), gate(cfg.token) {
    // Lets GatewayClient::connect() resolve GET /gateway/bot instead of
    // dialing a hardcoded host. `rest` is declared before `gate` in
    // FluxerClient.h, so it's already fully constructed here.
    gate.bind_rest(&rest);

    // Register the internal guild cache handler FIRST, before any user
    // handlers. Because on_guild_create callbacks run in registration order,
    // this ensures the cache is populated by the time user code runs.
    gate.on_guild_create([this](const models::Guild& guild) {
        std::lock_guard<std::mutex> lk(guilds_mutex_);
        guilds_[guild.id] = guild;
    });
}

RestClient& FluxerClient::api() {
    return rest;
}

GatewayClient& FluxerClient::gateway() {
    return gate;
}

void FluxerClient::login() {
    gate.connect();
}

const models::Guild* FluxerClient::guild(std::uint64_t id) const {
    std::lock_guard<std::mutex> lk(guilds_mutex_);
    auto it = guilds_.find(id);
    if (it != guilds_.end()) {
        return &it->second;
    }
    return nullptr;
}

} // namespace fluxerpp