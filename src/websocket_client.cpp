#include "websocket_client.h"
#include <iostream>

WebSocketClient::WebSocketClient(const std::string &url) : url_(url), is_closed(false)
{
}

WebSocketClient::~WebSocketClient()
{
    try
    {
        if (!is_closed)
        {
            close();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error during WebSocket cleanup: " << e.what() << std::endl;
    }
}

void WebSocketClient::connect()
{
    client_.connect(web::uri(url_)).wait();
    std::cout << "WebSocket connected to " << url_ << std::endl;
}

void WebSocketClient::send_message(const web::json::value &message)
{
    web::websockets::client::websocket_outgoing_message outgoing_msg;

    // Set the message payload using the serialized JSON string
    outgoing_msg.set_utf8_message(message.serialize());
    client_.send(outgoing_msg).wait();

    std::cout << "Message sent. " << std::endl;
}

void WebSocketClient::receive_message(std::function<void(const web::json::value &)> callback)
{
    client_.receive().then([=](web::websockets::client::websocket_incoming_message incoming_message)
                           {
        auto msg = incoming_message.extract_string().get();
        web::json::value json_message = web::json::value::parse(msg);
        callback(json_message); })
        .wait();

    std::cout << "Message Received. " << std::endl;
}

void WebSocketClient::close()
{
    client_.close().wait();
    std::cout << "WebSocket connection closed." << std::endl;
    is_closed = true;
}