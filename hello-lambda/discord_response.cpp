#include "discord_resopnse.hpp"

#include <aws/core/utils/json/JsonSerializer.h>

using namespace discord;
using namespace Aws::Utils::Json;

DiscordResponse DiscordResponse::ping()
{
    JsonValue response(R"({
        "statusCode": 200,
        "isBase64Encoded": false,
        "headers": {
            "content-type": "application/json"
        },
        "body": "{\"type\": 1 }"
        })");
    return {200,  response.View().WriteCompact()};
}

DiscordResponse DiscordResponse::bad_request()
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

DiscordResponse DiscordResponse::message(const std::string &body)
{
    JsonValue response(R"({
        "statusCode": 200,
        "isBase64Encoded": false,
        "headers": {
            "content-type": "application/json"
        },
        "body": "{\"message\": \"pong\" }"
        })");
    return {200, response.View().WriteCompact()};
}

DiscordResponse DiscordResponse::success(const std::string &resp)
{
    return {200, resp};
}

DiscordResponse DiscordResponse::failure(int statusCode, const std::string &message)
{
    return {statusCode, message};
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
