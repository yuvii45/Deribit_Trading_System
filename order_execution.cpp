#include "order_execution.h"
#include <sstream>
#include <iomanip>
#include <spdlog/spdlog.h>

OrderExecution::OrderExecution(const std::string &api_key, const std::string &api_secret, const std::string &access_token, WebSocketClient &deribit_client, WebSocketClient &local_client)
    : api_key_(api_key), api_secret_(api_secret), deribit_client_(deribit_client), local_client_(local_client), access_token_(access_token) {}

web::json::value OrderExecution::send_and_receive_request(const std::string &request)
{
    try
    {
        web::json::value message = web::json::value::parse(request);
        web::json::value response;
        deribit_client_.send_message(message);
        deribit_client_.receive_message([&response](const web::json::value &msg)
                                        { response = msg; });

        // Step 5: Check the response
        if (response.has_field("error"))
        {
            spdlog::error("Order placement failed. Response error: {}", response.serialize());
            throw std::runtime_error("Order placement failed. Check response for details.");
        }

        return response;
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error sending or receiving request: {}", e.what());
        throw std::runtime_error("Error sending or receiving request.");
    }
}

std::string OrderExecution::create_signed_request(const std::string &params, const std::string &request_type)
{
    try
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        std::string nonce = std::to_string(duration.count()); // Nonce as the current timestamp in milliseconds

        std::string request_data = "api_key=" + api_key_ + "&nonce=" + nonce + "&params=" + params;

        // Generate the signature using HMAC SHA-256
        unsigned char *result;
        result = HMAC(EVP_sha256(), api_secret_.c_str(), api_secret_.length(), (unsigned char *)request_data.c_str(), request_data.length(), NULL, NULL);

        // Convert the result to a hex string
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)result[i];
        }
        std::string signature = ss.str();

        // Create the JSON request string with the signature and other necessary parameters
        std::string request = "{\"jsonrpc\": \"2.0\", \"method\": \"" + request_type + "\", "
                                                                                       "\"params\": {" +
                              params + "}, \"nonce\": \"" + nonce + "\", "
                                                                    "\"api_key\": \"" +
                              api_key_ + "\", \"signature\": \"" + signature + "\"}";

        spdlog::info("Created signed request.");

        return request;
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error creating signed request: {}", e.what());
        throw std::runtime_error("Error creating signed request.");
    }
}

void OrderExecution::place_order(const std::string &instrument_name, double amount, double price, const std::string &order_type, bool market)
{
    // Step 1: Validate the input parameters
    if (instrument_name.empty())
    {
        spdlog::error("Instrument name cannot be empty.");
        throw std::invalid_argument("Instrument name cannot be empty.");
    }
    else if (amount <= 0)
    {
        spdlog::error("Amount must be greater than zero.");
        throw std::invalid_argument("Amount must be greater than zero.");
    }
    else if (!market && price <= 0)
    {
        spdlog::error("Price must be greater than zero for limit orders.");
        throw std::invalid_argument("Price must be greater than zero for limit orders.");
    }

    const std::string request_type = "private/" + order_type;
    std::string params;

    try
    {
        if (market)
        {
            params = "\"instrument_name\": \"" + instrument_name +
                     "\", \"amount\": " + std::to_string(amount) +
                     ", \"type\": \"market\", "
                     "\"direction\": \"" +
                     order_type + "\"";
        }
        else
        {
            params = "\"instrument_name\": \"" + instrument_name +
                     "\", \"amount\": " + std::to_string(amount) +
                     ", \"type\": \"limit\", \"price\": " + std::to_string(price) +
                     ", \"direction\": \"" + order_type + "\"";
        }
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error constructing request parameters: {}", e.what());
        throw std::runtime_error("Error constructing request parameters.");
    }

    std::string request = create_signed_request(params, request_type);

    spdlog::info("Placing order for instrument: {}, amount: {}, price: {}, order type: {}",
                 instrument_name, amount, price, order_type);

    try
    {
        web::json::value response = send_and_receive_request(request);
        spdlog::info("Order placed successfully. Response: {}", response.serialize());
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error placing order: {}", e.what());
        throw;
    }
}

void OrderExecution::cancel_order(const std::string &order_id)
{
    const std::string request_type = "private/cancel";

    std::string params = "\"order_id\": \"" + order_id + "\"";
    std::string request = create_signed_request(params, request_type);

    try
    {
        web::json::value response = send_and_receive_request(request);
        spdlog::info("Order Cancelled Successfully. Response: {}", response.serialize());
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error cancelling order: {}", e.what());
        throw;
    }
}

void OrderExecution::get_order_book(const std::string &instrument_name, int depth)
{
    const std::string request_type = "public/get_order_book";
    web::json::value message = web::json::value::object();

    message[U("jsonrpc")] = web::json::value::string(U("2.0"));
    message[U("id")] = web::json::value::number(2);
    message[U("method")] = web::json::value::string(U(request_type));
    message[U("params")] = web::json::value::object({{U("instrument_name"), web::json::value::string(U(instrument_name))},
                                                     {U("depth"), web::json::value::number((depth))}});

    try
    {
        deribit_client_.send_message(message);

        web::json::value response;
        deribit_client_.receive_message([&response](const web::json::value &msg)
                                        { response = msg; });

        spdlog::info("Got the order book: {}", response.serialize());
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error getting order book: {}", e.what());
        throw;
    }
}

void OrderExecution::view_open_orders()
{
    const std::string request_type = "private/get_open_orders";

    std::string request = create_signed_request("", request_type);

    try
    {
        web::json::value response = send_and_receive_request(request);
        spdlog::info("View Open Orders: {}", response.serialize());
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error viewing open orders: {}", e.what());
        throw;
    }
}

void OrderExecution::view_position()
{
    const std::string request_type = "private/get_positions";

    std::string request = create_signed_request("", request_type);

    try
    {
        web::json::value response = send_and_receive_request(request);
        spdlog::info("View Current Position: {}", response.serialize());
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error viewing position: {}", e.what());
        throw;
    }
}

void OrderExecution::modify_order(const std::string &order_id, int amount, double price)
{
    const std::string request_type = "private/edit";

    std::string params = "\"order_id\": \"" + order_id + "\", " +
                         "\"amount\": " + std::to_string(amount) + ", " +
                         "\"price\": \"" + std::to_string(price) + "\"";

    std::string request = create_signed_request(params, request_type);

    try
    {
        web::json::value response = send_and_receive_request(request);
        spdlog::info("Edited the given order: {}", response.serialize());
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error modifying order: {}", e.what());
        throw;
    }
}

void OrderExecution::subscribe(const std::string &instrument_name)
{
    spdlog::info("Subscribing to instrument: {}", instrument_name);

    // Build the subscription request for localhost
    web::json::value subscription_request = web::json::value::object();
    subscription_request[U("action")] = web::json::value::string(U("subscribe"));
    subscription_request[U("instrument")] = web::json::value::string(U(instrument_name));

    try
    {
        // Send the subscription request to the localhost server
        local_client_.send_message(subscription_request);

        spdlog::info("Subscription request sent to localhost server.");

        // Receive and log notifications
        while (true)
        {
            try
            {
                web::json::value notification;
                local_client_.receive_message([&notification](const web::json::value &msg)
                                              { notification = msg; });

                spdlog::info("Notification received at orderexec: {}", notification.serialize());
            }
            catch (const std::exception &e)
            {
                spdlog::error("Error while receiving notification: {}", e.what());
                break;
            }
        }
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error subscribing to instrument: {}", e.what());
        throw;
    }
}
