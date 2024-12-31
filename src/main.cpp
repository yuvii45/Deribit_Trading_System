#include "order_execution.h"
#include "websocket_client.h"
#include <iostream>
#include <spdlog/spdlog.h>

const std::string API_KEY = "wOAbyf0v";
const std::string API_SECRET = "33VzZ9W4mi4Yx8Hhqy_4AjJfYrlhFfAFtELvHEXY63Q";

const std::string DERIBIT_SERVER_URL = "wss://test.deribit.com/ws/api/v2";
const std::string LOCAL_SERVER_URL = "ws://127.0.0.1:9002";

std::string authenticate(const std::string &api_key, const std::string &api_secret, WebSocketClient &client)
{
    try
    {
        // Build the authentication message
        web::json::value auth_message = web::json::value::object();
        auth_message[U("jsonrpc")] = web::json::value::string(U("2.0"));
        auth_message[U("id")] = web::json::value::number(1);
        auth_message[U("method")] = web::json::value::string(U("public/auth"));
        auth_message[U("params")] = web::json::value::object({{U("grant_type"), web::json::value::string(U("client_credentials"))},
                                                              {U("client_id"), web::json::value::string(U(api_key))},
                                                              {U("client_secret"), web::json::value::string(U(api_secret))}});

        client.send_message(auth_message);

        web::json::value response;
        client.receive_message([&response](const web::json::value &msg)
                               { response = msg; });

        if (response.has_field("error"))
        {
            spdlog::error("Authentication failed. Response error: {}", response.serialize());
            throw std::runtime_error("Authentication failed. Check response for details.");
        }

        std::string access_token_ = response[U("result")][U("access_token")].as_string();

        return access_token_;
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error during authentication: {}", e.what());
        throw std::runtime_error("Error during authentication.");
    }
}

int main()
{
    try
    {
        WebSocketClient deribit_client(DERIBIT_SERVER_URL);
        deribit_client.connect();

        WebSocketClient local_client(LOCAL_SERVER_URL);
        local_client.connect();

        // Authenticate and obtain access token
        std::string access_token = authenticate(API_KEY, API_SECRET, deribit_client);
        spdlog::info("Authentication Successful.");

        // Initialize OrderExecution object
        OrderExecution order_exec(API_KEY, API_SECRET, access_token, deribit_client, local_client);

        int choice;
        do
        {
            std::cout << "\nSelect an action:\n";
            std::cout << "1. Place Order\n";
            std::cout << "2. View Open Orders\n";
            std::cout << "3. Modify Order\n";
            std::cout << "4. Cancel Order\n";
            std::cout << "5. Get Order Book\n";
            std::cout << "6. Subscribe to Channel\n";
            std::cout << "7. Exit\n";
            std::cout << "Enter your choice: ";
            std::cin >> choice;

            switch (choice)
            {
            case 1:
            {
                // Place Order
                std::string instrument;
                double amount, price;
                std::string side;
                bool market;

                std::cout << "Enter Instrument (e.g., ETH-PERPETUAL): ";
                std::cin >> instrument;
                std::cout << "Enter Amount: ";
                std::cin >> amount;
                std::cout << "Enter Price: (Enter random value if you wish to order at market price) ";
                std::cin >> price;
                std::cout << "Enter Side (buy/sell): ";
                std::cin >> side;
                std::cout << "Sell at market price (1 for true, 0 for false): ";
                std::cin >> market;

                order_exec.place_order(instrument, amount, price, side, market);
                break;
            }
            case 2:
            {
                // View Open Orders
                order_exec.view_open_orders();
                break;
            }
            case 3:
            {
                // Modify Order
                std::string order_id;
                double new_amount, new_price;

                std::cout << "Enter Order ID: ";
                std::cin >> order_id;
                std::cout << "Enter New Amount: ";
                std::cin >> new_amount;
                std::cout << "Enter New Price: ";
                std::cin >> new_price;

                order_exec.modify_order(order_id, new_amount, new_price);
                break;
            }
            case 4:
            {
                // Cancel Order
                std::string order_id;
                std::cout << "Enter Order ID to Cancel: ";
                std::cin >> order_id;

                order_exec.cancel_order(order_id);
                break;
            }
            case 5:
            {
                // Get Order Book
                std::string instrument;
                int depth;

                std::cout << "Enter Instrument (e.g., ETH-PERPETUAL): ";
                std::cin >> instrument;
                std::cout << "Enter Depth: ";
                std::cin >> depth;

                order_exec.get_order_book(instrument, depth);
                break;
            }
            case 6:
            {
                // Subscribe to Channel
                std::string instrument;

                std::cout << "Enter Instrument (e.g., ETH-PERPETUAL): ";
                std::cin >> instrument;

                order_exec.subscribe(instrument);
                break;
            }
            case 7:
                // Exit
                std::cout << "Exiting the platform...\n";
                break;
            default:
                std::cout << "Invalid choice. Please try again.\n";
                break;
            }
        } while (choice != 7);

        deribit_client.close();
        local_client.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
