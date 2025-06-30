#pragma once

#include "net_common.h"
#include "tcp_connection.h"

namespace kut::net::tcp::server {
    using work_guard_t = boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>;

    class server_interface {
    public:
        explicit server_interface(boost::asio::io_context& ioContext, uint16_t port)
            : ioContext_(ioContext),
              acceptor_(ioContext_,
                        boost::asio::ip::tcp::endpoint(
                            boost::asio::ip::tcp::v4(), port)),
              workGuard_(make_work_guard(ioContext_)) {
        }

        virtual ~server_interface() = default;
        virtual void setDoneCallback(std::function<void()>) {}
        bool start() {
            try {
                waitForClientConnection();
                threadContext_ = std::thread([this]() {
                    ioContext_.run();
                });
            } catch (std::exception &exception) {
                std::cerr << "Server start error: " << exception.what() << std::endl;
                return false;
            }
            return true;
        }

        void stop() {
            workGuard_.reset(); // allow run() to return once all ops finish
            ioContext_.stop();
            // if (threadContext_.joinable())
            //     threadContext_.join();
        }

        void send(boost::asio::const_buffer msg) const {
            if (connection_->isConnected()) {
                connection_->send(msg);
            }
        }

        void join() {
            if (threadContext_.joinable())
                threadContext_.join();
        }

    protected:
        virtual bool OnClientConnect() {
            return false; // Default implementation
        }

        virtual void OnClientDisconnect() {
            // Default implementation does nothing
        }

        virtual void OnMessage(std::span<const std::byte> msg) {
            // Default implementation does nothing
        }

        std::shared_ptr<conn::connection> connection_;
        boost::asio::io_context &ioContext_;
        std::thread threadContext_;
        boost::asio::ip::tcp::acceptor acceptor_;

    private:
        std::deque<std::vector<uint8_t> > incoming_;
        work_guard_t workGuard_;

        void waitForClientConnection() {
            acceptor_.async_accept(
                [this](std::error_code ec, boost::asio::ip::tcp::socket socket) {
                    if (!ec) {

                        // *** single-client policy ***
                        if (connection_)
                            connection_.reset(); // drop previous client

                        connection_ = std::make_shared<conn::connection>(
                            conn::connection::owner::server,
                            ioContext_, std::move(socket), incoming_);

                        // hook for disconnect notification
                        connection_->setDisconnectCallback(
                            [this] {
                                OnClientDisconnect();
                                connection_.reset();
                                waitForClientConnection();
                            });

                        if (OnClientConnect()) {
                            connection_->connect();
                            connection_->setMessageCallback(
                                [this](const std::span<const std::byte> data) {
                                    OnMessage(data);
                                });
                        } else {
                            std::cerr << "Client rejected.\n";
                            socket.close(); // reject connection
                            connection_.reset();
                            waitForClientConnection(); // wait for next client
                        }
                    } else {
                        std::cerr << "Accept error: " << ec.message() << '\n';
                        waitForClientConnection();
                    }
                });
        }
    };
}
