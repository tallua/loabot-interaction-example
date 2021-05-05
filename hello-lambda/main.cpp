// main.cpp
#include <iostream>
#include <string>

#include <aws/lambda-runtime/runtime.h>

using namespace aws::lambda_runtime;

extern std::string route(invocation_request const &request);

bool verify(invocation_request const &request)
{
    return true;
}

struct DiscordResponse
{
    static DiscordResponse success(const std::string &resp);
    static DiscordResponse failure(int statusCode, const std::string &message);

    int statusCode;
    std::string payload;

    invocation_response response();
};

DiscordResponse DiscordResponse::success(const std::string &resp)
{
    return {200, resp};
}
DiscordResponse DiscordResponse::failure(int statusCode, const std::string &message)
{
    return {statusCode, message};
}

invocation_response DiscordResponse::response()
{
    if (200 <= statusCode && statusCode < 300)
    {
        return invocation_response::success(payload, "application/json");
    }
    else
    {
        return invocation_response::failure("{\"message\":\"" + payload + "\"}", std::to_string(statusCode));
    }
}

invocation_response my_handler(invocation_request const &request)
{
    if (!verify(request))
    {
        std::cout << "RESULT: authorization fail" << std::endl;
        return DiscordResponse::failure(400, "Bad Request").response();
    }

    try
    {
        std::cout << "RESULT: " << request.payload << std::endl;
        return DiscordResponse::success("pong").response();
    }
    catch (...)
    {
        return DiscordResponse::failure(400, "Bad Request").response();
    }
}

int main()
{
    run_handler(my_handler);
    return 0;
}