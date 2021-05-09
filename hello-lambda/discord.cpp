#include "discord.hpp"

#include <aws/core/utils/json/JsonSerializer.h>

using namespace discord;
using namespace Aws::Utils::Json;

DiscordResponse DiscordResponse::BadRequest()
{
    JsonValue response(R"({
        "statusCode": 400,
        "isBase64Encoded": false,
        "headers": {
            "content-type": "application/json"
        },
        "body": "{\"message\": "Bad Request" }"
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
    JsonValue response(R"({
        "statusCode": 200,
        "isBase64Encoded": false,
        "headers": {
            "content-type": "application/json"
        },
        "body": "{\"type\": 4 }"
        })");
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

aws::lambda_runtime::invocation_response DiscordResponse::response()
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
        !body.GetObject("data").KeyExists("name"))
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

void DiscordBot::load()
{
    // TODO: load key
}

bool DiscordBot::verify(Aws::Utils::Json::JsonView request)
{
    if (!request.KeyExists("headers") ||
        !request.KeyExists("body"))
    {
        return false;
    }

    // TODO: sodium verify

    return true;
}