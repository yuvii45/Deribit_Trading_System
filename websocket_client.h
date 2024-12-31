#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <cpprest/ws_client.h>
#include <cpprest/json.h>
#include <string>
#include <functional>

class WebSocketClient
{
public:
    WebSocketClient(const std::string &url);
    ~WebSocketClient();

    void connect();
    void send_message(const web::json::value &message);
    void receive_message(std::function<void(const web::json::value &)> callback);
    void close();

private:
    web::websockets::client::websocket_client client_;
    std::string url_;
    bool is_closed;
};

#endif