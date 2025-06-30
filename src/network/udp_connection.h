#pragma once

#include "net_common.h"

#include <array>
#include <deque>
#include <iostream>
#include <memory>
#include <vector>
#include <span>

#include "ring_queue.h"

namespace kut::net::udp::conn {
    class connection : public std::enable_shared_from_this<connection> {
    public:
        enum class owner { server, client };

        connection(owner connType,
                   boost::asio::io_context &ioContext,
                   boost::asio::ip::udp::socket socket,
                   std::deque<std::vector<uint8_t> > &qMessagesIn)
            : connType_(connType)
              , ioContext_(ioContext)
              , strand_(boost::asio::make_strand(ioContext))
              , socket_(std::move(socket))
              , qMessagesIn_(qMessagesIn) {
            /* no UDP socket options to tweak here */
        }

        void connect() {
            if (socket_.is_open())
                boost::asio::dispatch(strand_,
                                      [self = shared_from_this()] { self->readPacket(); });
        }

        // client path: resolve and "connect" (sets default peer)
        void connect(const boost::asio::ip::udp::endpoint &endpoint) {
            socket_.connect(endpoint); // synchronous: no handshake
            defaultEndpoint_ = endpoint;
            connect(); // start RX
        }

        [[nodiscard]] bool isConnected() const { return socket_.is_open(); }

        void disconnect() {
            boost::asio::dispatch(strand_,
                                  [self = shared_from_this()] {
                                      if (self->socket_.is_open())
                                          self->socket_.close();
                                      if (self->onDisconnect_)
                                          self->onDisconnect_();
                                  });
        }


        void send(boost::asio::const_buffer buffer) {
            send(buffer, defaultEndpoint_);
        }

        void send(boost::asio::const_buffer buffer, const boost::asio::ip::udp::endpoint &dst) {
            boost::asio::dispatch(strand_,
                                  [self = shared_from_this(), buffer, dst]() {
                                      tx_item item{buffer, dst};
                                      if (!self->qMessagesOut_.push(std::move(item)))
                                          return; // queue full -> drop/back-pressure
                                      if (!self->writing_)
                                          self->writePacket();
                                  });
        }

        /* ---------- callbacks ---------------------------------------------- */
        void setDisconnectCallback(std::function<void()> cb) { onDisconnect_ = std::move(cb); }

        using message_cb_t =
        std::function<void(std::span<const std::byte>,
                           const boost::asio::ip::udp::endpoint &)>;
        void setMessageCallback(message_cb_t cb) { onMessage_ = std::move(cb); }

    private:
        /* ---------- read side ---------------------------------------------- */
        void readPacket() {
            socket_.async_receive_from(
                boost::asio::buffer(readBuffer_), remoteEndpoint_,
                boost::asio::bind_executor(
                    strand_,
                    [self = shared_from_this()](auto ec, std::size_t n) {
                        if (!ec) {
                            if (n && self->onMessage_) {
                                std::span<const std::byte> view(
                                    reinterpret_cast<const std::byte *>(self->readBuffer_.data()), n);
                                self->onMessage_(view, self->remoteEndpoint_);
                            }
                            self->readPacket(); // keep listening
                        } else if (ec != boost::asio::error::operation_aborted) {
                            std::cerr << "Read error: " << ec.message() << '\n';
                        }
                    }));
        }

        /* ---------- write side --------------------------------------------- */
        void writePacket() {
            writing_ = true;

            const auto &[buf, dst] = qMessagesOut_[0]; // one datagram

            auto handler = [self = shared_from_this()](auto ec, std::size_t) {
                self->qMessagesOut_.pop();
                if (ec && ec != boost::asio::error::operation_aborted)
                    std::cerr << "Write error: " << ec.message() << '\n';

                if (!self->qMessagesOut_.empty())
                    self->writePacket();
                else
                    self->writing_ = false;
            };

            if (connType_ == owner::client)
                socket_.async_send(buf,
                                   boost::asio::bind_executor(strand_, std::move(handler)));
            else
                socket_.async_send_to(buf, dst,
                                      boost::asio::bind_executor(strand_, std::move(handler)));
        }

        /* ---------- data members ------------------------------------------- */
        static constexpr std::size_t kReadBufferSize = 64 * 1024; // 64 KiB
        static constexpr std::size_t kTxQueueSlots = 1024; // tune

        using tx_item = std::pair<boost::asio::const_buffer, boost::asio::ip::udp::endpoint>;

        owner connType_{owner::client};

        boost::asio::io_context &ioContext_;
        boost::asio::strand<
            boost::asio::io_context::executor_type> strand_;
        boost::asio::ip::udp::socket socket_;

        // RX
        std::array<uint8_t, kReadBufferSize> readBuffer_{};
        boost::asio::ip::udp::endpoint remoteEndpoint_;

        // TX
        data::ring_queue::ring_queue<tx_item, kTxQueueSlots> qMessagesOut_;
        bool writing_{false};

        boost::asio::ip::udp::endpoint defaultEndpoint_;

        // external queues / callbacks
        std::deque<std::vector<uint8_t> > &qMessagesIn_;
        message_cb_t onMessage_;
        std::function<void()> onDisconnect_;
    };
}
