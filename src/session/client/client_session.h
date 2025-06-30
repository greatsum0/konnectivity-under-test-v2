#pragma once

#include <chrono>
#include <iostream>
#include <ranges>
#include <unordered_map>
#include <vector>
#include <thread>          // sleep_until
#include <future>          // promise, future

/* ─────────── Project headers ─────────── */
#include <logger.h>
#include <traffic_pattern_factory.h>
#include <cpu.h>
#include <network.h>
#include <udp_client.h>
#include <tcp_client.h>
#include <context.h>
#include <fstream>
#include <session_common.h>
#include <yaml_parser.h>

#include "control_channel.h"

namespace kut::session::client {
    template<typename ClientT>
    void runTransport(context::client::Context &context,
                      const common::Protocol protocol) {
        /* ── 1) Timing helpers ─────────────────────────────────────────── */
        const int runSec = context.traffic().duration_sec();
        const auto t0_steady = std::chrono::steady_clock::now(); // scheduler clock
        const auto deadline = t0_steady + std::chrono::seconds(runSec);

        /* ── 2) I/O contexts ───────────────────────────────────────────── */
        auto controlClientCtx = std::make_shared<boost::asio::io_context>();
        auto trafficCtx = std::make_shared<boost::asio::io_context>();

        /* ── 3) Control channel (start handshake) ─────────────────────── */
        std::promise<void> scenarioStartPromise;
        const auto scenarioStartFuture = scenarioStartPromise.get_future();

        control::ControlClient controlClient(*controlClientCtx, [&](const control::Packet &p) {
            if (p.type == control::Msg::Start)
                scenarioStartPromise.set_value();
        });

        const int controlPort = context.network().endpoint().controlPort();
        controlClient.connectAndRun("127.0.0.1", controlPort);
        Logger::instance().info("Control client connected to port ", controlPort);

        std::string contextYaml = context::io::YamlWriter::dumpClientContextToString(context);
        controlClient.sendCommand(control::Msg::Context, contextYaml); // announce presence
        /* ── 4) Traffic client + measurement tools ────────────────────── */
        ClientT client(*trafficCtx,
                       context.traffic().packet_size(),
                       TrafficPatternFactory::create(context.traffic()));

        measurement::network::PacketSniffer sniffer(
            context.measurement().interface(),
            context.network().endpoint().port());

        measurement::cpu::CPUUsageMonitor cpuMonitor(context.measurement().interval_sec());


        /* ── 5) Wait for START, then run traffic while sampling CPU ───── */
        scenarioStartFuture.wait(); // block until server says go

        auto t0 = measurement::clock::now();
        cpuMonitor.start();
        sniffer.start();
        client.start(context.network().endpoint().address(),
                     context.network().endpoint().port());


        Logger::instance().info("Traffic client started, running for ", runSec, " seconds");
        std::this_thread::sleep_until(deadline); // run for specified duration

        /* ── 6) Graceful shutdown ─────────────────────────────────────── */
        controlClient.sendCommand(control::Msg::Done, "");
        Logger::instance().info("Sending DONE command to server…");

        client.stop(); // stop traffic generator
        Logger::instance().info("Traffic client stopped");

        /* Wait until sniffer frame count stabilises (drain the wire) */
        std::size_t prev = 0;
        for (int stable = 0; stable < 3;) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::size_t curr = sniffer.peekCount();
            stable = (curr == prev) ? stable + 1 : 0;
            prev = curr;
        }
        sniffer.stop();
        Logger::instance().info("Sniffer stopped, collected ", sniffer.peekCount(), " frames");
        cpuMonitor.stop();
        Logger::instance().info("CPU monitor stopped");

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

        //filepath
        std::string outputFile = context.measurement().output_path() + "/" + context.meta().name() + ".csv";
        // 2. write to CSV (or inspect `result` programmatically)
        std::ofstream out(outputFile);
        write_csv(result, out);
    }


    /* ───────────────────── ClientSession façade ─────────────────────── */
    class ClientSession {
    public:
        explicit ClientSession(context::client::Context &context)
            : context_(context) {
        }

        void run() const {
            common::Protocol protocol = common::toProtocol(context_.network().protocol());
            switch (protocol) {
                case common::Protocol::Udp: runTransport<UDPClient>(context_, protocol);
                    break;
                case common::Protocol::Tcp: runTransport<TCPClient>(context_, protocol);
                    break;
                default:
                    throw std::runtime_error("Unsupported protocol: "
                                             + context_.network().protocol());
            }
        }

    private:
        context::client::Context &context_;
    };
}
