// #include "utilities.h"
#include "order_execution.h"
#include "websocket_client.h"
#include <iostream>

const std::string API_KEY = "wOAbyf0v";
const std::string API_SECRET = "33VzZ9W4mi4Yx8Hhqy_4AjJfYrlhFfAFtELvHEXY63Q";

std::string authenticate(const std::string &api_key, const std::string &api_secret, WebSocketClient &client)
{
    // Build the authentication message
    web::json::value auth_message = web::json::value::object();
    auth_message[U("jsonrpc")] = web::json::value::string(U("2.0"));
    auth_message[U("id")] = web::json::value::number(1);
    auth_message[U("method")] = web::json::value::string(U("public/auth"));
    auth_message[U("params")] = web::json::value::object({{U("grant_type"), web::json::value::string(U("client_credentials"))},
                                                          {U("client_id"), web::json::value::string(U(api_key))},
                                                          {U("client_secret"), web::json::value::string(U(api_secret))}});

    // Send the authentication message via WebSocket
    client.send_message(auth_message);

    web::json::value response;
    client.receive_message([&response](const web::json::value &msg)
                           { response = msg; });

    // Store the token for future use
    std::string access_token_ = response[U("result")][U("access_token")].as_string();

    return access_token_;
}

int main()
{
    try
    {
        std::string websocket_url = "wss://test.deribit.com/ws/api/v2";
        WebSocketClient websocket_client(websocket_url);
        websocket_client.connect();

        std::string access_token = authenticate(API_KEY, API_SECRET, websocket_client);
        std::cout << "Authentication successful: " << std::endl;

        OrderExecution order_exec(API_KEY, API_SECRET, access_token, websocket_client);
        // order_exec.get_balance("BTC");
        // auto balance_response = order_exec.get_balance("ETH");
        // std::cout << "Balance response: " << balance_response.serialize() << std::endl;

        // order_exec.place_order("ETH-PERPETUAL", 5, 932.0, "buy", true);
        // order_exec.get_order_book("BTC-PERPETUAL", 20);

        // order_exec.view_open_orders(); // Use this to identify the order id for modification or cancellation of orders
        // order_exec.cancel_order("ETH-15669113884");
        // order_exec.view_position();
        // order_exec.subscribe("ETH-PERPETUAL", "100ms");
        // order_exec.modify_order("30588855314", 10, 50000);

        websocket_client.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
