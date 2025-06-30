#pragma once

#include "net_common.h"
#include "udp_connection.h"

namespace kut::net::udp::client {
    using work_guard_t = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

    class client_interface {
    public:
        explicit client_interface(boost::asio::io_context &ioContext)
        : ioContext_(ioContext),
          workGuard_(make_work_guard(ioContext_)) {
        }

        explicit client_interface(const boost::asio::io_context & ioContext);

        virtual ~client_interface() {
            disconnect();
        }

        bool connect(const std::string &host, uint16_t port) {
            try {
                boost::asio::ip::udp::resolver resolver(ioContext_);
                auto endpoints = resolver.resolve(host, std::to_string(port));

                connection_ = std::make_shared<kut::net::udp::conn::connection>(
                    kut::net::udp::conn::connection::owner::client,
                    ioContext_,
                    boost::asio::ip::udp::socket(ioContext_), // unopened socket
                    incoming_);

                /* user callbacks */
                connection_->setMessageCallback(
                    [this](std::span<const std::byte> data,
                           const boost::asio::ip::udp::endpoint &) {
                        onMessage(data);
                    });
                connection_->setDisconnectCallback([this] { onDisconnect(); });

                /* pick the first resolved endpoint */
                connection_->connect(endpoints.begin()->endpoint());

                threadContext_ = std::thread(
                    [this] { ioContext_.run(); }); // I/O thread
            } catch (const std::exception &e) {
                std::cerr << "Client Exception: " << e.what() << '\n';
                return false;
            }
            return true;
        }

        void disconnect() {
            workGuard_.reset();
            if (connection_ && connection_->isConnected())
                connection_->disconnect();

            ioContext_.stop();
            if (threadContext_.joinable())
                threadContext_.join();
            connection_.reset();
        }

        [[nodiscard]] bool isConnected() const { return connection_ && connection_->isConnected(); }

        void send(boost::asio::const_buffer msg) const {
            if (isConnected())
                connection_->send(msg); // zero-copy UDP send
        }


    protected:
        virtual void onMessage(std::span<const std::byte>) {
        }

        virtual void onDisconnect() {
        }

        boost::asio::io_context &ioContext_;
        work_guard_t workGuard_;
        std::thread threadContext_;
        std::shared_ptr<conn::connection> connection_;

    private:
        std::deque<std::vector<uint8_t> > incoming_;
    };
}
