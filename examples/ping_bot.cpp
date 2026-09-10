// examples/ping_bot.cpp
//
// Ping bot demonstrating:
// - Guild discovery via GUILD_CREATE (auto-cached by FluxerClient)
// - Channel lookup via the cache (no hardcoded IDs)
// - READY event
// - MESSAGE_CREATE event with latency tracking
// - Embed replies using the model system

#include "fluxerpp/FluxerClient.h"
#include "fluxerpp/env.h"
#include "fluxerpp/util/Logger.h"
#include "fluxerpp/models/Embed.h"
#include "fluxerpp/models/EmbedField.h"
#include "fluxerpp/models/Message.h"

#include <cstdlib>
#include <iostream>
#include <atomic>
#include <chrono>
#include <optional>

int main() {
    fluxerpp::load_env();

    const char* tokenEnv = std::getenv("FLUXER_TOKEN");
    if (!tokenEnv) {
        std::cerr << "FLUXER_TOKEN not set (check your .env)\n";
        return 1;
    }

    fluxerpp::FluxerConfig cfg;
    cfg.token = tokenEnv;

    fluxerpp::FluxerClient client(cfg);

    client.gateway().set_debug_logging(false);  // set to true for verbose frame logging
    fluxerpp::util::Logger::instance().set_level(fluxerpp::util::LogLevel::Info);

    // ============================================================
    // LATENCY TRACKING (gateway heartbeat ACK)
    // ============================================================
    std::atomic<int> last_gateway_latency{0};

    client.gateway().on_latency([&](int ms) {
        last_gateway_latency.store(ms);
    });

    // ============================================================
    // TARGET CHANNEL DISCOVERY
    // ============================================================
    // Instead of hardcoding FLUXER_CHANNEL_ID, find the first text channel
    // in the bot's first guild. This gets populated by GUILD_CREATE events
    // and auto-cached by FluxerClient.
    std::optional<std::uint64_t> target_channel_id;

    client.gateway().on_guild_create([&](const fluxerpp::models::Guild& guild) {
        std::cout << "Guild ready: " << guild.name << " (" << guild.channels.size() << " channels)\n";

        if (!target_channel_id.has_value() && !guild.channels.empty()) {
            target_channel_id = guild.channels[0].id;
            std::cout << "Using channel: " << guild.channels[0].name << " (" << target_channel_id.value() << ")\n";
        }
    });

    // ============================================================
    // READY EVENT
    // ============================================================
    client.gateway().on_ready([&client, &target_channel_id]() {
        std::cout << "Ping bot is ready.\n";

        if (target_channel_id.has_value()) {
            try {
                client.api().send_message(target_channel_id.value(), "I'm online!");
            } catch (const std::exception& ex) {
                std::cerr << "Failed to send ready message: " << ex.what() << "\n";
            }
        } else {
            std::cerr << "No channel discovered yet; skipping ready message\n";
        }
    });

    // ============================================================
    // MESSAGE HANDLING
    // ============================================================
    client.gateway().on_message_create([&](const fluxerpp::models::Message& msg) {
        // Ignore bot's own messages
        if (msg.author.bot) return;

        // ============================================================
        // !ping command
        // ============================================================
        if (msg.content == "!ping") {
            auto reply_channel_id = msg.channel_id;

            try {
                client.api().send_message(reply_channel_id, "Pong!");
            } catch (const std::exception& ex) {
                std::cerr << "Failed to send pong: " << ex.what() << "\n";
                return;
            }

            auto api_latency = 0;  // could measure this with chrono if desired
            try {
                client.api().send_message(
                    reply_channel_id,
                    "Gateway latency: " + std::to_string(last_gateway_latency.load()) + "ms"
                );
            } catch (const std::exception& ex) {
                std::cerr << "Failed to send latency message: " << ex.what() << "\n";
            }

        // ============================================================
        // !Hello command
        // ============================================================
        } else if (msg.content == "!Hello") {
            auto reply_channel_id = msg.channel_id;
            std::string mention = "<@" + std::to_string(msg.author.id) + ">";

            try {
                client.api().send_message(reply_channel_id, "Hello " + mention);
            } catch (const std::exception& ex) {
                std::cerr << "Failed to send hello: " << ex.what() << "\n";
            }
        } else if (msg.content == "!Version") {
            client.api().send_message(reply_channel_id, "Fluxer++ v0.2.0 | FPP_BOT v1.1.0);
        }
    });

    client.login(); // blocks
    return 0;
}