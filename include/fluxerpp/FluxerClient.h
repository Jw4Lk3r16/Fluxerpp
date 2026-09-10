#pragma once
#include "fluxerpp/RestClient.h"
#include "fluxerpp/GatewayClient.h"
#include "fluxerpp/FluxerConfig.h"
#include "fluxerpp/models/Guild.h"
#include <nlohmann/json.hpp>
#include <map>
#include <mutex>

namespace fluxerpp {

class FluxerClient {
public:
    explicit FluxerClient(const FluxerConfig& cfg);

    void login();
    RestClient& api();
    GatewayClient& gateway();

    // Guild cache accessors — populated automatically from GUILD_CREATE events
    // received on the gateway.
    // guild(id) is thread-safe for lookups during normal bot operation.
    const models::Guild* guild(std::uint64_t id) const;

private:
    RestClient rest;
    GatewayClient gate;

    // Guild cache, keyed by guild ID
    mutable std::mutex guilds_mutex_;
    std::map<std::uint64_t, models::Guild> guilds_;
};

} // namespace fluxerpp