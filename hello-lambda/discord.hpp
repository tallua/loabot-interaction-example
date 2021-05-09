#pragma once

#include <map>

#include <aws/lambda-runtime/runtime.h>
#include <aws/core/utils/json/JsonSerializer.h>

namespace discord
{
    struct DiscordMessage
    {
        Aws::String message;
    };

    struct DiscordResponse
    {
        static DiscordResponse BadRequest();
        static DiscordResponse Ping();
        static DiscordResponse Immediate(const DiscordMessage &message);
        static DiscordResponse Pending();

        int statusCode;
        Aws::String payload;

        aws::lambda_runtime::invocation_response response();
    };

    class DiscordRouter
    {
    public:
        using CommandHandler = std::function<DiscordResponse(Aws::Utils::Json::JsonView)>;
    public:
        void setHandler(const Aws::String &command, CommandHandler handler);

        DiscordResponse route(Aws::Utils::Json::JsonView body);

    private:
        std::map<Aws::String, CommandHandler> handlers;
    };

    class DiscordBot
    {
    public:
        void load();

        bool verify(Aws::Utils::Json::JsonView request);
    };
}