#ifndef ORDER_EXECUTION_H
#define ORDER_EXECUTION_H

#include <string>
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include "websocket_client.h"
#include <chrono>

class OrderExecution
{
public:
    OrderExecution(const std::string &api_key, const std::string &api_secret, const std::string &acess_token, WebSocketClient &client);

    web::json::value send_and_receive_request(const std::string &request);

    std::string create_signed_request(const std::string &params, const std::string &request_type);
    web::json::value get_balance(const std::string &currency);
    void place_order(const std::string &instrument_name, double amount, double price, const std::string &order_type, bool market = false);
    void cancel_order(const std::string &order_id);
    void view_open_orders();
    void view_position();
    void modify_order(const std::string &order_id, int amount, double price);
    void subscribe(const std::string &instrument_name, const std::string &interval);
    void get_order_book(const std::string &instrument_name, int depth = 10);

private:
    WebSocketClient &client_;
    std::string api_key_;
    std::string api_secret_;
    std::string access_token_;
};

#endif