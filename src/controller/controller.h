// controller.hpp
#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <string>
#include <tcp_client.h>
#include <tcp_server.h>

class Controller : public std::enable_shared_from_this<Controller> {
public:
    enum class Mode { Server, Client };

    enum class State { Disconnected, Connecting, Connected };

    using ReceiveHandler = std::function<void(const std::string &)>;

    Controller(boost::asio::io_context &io_context,
               ReceiveHandler handler);

    void startServer(unsigned short port);

    void startClient(const std::string &host, unsigned short port);

    void send(const std::string &message) const;

    void stop();

    State state() const;

private:
    void onReceive(const std::string &message) const;

    void updateState(State new_state);


    boost::asio::io_context &io_context_;
    ReceiveHandler receive_handler_;
    Mode mode_;
    State state_;
    std::shared_ptr<TcpServer> server_;
    std::shared_ptr<TcpClient> client_;
};
