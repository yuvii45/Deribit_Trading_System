#include "order_execution.h"
#include <sstream>
#include <iomanip>

OrderExecution::OrderExecution(const std::string &api_key, const std::string &api_secret, const std::string &acess_token, WebSocketClient &client)
    : api_key_(api_key), api_secret_(api_secret), client_(client), access_token_(access_token_) {}

web::json::value OrderExecution::send_and_receive_request(const std::string &request)
{
    web::json::value message = web::json::value::parse(request);
    web::json::value response;
    client_.send_message(message);
    client_.receive_message([&response](const web::json::value &msg)
                            { response = msg; });
    return response;
}

std::string OrderExecution::create_signed_request(const std::string &params, const std::string &request_type)
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

    std::cout << "Created signed request." << std::endl;

    return request;
}

web::json::value OrderExecution::get_balance(const std::string &currency)
{
    const std::string request_type = "private/get_account_summary";
    std::cout << "Fetching balance for currency: " << currency << std::endl;

    std::string params = "\"currency\": \"" + currency + "\"";
    std::string request = create_signed_request(params, request_type);
    web::json::value response = send_and_receive_request(request);
    std::cout << response.serialize() << std::endl;

    return response;
}

void OrderExecution::place_order(const std::string &instrument_name, double amount, double price, const std::string &order_type, bool market)
{
    const std::string request_type = "private/" + order_type;
    std::string params;

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

    std::string request = create_signed_request(params, request_type);
    web::json::value response = send_and_receive_request(request);

    std::cout << "Order Placed Successfully.\n"
              << response.serialize() << std::endl;

    return;
}

void OrderExecution::cancel_order(const std::string &order_id)
{
    const std::string request_type = "private/cancel";

    std::string params = "\"order_id\": \"" + order_id + "\"";
    std::string request = create_signed_request(params, request_type);

    web::json::value response = send_and_receive_request(request);
    std::cout << "Order Cancelled Successfully.\n"
              << response.serialize() << std::endl;

    return;
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

    client_.send_message(message);

    web::json::value response;
    client_.receive_message([&response](const web::json::value &msg)
                            { response = msg; });

    std::cout << "Got the order book:\n"
              << response.serialize() << std::endl;
}

void OrderExecution::view_open_orders()
{
    const std::string request_type = "private/get_open_orders";

    std::string request = create_signed_request("", request_type);
    web::json::value response = send_and_receive_request(request);

    std::cout << "View Open Orders:\n"
              << response.serialize() << std::endl;

    return;
}

void OrderExecution::view_position()
{
    const std::string request_type = "private/get_positions";

    std::string request = create_signed_request("", request_type);

    web::json::value response = send_and_receive_request(request);
    std::cout << "View Current Position:\n"
              << response.serialize() << std::endl;

    return;
}

void OrderExecution::modify_order(const std::string &order_id, int amount, double price)
{
    const std::string request_type = "private/edit";

    std::string params = "\"order_id\": \"" + order_id + "\", " +
                         "\"amount\": " + std::to_string(amount) + ", " +
                         "\"price\": \"" + std::to_string(price) + "\"";

    std::string request = create_signed_request(params, request_type);

    web::json::value response = send_and_receive_request(request);

    std::cout << "Edited the given order:\n"
              << response.serialize() << std::endl;

    return;
}

void OrderExecution::subscribe(const std::string &instrument_name,
                               const std::string &interval)
{
    const std::string request_type = "public/subscribe";
    // ADD MULTIPLE CHANNELS HERE?
    const std::string group = "none";
    int depth = 10;
    std::string channel_name = "book." + instrument_name + "." + group + "." +
                               std::to_string(depth) + "." + interval;

    std::cout << "Channel Name: " << channel_name << std::endl;

    web::json::value channels = web::json::value::array();
    channels[0] = web::json::value::string(U(channel_name));

    web::json::value message = web::json::value::object();
    message[U("jsonrpc")] = web::json::value::string(U("2.0"));
    message[U("id")] = web::json::value::number(4);
    message[U("method")] = web::json::value::string(U(request_type));
    message[U("params")] = web::json::value::object({
        {U("channels"), channels},
    });

    client_.send_message(message);

    while (true)
    {
        try
        {
            web::json::value notification;
            client_.receive_message([&notification](const web::json::value &msg)
                                    { notification = msg; });

            std::cout << "Notification received: " << notification.serialize() << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error while receiving notification: " << e.what() << std::endl;
            break;
        }
    }
}
