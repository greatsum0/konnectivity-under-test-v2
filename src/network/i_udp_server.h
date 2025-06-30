#pragma once

#include "net_common.h"
#include "udp_connection.h"          // the class we built earlier

namespace kut::net::udp::server {
    using work_guard_t =
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

    class server_interface {
    public:
        explicit server_interface(boost::asio::io_context& ioContext, uint16_t port)
            : ioContext_(ioContext),
              socket_(ioContext_,
                      boost::asio::ip::udp::endpoint(
                          boost::asio::ip::udp::v4(), port)),
              workGuard_(boost::asio::make_work_guard(ioContext_)) {
        }

        virtual ~server_interface() = default;

        bool start() {
            try {
                waitForClientConnection(); // bind + RX loop
                threadContext_ = std::thread([this] {
                    ioContext_.run();
                });
            } catch (const std::exception &e) {
                std::cerr << "Server start error: " << e.what() << '\n';
                return false;
            }
            return true;
        }

        void stop() {
            workGuard_.reset();
            ioContext_.stop();
            // if (threadContext_.joinable())
            //     threadContext_.join();
        }

        void join() {
            if (threadContext_.joinable())
                threadContext_.join();
        }

        /*────────────────── send to active client ─────────────────────────*/
        void send(boost::asio::const_buffer msg) const {
            if (connection_ && connection_->isConnected())
                connection_->send(msg); // uses defaultEndpoint_
        }

    protected:
        /* same virtual hooks as TCP version */
        virtual bool OnClientConnect() { return false; }

        virtual void OnClientDisconnect() {
        }

        virtual void OnMessage(std::span<const std::byte>) {
        }

        /* identical member names so derived classes compile unchanged */
        std::shared_ptr<kut::net::udp::conn::connection> connection_;
        boost::asio::io_context &ioContext_;
        std::thread threadContext_;
        boost::asio::ip::udp::socket socket_;

    private:
        std::deque<std::vector<uint8_t> > incoming_;
        work_guard_t workGuard_;

        /*────────────────── accept-equivalent for UDP ─────────────────────*/
        void waitForClientConnection() {
            /* create a connection wrapper around the already-bound socket */
            connection_ = std::make_shared<kut::net::udp::conn::connection>(
                kut::net::udp::conn::connection::owner::server,
                ioContext_, std::move(socket_), incoming_);

            /* callback fired for every datagram */
            connection_->setMessageCallback(
                [this](std::span<const std::byte> data,
                       const boost::asio::ip::udp::endpoint &remote) {
                    /* first packet establishes “the client” */
                    if (!clientSeen_) {
                        clientSeen_ = true;
                        connection_->send(boost::asio::const_buffer{}, remote); // set default
                        if (!OnClientConnect()) {
                            // user veto
                            clientSeen_ = false;
                            return;
                        }
                    }
                    OnMessage(data);
                });

            connection_->setDisconnectCallback([this] {
                OnClientDisconnect();
                clientSeen_ = false;
            });

            connection_->connect(); // start async_receive loop
        }

        bool clientSeen_{false}; // single-client policy
    };
}
