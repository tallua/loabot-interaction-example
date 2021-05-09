#pragma once

#include <string>

#include <aws/lambda-runtime/runtime.h>

namespace discord
{
    struct DiscordResponse
    {
        static DiscordResponse ping();
        static DiscordResponse bad_request();
        static DiscordResponse message(const std::string &body);

        static DiscordResponse success(const std::string &resp);
        static DiscordResponse failure(int statusCode, const std::string &message);

        int statusCode;
        std::string payload;

        aws::lambda_runtime::invocation_response response();
    };

}