#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>
#include "websocket_client.h"

class WebSocketServer
{
public:
    WebSocketServer();
    void run(uint16_t port);

private:
    typedef websocketpp::server<websocketpp::config::asio> server;

    server m_server;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections_;
    WebSocketClient deribit_client_;
    std::mutex mutex_; // Mutex for thread-safe operations

    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);
};

#endif // WEBSOCKET_SERVER_H
