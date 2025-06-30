#pragma once

#include "net_common.h"
#include "tcp_connection.h"

namespace kut::net::tcp::client {
    using work_guard_t =
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

    class client_interface {
    public:
        explicit client_interface(boost::asio::io_context &ioContext)
        : ioContext_(ioContext),
          workGuard_(make_work_guard(ioContext_)) {
        }

        virtual ~client_interface() {
            disconnect();
        }

        bool connect(const std::string &host, const uint16_t port) {
            try {
                boost::asio::ip::tcp::resolver resolver(ioContext_);

                const auto endpoints = resolver.resolve(host,
                                                        std::to_string(port));

                connection_ = std::make_shared<conn::connection>(
                    conn::connection::owner::client,
                    ioContext_,
                    boost::asio::ip::tcp::socket(ioContext_),
                    incoming_);

                connection_->setMessageCallback(
                    [this](std::span<const std::byte> data) {
                        onMessage(data);
                    });
                connection_->setDisconnectCallback(
                    [this] {
                        onDisconnect();
                    });

                connection_->connect(endpoints);

                threadContext_ = std::thread([this]() { ioContext_.run(); });
            } catch (std::exception &e) {
                std::cerr << "Client Exception: " << e.what() << "\n";
                return false;
            }
            return true;
        }

        void disconnect() {
            workGuard_.reset(); // let io.run() finish when idle
            if (connection_ && connection_->isConnected())
                connection_->disconnect();

            ioContext_.stop();
            if (threadContext_.joinable())
                threadContext_.join();

            connection_.reset();
        }

        [[nodiscard]] bool isConnected() const {
            return connection_ && connection_->isConnected();
        }

        void send(const boost::asio::const_buffer msg) const {
            if (isConnected())
                connection_->send(msg);
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
