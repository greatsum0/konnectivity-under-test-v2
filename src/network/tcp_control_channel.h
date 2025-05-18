#pragma once

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <client_context.h>
#include <functional>
#include <string>
#include <memory>
#include <deque>
#include <iostream>   // for logging macros

#define CTRL_LOG_INFO(msg) \
LOG_INFO("[CTRL] " << msg)

#define CTRL_LOG_WARN(msg) \
LOG_WARN("[CTRL] " << msg)

#define CTRL_LOG_ERROR(msg) \
LOG_ERROR("[CTRL] " << msg)

// ————————————————————————————————————————————————————————————————————————————
/**
 * @brief Bidirectional control channel over TCP.
 *
 * Supports both client (connect/sendScenarioFile) and
 * server (assignSocket) roles, plus shared async I/O:
 *
 *  - startReadLoop():               begin async receive
 *  - send(msg):                     queue & async write
 *  - close():                       graceful shutdown
 *  - isOpen():                      query socket state
 *  - setMessageCallback(cb)
 *  - setDisconnectCallback(cb)
 *  - sendScenarioFile(context)      client → serialize+send YAML
 */
class TcpControlChannel : public std::enable_shared_from_this<TcpControlChannel> {
public:
    explicit TcpControlChannel(asio::io_context& io_context);

    /// Client side: synchronously connect to host:port
    void connect(const std::string& host, uint16_t port);

    /// Server side: take ownership of an accepted socket
    void assignSocket(asio::ip::tcp::socket socket);

    /// Begin async read loop (calls messageCallback_ / disconnectCallback_)
    void startReadLoop();

    /// Queue up a message for async write
    void send(const std::string& msg);

    /// Gracefully shutdown & close the socket
    void close();

    /// True if the underlying socket is open
    bool isOpen() const;

    /// Called when a full message arrives
    void setMessageCallback(std::function<void(const std::string&)> cb);

    /// Called on remote‐side EOF or error
    void setDisconnectCallback(std::function<void()> cb);

    /// Client helper: serialize & send the ClientContext as YAML
    void sendScenarioFile(const ClientContext& context);

private:
    void doWrite();

    asio::io_context&                        io_;
    asio::ip::tcp::socket                    socket_;
    asio::ip::tcp::resolver                  resolver_;
    std::string                              host_;
    uint16_t                                 port_{0};

    std::function<void(const std::string&)>  messageCallback_;
    std::function<void()>                    disconnectCallback_;

    std::deque<std::string>                  writeQueue_;
};
