// main.cpp
#include <iostream>
#include <string>
#include <functional>

#include <aws/core/Aws.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/lambda-runtime/runtime.h>
#include <aws/lambda/LambdaClient.h>
#include <aws/lambda/model/InvokeRequest.h>

#include "discord.hpp"

using namespace discord;
using namespace aws::lambda_runtime;
using namespace Aws::Utils::Json;
using namespace Aws::Lambda;
using namespace Aws::Lambda::Model;

#define LAMBDA_CLIENT_NAME "discord-bot"

std::shared_ptr<DiscordBot> bot;
DiscordRouter router;

invocation_response my_handler(invocation_request const &request)
{
    JsonValue request_json(request.payload);
    if (!request_json.WasParseSuccessful())
    {
        std::cout << "ERROR: request format is not json" << std::endl;
        return DiscordResponse::BadRequest().response();
    }

    if (!bot->verify(request_json.View()))
    {
        std::cout << "ERROR: authorization fail" << std::endl;
        return DiscordResponse::BadRequest().response();
    }

    JsonValue body_json(request_json.View().GetString("body"));
    if (!body_json.WasParseSuccessful() ||
        !body_json.View().KeyExists("type"))
    {
        std::cout << "ERROR: invalid discord message format" << std::endl;
        std::cout << "ERROR: body:" << std::endl;
        std::cout << request_json.View().GetString("body") << std::endl;
        return DiscordResponse::BadRequest().response();
    }

    const int message_type = body_json.View().GetInteger("type");
    switch (message_type)
    {
    case 1:
    {
        std::cout << "RESULT: ping response" << std::endl;
        return DiscordResponse::Ping().response();
    }
    case 2:
    {
        try
        {
            DiscordResponse response = router.route(body_json.View());
            std::cout << "RESULT: success: " << response.payload << std::endl;
            return response.response();
        }
        catch (const std::exception &e)
        {
            std::cout << "ERROR: exception [" << e.what() << "]" << std::endl;
            return DiscordResponse::BadRequest().response();
        }
        catch (...)
        {
            std::cout << "ERROR: unknown exception" << std::endl;
            return DiscordResponse::BadRequest().response();
        }
    }
    default:
    {
        std::cout << "ERROR: type unknown [" << message_type << "]" << std::endl;
        return DiscordResponse::BadRequest().response();
    }
    }
}

void callLambda(std::shared_ptr<LambdaClient> client,
                Aws::String lambda_name, JsonView payload)
{
    InvokeRequest invokeRequest;
    invokeRequest.SetFunctionName(lambda_name);
    invokeRequest.SetInvocationType(Aws::Lambda::Model::InvocationType::Event);
    invokeRequest.SetLogType(Aws::Lambda::Model::LogType::Tail);

    std::shared_ptr<Aws::IOStream> payload_stream = Aws::MakeShared<Aws::StringStream>("FunctionTest");
    *payload_stream << payload.WriteCompact();
    invokeRequest.SetBody(payload_stream);
    invokeRequest.SetContentType("application/json");

    auto outcome = client->Invoke(invokeRequest);

    if (outcome.IsSuccess())
    {
        auto &result = outcome.GetResult();

        int statusCode = result.GetStatusCode();
        if (statusCode < 200 || 300 <= statusCode)
        {
            throw std::runtime_error(result.GetLogResult());
        }
    }
    else
    {
        auto &error = outcome.GetError();

        throw std::runtime_error(error.GetMessage());
    }
}

Aws::String extract_option(JsonView param, Aws::String option)
{
    if (!param.IsObject() ||
        !param.KeyExists("data") ||
        !param.GetAllObjects()["data"].KeyExists("options"))
    {
        throw std::runtime_error("ERROR: invalid option format");
    }

    auto options = param.GetAllObjects()["data"].GetArray("options");
    for (std::size_t i = 0; i < options.GetLength(); ++i)
    {
        if (options.GetItem(i).GetString("name") == option)
        {
            return options.GetItem(i).GetString("value");
        }
    }

    throw std::runtime_error("ERROR: option not found");
}

void init()
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    std::cout << "INIT: loading bot information" << std::endl;
    bot = std::make_shared<DiscordBot>();

    std::cout << "INIT: creating lambda client" << std::endl;
    Aws::Client::ClientConfiguration clientConfig;
    clientConfig.region = "ap-northeast-2";
    auto client = Aws::MakeShared<Aws::Lambda::LambdaClient>(LAMBDA_CLIENT_NAME,
                                                             clientConfig);
    if (!client)
    {
        std::cout << "ERROR: Failed to create LambdaClient" << std::endl;
        return;
    }

    std::cout << "INIT: registering commands" << std::endl;
    router.setHandler("캐릭터", [client](JsonView body) -> DiscordResponse {
        try
        {
            JsonValue param;
            param.WithObject("discord", bot->sessionInfo(body.GetString("token")));
            param.WithString("user_name", extract_option(body, "이름"));

            callLambda(client, "loabot-back-get-character",
                       JsonValue().WithObject("param", param).View());
            return DiscordResponse::Pending();
        }
        catch (const std::exception &e)
        {
            std::cout << "ERROR: exception [" << e.what() << "]" << std::endl;
            return DiscordResponse::Immediate({"Invalid Request"});
        }
        catch (...)
        {
            std::cout << "ERROR: unknown exception" << std::endl;
            return DiscordResponse::Immediate({"Invalid Request"});
        }
    });
    router.setHandler("수집품", [client](JsonView body) -> DiscordResponse {
        try
        {
            JsonValue param;
            param.WithObject("discord", bot->sessionInfo(body.GetString("token")));
            param.WithString("user_name", extract_option(body, "이름"));
            param.WithString("collection_type", extract_option(body, "종류"));

            callLambda(client, "loabot-back-get-collection",
                       JsonValue().WithObject("param", param).View());
            return DiscordResponse::Pending();
        }
        catch (const std::exception &e)
        {
            std::cout << "ERROR: exception [" << e.what() << "]" << std::endl;
            return DiscordResponse::Immediate({"Invalid Request"});
        }
        catch (...)
        {
            std::cout << "ERROR: unknown exception" << std::endl;
            return DiscordResponse::Immediate({"Invalid Request"});
        }
    });
    router.setHandler("거래소", [client](JsonView body) -> DiscordResponse {
        try
        {
            JsonValue param;
            param.WithObject("discord", bot->sessionInfo(body.GetString("token")));
            param.WithString("item_name", extract_option(body, "이름"));

            callLambda(client, "loabot-back-get-market",
                       JsonValue().WithObject("param", param).View());
            return DiscordResponse::Pending();
        }
        catch (const std::exception &e)
        {
            std::cout << "ERROR: exception [" << e.what() << "]" << std::endl;
            return DiscordResponse::Immediate({"Invalid Request"});
        }
        catch (...)
        {
            std::cout << "ERROR: unknown exception" << std::endl;
            return DiscordResponse::Immediate({"Invalid Request"});
        }
    });
    router.setHandler("주사위", [client](JsonView body) -> DiscordResponse {
        try
        {
            JsonValue param;
            param.WithObject("discord", bot->sessionInfo(body.GetString("token")));

            callLambda(client, "loabot-back-dice",
                       JsonValue().WithObject("param", param).View());
            return DiscordResponse::Pending();
        }
        catch (const std::exception &e)
        {
            std::cout << "ERROR: exception [" << e.what() << "]" << std::endl;
            return DiscordResponse::Immediate({"Invalid Request"});
        }
        catch (...)
        {
            std::cout << "ERROR: unknown exception" << std::endl;
            return DiscordResponse::Immediate({"Invalid Request"});
        }
    });
    router.setHandler("핑", [](JsonView view) -> DiscordResponse {
        return DiscordResponse::Immediate({"퐁!"});
    });
}

int main()
{
    std::cout << "INIT: Start Init" << std::endl;
    init();
    std::cout << "INIT: End Init" << std::endl;

    run_handler(my_handler);

    return 0;
}