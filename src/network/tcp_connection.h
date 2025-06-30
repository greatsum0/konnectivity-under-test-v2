#pragma once

#include "net_common.h"

#include <array>
#include <deque>
#include <iostream>
#include <memory>
#include <vector>
#include <span>

#include "ring_queue.h"

namespace kut::net::tcp::conn {
    class connection : public std::enable_shared_from_this<connection> {
    public:
        enum class owner { server, client };

        connection(const owner connType,
                   boost::asio::io_context &ioContext,
                   boost::asio::ip::tcp::socket socket,
                   std::deque<std::vector<uint8_t> > &qMessagesIn)
            : connType_(connType)
              , ioContext_(ioContext)
              , strand_(boost::asio::make_strand(ioContext))
              , socket_(std::move(socket))
              , qMessagesIn_(qMessagesIn) {
            if (socket_.is_open()) {
                socket_.set_option(boost::asio::ip::tcp::no_delay(true));
            }
        }

        /* ---------- lifecycle -------------------------------------------------- */

        void connect() {
            if (socket_.is_open())
                boost::asio::dispatch(strand_,
                                      [self = shared_from_this()] { self->readPacket(); });
        }

        void connect(const boost::asio::ip::tcp::resolver::results_type &endpoints) {
            boost::asio::async_connect(socket_, endpoints,
                                       boost::asio::bind_executor(
                                           strand_,
                                           [self = shared_from_this()](const std::error_code &ec,
                                                                       const boost::asio::ip::tcp::endpoint &) {
                                               if (!ec) {
                                                   self->socket_.set_option(
                                                       boost::asio::ip::tcp::no_delay(true));
                                                   self->readPacket();
                                               } else {
                                                   std::cerr << "Connect error: " << ec.message() << '\n';
                                                   self->closeSocket();
                                               }
                                           }));
        }

        [[nodiscard]] bool isConnected() const { return socket_.is_open(); }

        void disconnect() {
            boost::asio::dispatch(strand_,
                                  [self = shared_from_this()] {
                                      if (self->socket_.is_open())
                                          self->closeSocket();
                                  });
        }

        /* ---------- public send (move-aware) ----------------------------------- */

        void send(boost::asio::const_buffer buffer) {
            boost::asio::dispatch(strand_,
                                  [self = shared_from_this(), buffer]() mutable {
                                      if (!self->qMessagesOut_.push(buffer)) {
                                          // optional: log / drop / close
                                          return;
                                      }

                                      if (!self->writing_) // write idle?
                                          self->writePacket();
                                  });
        }

        void setDisconnectCallback(std::function<void()> cb) { onDisconnect_ = std::move(cb); }

        using message_cb_t = std::function<void(std::span<const std::byte>)>;

        void setMessageCallback(message_cb_t cb) {
            onMessage_ = std::move(cb);
        }

    private:
        /* ---------- read side --------------------------------------------------- */

        void readPacket() {
            socket_.async_read_some(boost::asio::buffer(readBuffer_),
                                    boost::asio::bind_executor(
                                        strand_,
                                        [self = shared_from_this()](auto ec, std::size_t n) {
                                            if (!ec) {
                                                if (n && self->onMessage_) {
                                                    std::span<const std::byte> view(
                                                        reinterpret_cast<const std::byte *>(self->readBuffer_.data()),
                                                        n);
                                                    self->onMessage_(view); // ← no copy, no post
                                                }
                                                self->readPacket();
                                            } else {
                                                std::cerr << "Read error: " << ec.message() << '\n';
                                                self->closeSocket();
                                            }
                                        }));
        }

        /* ---------- write side (batched) --------------------------------------- */

        void writePacket() {
            writing_ = true;

            std::size_t bytesTotal = 0;
            std::size_t messageCount = 0;
            std::vector<boost::asio::const_buffer> buffers;

            const std::size_t qsize = qMessagesOut_.size();
            buffers.reserve(qsize);

            for (std::size_t i = 0, qs = qMessagesOut_.size(); i < qs; ++i) {
                auto buf = qMessagesOut_[i];
                bytesTotal += boost::asio::buffer_size(buf);
                if (bytesTotal > kBatchBytes && messageCount) break;
                buffers.emplace_back(buf);
                ++messageCount;
            }

            boost::asio::async_write(
                socket_, buffers,
                boost::asio::bind_executor(
                    strand_,
                    [self = shared_from_this(), messageCount](std::error_code ec, std::size_t) {
                        if (ec) {
                            std::cerr << "Write error: " << ec.message() << '\n';
                            self->closeSocket();
                            return;
                        }
                        for (std::size_t i = 0; i < messageCount; ++i)
                            self->qMessagesOut_.pop();

                        if (!self->qMessagesOut_.empty())
                            self->writePacket();
                        else
                            self->writing_ = false;
                    }));
        }

        void closeSocket() {
            if (socket_.is_open())
                socket_.close();
            if (onDisconnect_)
                onDisconnect_();
        }

        /* ---------- data members ----------------------------------------------- */

        static constexpr std::size_t kReadBufferSize = 64 * 1024; // 64 KiB
        static constexpr std::size_t kBatchBytes = 64 * 1024; // 64 KiB

        owner connType_{owner::client};
        boost::asio::io_context &ioContext_;
        boost::asio::strand<boost::asio::io_context::executor_type> strand_;
        boost::asio::ip::tcp::socket socket_;

        std::deque<std::vector<uint8_t> > &qMessagesIn_; // external queue (rx)
        std::array<uint8_t, kReadBufferSize> readBuffer_{}; // fixed read buffer

        static constexpr std::size_t kTxQueueSlots = 1024; // tune for workload
        using tx_buffer_t = boost::asio::const_buffer;
        data::ring_queue::ring_queue<tx_buffer_t, kTxQueueSlots> qMessagesOut_;

        bool writing_{false}; // are we in a write?
        std::function<void()> onDisconnect_;
        message_cb_t onMessage_;
    };
}
