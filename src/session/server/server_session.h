#pragma once
#include <atomic>
#include <chrono>
#include <thread>
#include <logger.h>
#include <context.h>
#include <fstream>
#include <session_common.h>
#include <tcp_server.h>
#include <udp_server.h>
#include <yaml_parser.h>
#include <filesystem>

#include "control_channel.h"

namespace kut::session::server {
    /* ──────────────────────────────────────────────────────────────────── */
    /*  One run of the server: handles handshake, traffic, shutdown        */
    /* ──────────────────────────────────────────────────────────────────── */
    template<typename ServerT>
    void runListen(context::server::Context &context,
                   const common::Protocol protocol) {
        auto ctrlCtx = std::make_shared<boost::asio::io_context>();
        auto trafficCtx = std::make_shared<boost::asio::io_context>();

        std::unique_ptr<context::client::Context> clientContext;

        /* 2) Synchronisation primitives – FRESH every time we restart */
        std::promise<void> contextReceivedPromise;
        std::future<void> contextReceivedFuture = contextReceivedPromise.get_future();

        std::promise<void> scenarioDonePromise;
        std::future<void> scenarioDoneFuture = scenarioDonePromise.get_future();

        std::atomic_bool doneFlag{false}; // first event wins

        /* 3) Control channel callback */
        const control::Handler controlCallback = [&](const control::Packet &p) {
            switch (p.type) {
                case control::Msg::Context:
                    clientContext = std::make_unique<context::client::Context>(
                        context::io::YamlReader::loadClientContextFromString(p.payload));
                    contextReceivedPromise.set_value();
                    break;

                case control::Msg::Done:
                    Logger::instance().info("Received DONE, stopping server");
                    if (!doneFlag.exchange(true))
                        scenarioDonePromise.set_value();
                    break;

                default:
                    std::cout << "Unknown control message: "
                            << static_cast<int>(p.type) << '\n';
            }
        };

        /* 4) Fire up the control server */
        const int controlPort = context.network().endpoint().controlPort();
        control::ControlServer controlServer(controlCallback, *ctrlCtx, controlPort);
        controlServer.start();

        std::thread controlThread([&controlServer] {
            try { controlServer.join(); } catch (const std::exception &e) {
                Logger::instance().error("Control server error: ", e.what());
            }
        });

        Logger::instance().info("Control server started on port ", controlPort);

        /* 5) Telemetry helpers */
        measurement::network::PacketSniffer sniffer(
            context.measurement().interface(),
            context.network().endpoint().port());

        measurement::cpu::CPUUsageMonitor cpuMonitor(
            context.measurement().interval_sec());

        /* 6) Wait for CONTEXT from client, then start everything */
        contextReceivedFuture.wait();
        controlServer.sendCommand(control::Msg::Start, "");

        const auto t0 = measurement::clock::now();
        cpuMonitor.start();
        sniffer.start();

        /* 7) Traffic server (TCP or UDP) */
        ServerT server(*trafficCtx, context.network().endpoint().port());

        /*    Notify runListen() if the client drops unexpectedly           */
        server.setDoneCallback([&] {
            if (!doneFlag.exchange(true))
                scenarioDonePromise.set_value();
        });

        server.start();
        Logger::instance().info("Server listening on port ",
                                context.network().endpoint().port());

        /* 8) Block until either DONE or disconnect */
        scenarioDoneFuture.wait();

        /* 9) Graceful shutdown */
        Logger::instance().info("Stopping server…");
        server.stop();
        server.join();
        Logger::instance().info("Server stopped");

        sniffer.stop();
        Logger::instance().info("Sniffer stopped, collected frames ",
                                sniffer.peekCount());

        cpuMonitor.stop();
        Logger::instance().info("CPU monitor stopped");

        controlServer.stop();
        controlThread.join();
        Logger::instance().info("Control server stopped");

        /* 10) Metric merging + CSV dump */
        std::vector<measurement::cpu::Record> cpuRecs = cpuMonitor.consume();
        std::vector<measurement::Metric> cpuFlat;
        for (auto &r: cpuRecs) {
            auto v = r.flatten(t0);
            cpuFlat.insert(cpuFlat.end(),
                           std::make_move_iterator(v.begin()),
                           std::make_move_iterator(v.end()));
        }

        std::vector<measurement::Metric> netMetrics = sniffer.consume();
        std::vector<measurement::Metric> all;
        all.reserve(netMetrics.size() + cpuFlat.size());
        std::move(netMetrics.begin(), netMetrics.end(), std::back_inserter(all));
        std::move(cpuFlat.begin(), cpuFlat.end(), std::back_inserter(all));

        const auto result = common::compute(std::move(all));

        std::filesystem::create_directories(context.measurement().output_path());

        const auto now = std::chrono::time_point_cast<std::chrono::seconds>(t0);
        const auto nowStr = std::to_string(now.time_since_epoch().count());

        const std::string outputFile = context.measurement().output_path() + '/'
                                       + nowStr + '_' + clientContext->meta().name() + ".csv";

        std::ofstream out(outputFile);
        Logger::instance().info("Writing metrics to", outputFile);
        write_csv(result, out);
    }

    class ServerSession {
    public:
        explicit ServerSession(context::server::Context &context)
            : context_(context) {
        }

        void run() const {
            const auto protocol = common::toProtocol(context_.network().protocol());

            for (;;) {
                // <── automatic restart loop
                try {
                    switch (protocol) {
                        case common::Protocol::Udp:
                            runListen<UDPServer>(context_, protocol);
                            break;
                        case common::Protocol::Tcp:
                            runListen<TCPServer>(context_, protocol);
                            break;
                        default:
                            throw std::runtime_error("Unsupported protocol: " + context_.network().protocol());
                    }
                } catch (const std::exception &e) {
                    Logger::instance().error("Session crashed: ", e.what());
                }
                Logger::instance().info("Session ended – restarting …");
            }
        }

    private:
        context::server::Context &context_;
    };
}
