#pragma once
// header‑only, simplified implementation of the original control_channel
// All public names, member functions and behaviour are preserved.
// Only duplication between the .h/.cpp pair has been collapsed and
// everything is now defined inline.

#include "i_tcp_client.h"
#include "i_tcp_server.h"

#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <thread>

namespace kut::control {
    // ───────────────────── enums/structs ─────────────────────

    enum class Msg { Context, Ok, Done, Bye, Unknown, Start };

    struct Packet {
        Msg type;
        std::string payload;
    };

    using Handler = std::function<void(const Packet &)>;

    // ───────────────────── implementation helpers ─────────────────────

    namespace detail {
        // ───────── token ↔ Msg ─────────
        constexpr std::string_view to_token(Msg m) {
            using namespace std::string_view_literals;
            switch (m) {
                case Msg::Ok: return "OK"sv;
                case Msg::Done: return "DONE"sv;
                case Msg::Bye: return "BYE"sv;
                case Msg::Start: return "START"sv;
                case Msg::Context: return "CONTEXT"sv;
                default: return ""sv;
            }
        }

        constexpr Msg parse_token(std::string_view t) {
            if (t == "OK") return Msg::Ok;
            if (t == "DONE") return Msg::Done;
            if (t == "BYE") return Msg::Bye;
            if (t == "START") return Msg::Start;
            if (t == "CONTEXT") return Msg::Context;
            return Msg::Unknown;
        }

        // ───────── sender side ─────────
        template<typename Owner>
        inline void send_literal(Owner &self, Msg m) {
            std::string header = std::string(to_token(m)) + " 0\n";
            self.send(boost::asio::buffer(header));
        }

        template<typename Owner>
        inline void send_with_payload(Owner &self,
                                      Msg m,
                                      std::string_view body) {
            auto frame = std::make_shared<std::string>();
            frame->reserve(detail::to_token(m).size() + 1 + 20 + 1 + body.size());

            frame->append(detail::to_token(m));
            frame->push_back(' ');
            frame->append(std::to_string(body.size()));
            frame->push_back(' ');
            frame->append(body);

            /*  Kuyrukta tut; yazı tamamlanınca yaşam süresi garanti */
            self.tx_keep_alive_.push_back(frame);
            self.send(boost::asio::buffer(*frame));
        }

        // ───────── receiver side ───────
        struct Decoder {
            std::string buf; // henüz işlenmemiş baytlar
            std::size_t want = 0; // >0 ise gövdenin kalan byte sayısı
            Msg cur = Msg::Unknown;

            template<typename Dispatch>
            void feed(std::span<const std::byte> data, Dispatch &&out) {
                buf.append(reinterpret_cast<const char *>(data.data()), data.size());

                while (true) {
                    /* 1) Gövde bekliyorsak … */
                    if (want) {
                        if (buf.size() < want) return; // eksik
                        std::string body = buf.substr(0, want);
                        buf.erase(0, want);
                        out(Packet{cur, std::move(body)});
                        cur = Msg::Unknown;
                        want = 0;
                        continue; // başka frame olabilir
                    }

                    /* 2) Başlık bekliyorsak … */
                    if (buf.empty()) return;

                    /* a) TOKEN + '\n'  → payload yok */
                    if (auto nl = buf.find('\n'); nl == 0) {
                        buf.erase(0, 1);
                        continue;
                    } else if (nl != std::string::npos && nl < buf.find(' ')) {
                        std::string token = buf.substr(0, nl);
                        buf.erase(0, nl + 1);
                        out(Packet{parse_token(token), {}});
                        continue;
                    }

                    /* b) TOKEN SP LEN SP … */
                    auto sp1 = buf.find(' ');
                    if (sp1 == std::string::npos) return; // header henüz tam değil

                    auto sp2 = buf.find(' ', sp1 + 1);
                    if (sp2 == std::string::npos) return; // len tamamlanmadı

                    std::string token = buf.substr(0, sp1);
                    std::size_t len = std::stoul(buf.substr(sp1 + 1, sp2 - sp1 - 1));

                    cur = parse_token(token);
                    want = len;
                    buf.erase(0, sp2 + 1);

                    if (want == 0) {
                        // aslında gövde yok
                        out(Packet{cur, {}});
                        cur = Msg::Unknown;
                        continue;
                    }

                    /* Döngü tekrar dönecek ve 1) dalında gövdeyi bekleyecek */
                }
            }
        };
    } // namespace detail

    // ───────────────────── ControlClient ─────────────────────

    class ControlClient final : public net::tcp::client::client_interface {
    public:
        explicit ControlClient(boost::asio::io_context &ioContext, Handler cb)
            : client_interface(ioContext), handler_(std::move(cb)) {
        }

        void connectAndRun(const std::string &host, uint16_t port) {
            if (!connect(host, port)) return;
            while (!isConnected()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        void disconnect() { client_interface::disconnect(); }
        void sendCommand(Msg m) { detail::send_literal(*this, m); }
        void sendCommand(Msg m, std::string_view payload) { detail::send_with_payload(*this, m, payload); }

        std::deque<std::shared_ptr<std::string>> tx_keep_alive_;
    private:
        void onMessage(std::span<const std::byte> msg) override {
            dec_.feed(msg, [this](const control::Packet &p) { handler_(p); });
        }

        detail::Decoder dec_;
        std::string rxBuf_;
        Handler handler_;

    };

    // ───────────────────── ControlServer ─────────────────────

    class ControlServer final : public net::tcp::server::server_interface {
    public:
        explicit ControlServer(Handler cb, boost::asio::io_context &ioContext, uint16_t port)
            : server_interface(ioContext, port), handler_(std::move(cb)) {
        }

        void sendCommand(Msg m) { detail::send_literal(*this, m); }
        void sendCommand(Msg m, std::string_view payload) { detail::send_with_payload(*this, m, payload); }

        std::deque<std::shared_ptr<std::string>> tx_keep_alive_;
    private:
        bool OnClientConnect() override { return true; }
        void OnClientDisconnect() override { rxBuf_.clear(); }


        void OnMessage(std::span<const std::byte> msg) override {
            dec_.feed(msg, [this](const control::Packet &p) { handler_(p); });
        }

        detail::Decoder dec_;
        std::string rxBuf_;
        Handler handler_;

    };
} // namespace kut::control
