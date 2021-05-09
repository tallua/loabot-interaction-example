#include "discord.hpp"

#include <fstream>
#include <iostream>

#include <aws/core/utils/json/JsonSerializer.h>
#include <sodium.h>

using namespace discord;
using namespace Aws::Utils::Json;

namespace
{
    int hex_to_number(char ch)
    {
        if ('0' <= ch && ch <= '9')
        {
            return ch - '0';
        }
        if ('a' <= ch && ch <= 'f')
        {
            return ch - 'a' + 10;
        }
        if ('A' <= ch && ch <= 'F')
        {
            return ch - 'A' + 10;
        }

        throw std::runtime_error("invalid hex character");
    }

    std::vector<unsigned char> to_buffer_hex(const Aws::String &str)
    {
        std::vector<unsigned char> result;
        result.reserve(str.size() / 2u);

        for (size_t i = 0; i < str.size(); i += 2)
        {
            unsigned char ch =
                hex_to_number(str[i]) * 16 +
                hex_to_number(str[i + 1]);
            result.push_back(ch);
        }

        return result;
    }

    std::vector<unsigned char> to_buffer(const Aws::String &str)
    {
        std::vector<unsigned char> result;
        result.reserve(str.size());

        for (auto &&ch : str)
        {
            result.push_back(static_cast<unsigned char>(ch));
        }

        return result;
    }
}

JsonValue DiscordMessage::to_json() const
{
    JsonValue result;
    result.WithBool("tts", false)
        .WithString("content", content)
        .WithArray("embeds", Aws::Utils::Array<JsonValue>())
        .WithObject("allowed_mentions", JsonValue()
                                            .WithArray("parse", Aws::Utils::Array<JsonValue>())
                                            .WithArray("roles", Aws::Utils::Array<JsonValue>())
                                            .WithArray("users", Aws::Utils::Array<JsonValue>())
                                            .WithBool("replied_user", false))
        .WithInteger("flags", 0);

    return result;
}

DiscordResponse DiscordResponse::BadRequest()
{
    JsonValue response(R"({
        "statusCode": 400,
        "isBase64Encoded": false,
        "headers": {
            "content-type": "application/json"
        },
        "body": "{\"message\": \"Bad Request\" }"
        })");

    return {400, response.View().WriteCompact()};
}

DiscordResponse DiscordResponse::Ping()
{
    JsonValue response(R"({
        "statusCode": 200,
        "isBase64Encoded": false,
        "headers": {
            "content-type": "application/json"
        },
        "body": "{\"type\": 1 }"
        })");
    return {200, response.View().WriteCompact()};
}

DiscordResponse DiscordResponse::Immediate(const DiscordMessage &message)
{
    JsonValue body;
    body.WithInteger("type", 4)
        .WithObject("data", message.to_json());
    JsonValue response(R"({
        "statusCode": 200,
        "isBase64Encoded": false,
        "headers": {
            "content-type": "application/json"
        },
        "body": "{\"type\": 4 }"
        })");
    response.WithString("body", body.View().WriteCompact());

    return {200, response.View().WriteCompact()};
}

DiscordResponse DiscordResponse::Pending()
{
    JsonValue response(R"({
        "statusCode": 200,
        "isBase64Encoded": false,
        "headers": {
            "content-type": "application/json"
        },
        "body": "{\"type\": 5 }"
        })");
    return {200, response.View().WriteCompact()};
}

aws::lambda_runtime::invocation_response DiscordResponse::response() const
{
    if (200 <= statusCode && statusCode < 300)
    {
        return aws::lambda_runtime::invocation_response::success(payload, "application/json");
    }
    else
    {
        return aws::lambda_runtime::invocation_response::failure("{\"message\":\"" + payload + "\"}", std::to_string(statusCode));
    }
}

void DiscordRouter::setHandler(const Aws::String &command, CommandHandler handler)
{
    handlers[command] = handler;
}

DiscordResponse DiscordRouter::route(Aws::Utils::Json::JsonView body)
{
    if (!body.KeyExists("data") ||
        !body.GetObject("data").KeyExists("name") ||
        !body.KeyExists("token"))
    {
        throw std::runtime_error("invalid body type");
    }

    const Aws::String command = body.GetObject("data").GetString("name");
    auto handler_it = handlers.find(command);
    if (handler_it == handlers.end())
    {
        return DiscordResponse::BadRequest();
    }

    return handler_it->second(body);
}

DiscordBot::DiscordBot()
{
    load();
}

void DiscordBot::load()
{
    std::ifstream ifs("./config/bot.json");
    std::string config_str((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());

    JsonValue config(config_str);
    Aws::String raw_public_key = config.View().GetString("APPLICATION_KEY");
    public_key = to_buffer_hex(raw_public_key);
    std::cout << "INIT: PUBLIC KEY length: " << public_key.size() << std::endl;

    app_id = config.View().GetString("APPLICATION_ID");
    bot_token = config.View().GetString("BOT_TOKEN");
}

JsonValue DiscordBot::sessionInfo(Aws::String token)
{
    JsonValue result;

    result.WithString("app_id", app_id);
    result.WithString("interaction_token", token);
    result.WithString("bot_token", bot_token);

    return result;
}

bool DiscordBot::verify(Aws::Utils::Json::JsonView request)
{
    if (!request.KeyExists("headers") ||
        !request.KeyExists("body"))
    {
        std::cout << "ERROR: invalid request format" << std::endl;
        return false;
    }

    auto header = request.GetObject("headers");
    if (!header.KeyExists("x-signature-ed25519") ||
        !header.KeyExists("x-signature-timestamp"))
    {
        std::cout << "ERROR: invalid key on header" << std::endl;
        return false;
    }

    if (public_key.size() != crypto_sign_PUBLICKEYBYTES)
    {
        std::cout << "ERROR: invalid public key length" << std::endl;
        return false;
    }

    auto signature = header.GetString("x-signature-ed25519");
    auto timestamp = header.GetString("x-signature-timestamp");
    auto raw_body = request.GetString("body");

    auto message = timestamp + raw_body;

    auto signature_hex = to_buffer_hex(signature);
    auto message_hex = to_buffer(message);

    if (crypto_sign_verify_detached(signature_hex.data(),
                                    message_hex.data(), message_hex.size(),
                                    public_key.data()) != 0)
    {
        std::cout << "ERROR: verification failed" << std::endl;
        return false;
    }

    return true;
}