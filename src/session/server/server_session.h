#pragma once
#include <chrono>
#include <thread>
#include <logger.h>
#include <context.h>
#include <fstream>
#include <session_common.h>
#include <tcp_server.h>
#include <udp_server.h>
#include <yaml_parser.h>

#include "control_channel.h"


namespace kut::session::server {
    template<typename ServerT>
    void runListen(context::server::Context &context, const common::Protocol protocol) {
        /* ── 2) I/O contexts ───────────────────────────────────────────── */
        auto ctrlCtx = std::make_shared<boost::asio::io_context>();
        auto trafficCtx = std::make_shared<boost::asio::io_context>();

        std::unique_ptr<context::client::Context> clientContext;

        /* ── 3) Control channel (start handshake) ─────────────────────── */
        std::promise<void> contextReceivedPromise, scenarioDonePromise;
        const auto contextReceivedFuture = contextReceivedPromise.get_future();
        const auto scenarioDoneFuture = scenarioDonePromise.get_future();

        const control::Handler controlCallback = [&](const control::Packet &p) {
            switch (p.type) {
                case control::Msg::Context:
                    clientContext = std::make_unique<
                        context::client::Context>(context::io::YamlReader::loadClientContextFromString(p.payload));

                    contextReceivedPromise.set_value();
                    break;
                case control::Msg::Done:
                    Logger::instance().info("Received DONE, stopping server");
                    scenarioDonePromise.set_value();
                    break;
                default:
                    std::cout << "Unknown control message: " << static_cast<int>(p.type) << std::endl;
                    break;
            }
        };

        const int controlPort = context.network().endpoint().controlPort();
        control::ControlServer controlServer(controlCallback, *ctrlCtx, controlPort);
        controlServer.start();
        std::thread controlThread([&controlServer]() {
            try {
                controlServer.join();
            } catch (const std::exception &e) {
                Logger::instance().error("Control server error: " + std::string(e.what()));
            }
        });
        Logger::instance().info("Control server started on port ", controlPort);

        measurement::network::PacketSniffer sniffer(
            context.measurement().interface(),
            context.network().endpoint().port());
        measurement::cpu::CPUUsageMonitor cpuMonitor(context.measurement().interval_sec());

        contextReceivedFuture.wait();
        controlServer.sendCommand(control::Msg::Start, "");

        auto t0 = measurement::clock::now();
        cpuMonitor.start();
        sniffer.start();

        ServerT server(*trafficCtx, context.network().endpoint().port());

        server.start();

        Logger::instance().info("Server listening on port ", context.network().endpoint().port());

        scenarioDoneFuture.wait();
        /* Graceful shutdown */
        Logger::instance().info("Stopping server…");
        server.stop();
        Logger::instance().info("Server stopped");
        server.join();
        sniffer.stop();
        Logger::instance().info("Sniffer stopped, collected ", sniffer.peekCount(), " frames");
        cpuMonitor.stop();
        Logger::instance().info("CPU monitor stopped");
        controlServer.stop();
        Logger::instance().info("Control server stopped");
        controlThread.join();

        /* ── 7) Merge metrics + quick report ──────────────────────────── */
        std::vector<measurement::cpu::Record> cpuRecs = cpuMonitor.consume(); // move-out
        std::vector<measurement::Metric> cpuFlat;
        for (const auto &r: cpuRecs) {
            auto v = r.flatten(t0); // returns 4 metrics
            cpuFlat.insert(cpuFlat.end(),
                           std::make_move_iterator(v.begin()),
                           std::make_move_iterator(v.end()));
        }

        std::vector<measurement::Metric> netMetrics = sniffer.consume(); // move-out
        std::vector<measurement::Metric> all;
        all.reserve(netMetrics.size() + cpuFlat.size());

        std::move(netMetrics.begin(), netMetrics.end(),
                  std::back_inserter(all));
        std::move(cpuFlat.begin(), cpuFlat.end(),
                  std::back_inserter(all));

        auto result = common::compute(std::move(all));

        // make sure path exists
        if (!std::filesystem::exists(context.measurement().output_path())) {
            std::filesystem::create_directories(context.measurement().output_path());
        }

        // now
        const auto now = std::chrono::time_point_cast<std::chrono::seconds>(t0);
        // convert type to string
        std::string nowStr = std::to_string(now.time_since_epoch().count());

        std::string outputFile = context.measurement().output_path() + "/" + nowStr + "_" + clientContext->meta().name()
                                 + ".csv";
        std::ofstream out(outputFile);
        Logger::instance().info("Writing metrics to ", outputFile);
        write_csv(result, out);
    }

    class ServerSession {
    public:
        explicit ServerSession(context::server::Context &context)
            : context_(context) {
        }

        void run() const {
            common::Protocol protocol = common::toProtocol(context_.network().protocol());
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
        }

    private:
        context::server::Context &context_;
    };
}
