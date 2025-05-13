#pragma once

#include <asio.hpp>
#include <client_context.h>
#include <functional>
#include <string>
#include <memory>
#include <deque>

/*
| Function Name         | Side     | Description                                        |
|-----------------------|----------|----------------------------------------------------|
| connect               | Client   | Establishes TCP connection to the server           |
| assignSocket          | Server   | Assigns accepted socket from acceptor              |
| startReadLoop         | Shared   | Starts async loop to receive incoming messages     |
| send                  | Shared   | Sends a message through the socket                 |
| close                 | Shared   | Gracefully shuts down and closes the socket        |
| setMessageCallback    | Shared   | Sets callback to handle incoming messages          |
| setDisconnectCallback | Shared   | Sets callback triggered on remote disconnect       |
| sendScenarioFile      | Client   | Sends ClientContext as YAML to server              |
| doWrite (private)     | Internal | Handles queued messages for async sending          |
*/
class TcpControlChannel : public std::enable_shared_from_this<TcpControlChannel> {
public:
    explicit TcpControlChannel(asio::io_context &io_context);

    // === CLIENT SIDE ===
    void connect(const std::string &host, uint16_t port);

    // === SERVER SIDE ===
    void assignSocket(asio::ip::tcp::socket socket);

    // === COMMON ===
    void startReadLoop(); // reads messages from the socket
    void send(const std::string &msg); // send a message to the socket
    void close(); // close the connection
    bool isOpen() const; // check if socket is currently open
    void setMessageCallback(std::function<void(const std::string &)> cb); // callback for received messages
    void setDisconnectCallback(std::function<void()> cb); // callback for disconnect event
    void sendScenarioFile(const ClientContext &context); // client sends scenario file to server

private:
    void doWrite();

    asio::io_context &io_;
    asio::ip::tcp::socket socket_;
    asio::ip::tcp::resolver resolver_;
    std::string host_;
    std::string portStr_;
    std::function<void(const std::string &)> messageCallback_;
    std::function<void()> disconnectCallback_; // NEW

    std::deque<std::string> writeQueue_;
};
