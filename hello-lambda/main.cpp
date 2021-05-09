// main.cpp
#include <iostream>
#include <string>

#include <aws/lambda-runtime/runtime.h>
#include <aws/core/utils/json/JsonSerializer.h>

#include "discord_resopnse.hpp"

using namespace discord;
using namespace aws::lambda_runtime;
using namespace Aws::Utils::Json;

bool verify(JsonView const &request)
{
    return true;
}

invocation_response my_handler(invocation_request const &request)
{
    JsonValue request_json(request.payload);
    if (!request_json.WasParseSuccessful() ||
        !request_json.View().KeyExists("headers") ||
        !request_json.View().KeyExists("body"))
    {
        std::cout << "ERROR: invalid request format" << std::endl;
        return DiscordResponse::bad_request().response();
    }

    if (!verify(request_json.View()))
    {
        std::cout << "ERROR: authorization fail" << std::endl;
        return DiscordResponse::bad_request().response();
    }

    JsonValue body_json(request_json.View().GetString("body"));
    if (!body_json.WasParseSuccessful() ||
        !body_json.View().KeyExists("type"))
    {
        std::cout << "ERROR: invalid discord message format" << std::endl;
        std::cout << "ERROR: body:" << std::endl;
        std::cout << request_json.View().GetString("body") << std::endl;
        return DiscordResponse::bad_request().response();
    }

    const int message_type = body_json.View().GetInteger("type");
    switch (message_type)
    {
    case 1:
    {
        std::cout << "RESULT: ping response" << std::endl;
        return DiscordResponse::ping().response();
    }
    case 2:
    {
        try
        {
            // should route here
            DiscordResponse response = DiscordResponse::message("\"pong\"");
            std::cout << "RESULT: success" << std::endl;
            return response.response();
        }
        catch (const std::exception &e)
        {
            std::cout << "ERROR: std::exception [" << e.what() << "]" << std::endl;
            return DiscordResponse::bad_request().response();
        }
        catch (...)
        {
            std::cout << "ERROR: unknown exception" << std::endl;
            return DiscordResponse::bad_request().response();
        }
    }
    default:
    {
        std::cout << "ERROR: type unknown [" << message_type << "]" << std::endl;
        return DiscordResponse::bad_request().response();
    }
    }
}

int main()
{
    run_handler(my_handler);
    return 0;
}