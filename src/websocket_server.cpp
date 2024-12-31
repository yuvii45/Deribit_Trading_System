#include "websocket_server.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <cpprest/json.h>
#include <iostream>
#include <spdlog/spdlog.h>

// Include a client for communicating with Deribit
#include "websocket_client.h"

WebSocketServer::WebSocketServer() : deribit_client_("wss://test.deribit.com/ws/api/v2")
{
    m_server.init_asio();

    m_server.set_open_handler(websocketpp::lib::bind(
        &WebSocketServer::on_open, this, websocketpp::lib::placeholders::_1));

    m_server.set_close_handler(websocketpp::lib::bind(
        &WebSocketServer::on_close, this, websocketpp::lib::placeholders::_1));

    m_server.set_message_handler(websocketpp::lib::bind(
        &WebSocketServer::on_message, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));

    // Connect to Deribit on startup
    try
    {
        deribit_client_.connect();
        spdlog::info("Connected to Deribit.");
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error connecting to Deribit: {}", e.what());
    }
}

void WebSocketServer::run(uint16_t port)
{
    try
    {
        m_server.listen(port);
        m_server.start_accept();
        m_server.run();
    }
    catch (websocketpp::exception const &e)
    {
        spdlog::error("Error running WebSocket server: {}", e.what());
    }
}

void WebSocketServer::on_open(websocketpp::connection_hdl hdl)
{
    std::cout << "New connection opened!" << std::endl;
    connections_.insert(hdl);
}

void WebSocketServer::on_close(websocketpp::connection_hdl hdl)
{
    std::cout << "Connection closed!" << std::endl;
    connections_.erase(hdl);
}

void WebSocketServer::on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
    try
    {
        auto payload = msg->get_payload();
        spdlog::info("Received message from client: {}", payload);

        // Parse the incoming message
        auto json_message = web::json::value::parse(payload);

        if (json_message[U("action")].as_string() == "subscribe")
        {
            // Extract the instrument name
            std::string instrument = json_message[U("instrument")].as_string();
            spdlog::info("Subscription request for instrument: {}", instrument);

            // Build the Deribit channel name
            std::string channel_name = "book." + instrument + ".none.10.100ms";
            web::json::value channels = web::json::value::array();
            channels[0] = web::json::value::string(U(channel_name));

            // Create the Deribit subscription request
            web::json::value deribit_request = web::json::value::object();
            deribit_request[U("jsonrpc")] = web::json::value::string(U("2.0"));
            deribit_request[U("id")] = web::json::value::number(4);
            deribit_request[U("method")] = web::json::value::string(U("public/subscribe"));
            deribit_request[U("params")] = web::json::value::object({
                {U("channels"), channels},
            });

            // Send subscription request to Deribit
            deribit_client_.send_message(deribit_request);
            spdlog::info("Sent subscription request to Deribit for channel: {}", channel_name);

            // Forward updates from Deribit to the client asynchronously
            std::thread([this, hdl]()
                        {
                try
                {
                    while (true)
                    {
                        web::json::value update;
                        deribit_client_.receive_message([&update](const web::json::value &msg)
                                                        { update = msg; });

                        spdlog::info("Received update from Deribit: {}", update.serialize());

                        // Forward the update to the WebSocket client
                        std::lock_guard<std::mutex> lock(mutex_); // Ensure thread safety
                        m_server.send(hdl, update.serialize(), websocketpp::frame::opcode::text);
                    }
                }
                catch (const std::exception &e)
                {
                    spdlog::error("Error while forwarding updates to client: {}", e.what());
                } })
                .detach(); // Detach thread to handle updates independently
        }
        else
        {
            spdlog::warn("Unsupported action: {}", json_message[U("action")].as_string());
            m_server.send(hdl, "Unsupported action", websocketpp::frame::opcode::text);
        }
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error processing message: {}", e.what());
        m_server.send(hdl, "Error processing your request", websocketpp::frame::opcode::text);
    }
}

int main()
{
    WebSocketServer server;
    uint16_t port = 9002; // You can choose any port
    std::cout << "Starting WebSocket server on port " << port << "..." << std::endl;
    server.run(port);

    return 0;
}
