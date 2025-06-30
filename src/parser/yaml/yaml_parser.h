#pragma once
/**
 * @file context_yaml_reader.h
 * @brief Header‑only YAML <‑‑> object bridge for the kut::context
 *        data model (see *client_context.hpp*).
 *
 * Depends on **yaml‑cpp** (https://github.com/jbeder/yaml‑cpp).
 * Usage example:
 *
 * ```cpp
 * #include "client_context.hpp"
 * #include "context_yaml_reader.hpp"
 *
 * using kut::context::io::YamlReader;
 *
 * int main() {
 *     auto clientCtx = YamlReader::loadClientContextFromFile("scenario.yaml");
 * }
 * ```
 */

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

#include "context.h"   // data model

namespace kut::context::io {
    class YamlReader {
    public:
        /*------------------------------------------------------ FILE ENTRY */
        [[nodiscard]] static client::Context
        loadClientContextFromFile(const std::string &path) {
            return node2ClientCtx(path2Yaml(path));
        }

        [[nodiscard]] static client::Context
        loadClientContextFromString(const std::string &text) {
            return node2ClientCtx(string2Yaml(text));
        }

        [[nodiscard]] static server::Context
        loadServerContextFromFile(const std::string &path) {
            return node2ServerCtx(path2Yaml(path));
        }

        [[nodiscard]] static server::Context
        loadServerContextFromString(const std::string &text) {
            return node2ServerCtx(string2Yaml(text));
        }

    private:
        /*------------------------------------------------ YAML UTILITIES */
        [[nodiscard]] static YAML::Node path2Yaml(const std::string &path) {
            try {
                return YAML::LoadFile(path);
            } catch (const std::exception &e) {
                throw std::runtime_error("[YamlReader] Cannot open YAML file '" + path + "': " + e.what());
            }
        }

        [[nodiscard]] static YAML::Node string2Yaml(const std::string &text) {
            try {
                return YAML::Load(text);
            } catch (const YAML::ParserException &e) {
                throw std::runtime_error(std::string{"[YamlReader] YAML parse error: "} + e.what());
            }
        }

        /*------------------------------------------------ CLIENT CONTEXT */
        [[nodiscard]] static client::Context node2ClientCtx(const YAML::Node &root) {
            const YAML::Node scenario = root["scenario"];
            if (!scenario) {
                throw std::runtime_error("[YamlReader] 'scenario' root key missing for client context");
            }

            client::Context ctx;
            parseMeta(ctx.meta(), scenario["meta"]);
            parseNetwork(ctx.network(), scenario["network"], /*endpointIsRequired=*/true);
            parseTraffic(ctx.traffic(), scenario["traffic_pattern"]);
            parseMetric(ctx.measurement(), scenario["metric_collection"]);
            return ctx;
        }

        /*------------------------------------------------ SERVER CONTEXT */
        [[nodiscard]] static server::Context node2ServerCtx(const YAML::Node &root) {
            const YAML::Node server = root["server"];
            if (!server) {
                throw std::runtime_error("[YamlReader] 'server' root key missing for server context");
            }

            server::Context ctx;
            parseMeta(ctx.meta(), server["meta"]);
            parseNetwork(ctx.network(), server["network"], /*endpointIsRequired=*/false);
            parseMetric(ctx.measurement(), server["metric_collection"]);
            return ctx;
        }

        /*------------------------------------------------ META */
        static void parseMeta(meta::Config &metaCfg, const YAML::Node &node) {
            if (!node) return; // optional
            if (auto n = node["name"]) metaCfg.name(n.as<std::string>());
        }

        /*------------------------------------------------ NETWORK */
        static void parseNetwork(network::Config &netCfg,
                                 const YAML::Node &node,
                                 bool endpointIsRequired) {
            if (!node) return;

            if (auto proto = node["protocol"]) netCfg.protocol(proto.as<std::string>());

            auto &ep = netCfg.endpoint();
            const YAML::Node endpointNode = node["endpoint"];
            if (endpointNode) {
                if (auto ip = endpointNode["ip"]) ep.address(ip.as<std::string>());
                if (auto port = endpointNode["port"]) ep.port(port.as<int>());
                if (auto ctrlPort = endpointNode["control_port"]) {
                    ep.controlPort(ctrlPort.as<int>());
                } else {
                    // Default control port is 60001
                    ep.controlPort(60001);
                }
            } else if (endpointIsRequired) {
                throw std::runtime_error("[YamlReader] 'endpoint' section missing under network");
            }
        }

        /*------------------------------------------------ TRAFFIC */
        static void parseTraffic(traffic::Config &trCfg, const YAML::Node &node) {
            if (!node) return;

            if (auto dur = node["duration_sec"]) trCfg.duration_sec(dur.as<double>());
            if (auto intv = node["interval_sec"]) trCfg.interval_sec(intv.as<double>());
            if (auto pcks = node["packet_size"]) trCfg.packet_size(pcks.as<int>());

            // Select traffic pattern
            const std::string type = node["type"].as<std::string>("constant");

            const YAML::Node params = node["parameters"];

            if (type == "constant") {
                fillConstant(trCfg.constant(), params);
            } else if (type == "burst") {
                fillBurst(trCfg.burst(), params);
            } else if (type == "pure_burst") {
                fillPureBurst(trCfg.pure_burst(), params);
            } else if (type == "random_burst") {
                fillRandomBurst(trCfg.random_burst(), params);
            } else if (type == "sinusoidal") {
                fillSinusoidal(trCfg.sinusoidal(), params);
            } else if (type == "ramp_up") {
                fillRampUp(trCfg.ramp_up(), params);
            } else if (type == "ramp_down") {
                fillRampDown(trCfg.ramp_down(), params);
            } else {
                throw std::runtime_error("[YamlReader] Unsupported traffic type '" + type + "'");
            }
        }

        /* traffic‑pattern fillers ------------------------------------------------ */
        static void fillConstant(traffic::Constant &c, const YAML::Node &p) {
            if (!p) return;
            if (auto v = p["rate_mbps"]) c.rate_mbps(v.as<double>());
        }

        static void fillBurst(traffic::Burst &b, const YAML::Node &p) {
            if (!p) return;
            if (auto v = p["rate_mbps"]) b.rate_mbps(v.as<double>());
            if (auto v = p["burst_size_mb"]) b.burst_size_mb(v.as<double>());
            if (auto v = p["burst_duration_sec"]) b.burst_duration_sec(v.as<double>());
            if (auto v = p["burst_interval_sec"]) b.burst_interval_sec(v.as<double>());
        }

        static void fillPureBurst(traffic::PureBurst &pb, const YAML::Node &p) {
            if (!p) return;
            if (auto v = p["rate_mbps"]) pb.rate_mbps(v.as<double>());
            if (auto v = p["burst_size_mb"]) pb.burst_size_mb(v.as<double>());
            if (auto v = p["burst_duration_sec"]) pb.burst_duration_sec(v.as<double>());
            if (auto v = p["burst_interval_sec"]) pb.burst_interval_sec(v.as<double>());
        }

        static void fillRandomBurst(traffic::RandomBurst &rb, const YAML::Node &p) {
            if (!p) return;
            if (auto v = p["rate_mbps"]) rb.rate_mbps(v.as<double>());
            if (auto v = p["min_burst_size_mb"]) rb.min_burst_size_mb(v.as<double>());
            if (auto v = p["max_burst_size_mb"]) rb.max_burst_size_mb(v.as<double>());
            if (auto v = p["min_burst_duration_ms"]) rb.min_burst_duration_ms(v.as<double>());
            if (auto v = p["max_burst_duration_ms"]) rb.max_burst_duration_ms(v.as<double>());
            if (auto v = p["min_burst_interval_ms"]) rb.min_burst_interval_ms(v.as<double>());
            if (auto v = p["max_burst_interval_ms"]) rb.max_burst_interval_ms(v.as<double>());
            if (auto v = p["random_seed"]) rb.random_seed(v.as<int>());
        }

        static void fillSinusoidal(traffic::Sinusoidal &s, const YAML::Node &p) {
            if (!p) return;
            if (auto v = p["rate_mbps"]) s.rate_mbps(v.as<double>());
            if (auto v = p["amplitude"]) s.amplitude(v.as<double>());
            if (auto v = p["frequency"]) s.frequency(v.as<double>());
        }

        static void fillRampUp(traffic::RampUp &r, const YAML::Node &p) {
            if (!p) return;
            if (auto v = p["rate_mbps"]) r.rate_mbps(v.as<double>());
            if (auto v = p["ramp_up_time"]) r.ramp_up_time(v.as<double>());
        }

        static void fillRampDown(traffic::RampDown &r, const YAML::Node &p) {
            if (!p) return;
            if (auto v = p["rate_mbps"]) r.rate_mbps(v.as<double>());
            if (auto v = p["ramp_down_time"]) r.ramp_down_time(v.as<double>());
        }

        /*------------------------------------------------ METRIC */
        static void parseMetric(metric::Config &mCfg, const YAML::Node &node) {
            if (!node) return;
            if (auto v = node["interval_sec"]) mCfg.interval_sec(v.as<double>());
            if (auto v = node["output_path"]) mCfg.output_path(v.as<std::string>());
            if (auto v = node["interface"]) mCfg.interface(v.as<std::string>());
            if (auto v = node["metrics"]) mCfg.metric_names(v.as<std::vector<std::string> >());
        }
    };

    class YamlWriter final {
    public:
        /*------------------------------------------------------ PUBLIC API */
        /// Serialize a **client::Context** instance to a YAML string.
        [[nodiscard]] static std::string dumpClientContextToString(const client::Context &ctx) {
            YAML::Emitter out;
            clientCtx2Yaml(ctx, out);
            return {out.c_str()};
        }

        /// Serialize a **client::Context** instance and write it to *path*.
        static void dumpClientContextToFile(const client::Context &ctx, const std::string &path) {
            std::ofstream ofs(path);
            if (!ofs) {
                throw std::runtime_error("[YamlWriter] Cannot open file for writing: " + path);
            }
            ofs << dumpClientContextToString(ctx);
        }

    private:
        /*------------------------------------------------ INTERNAL HELPERS */
        static void clientCtx2Yaml(const client::Context &ctx, YAML::Emitter &out) {
            out << YAML::BeginMap;
            out << YAML::Key << "scenario" << YAML::Value;
            out << YAML::BeginMap;

            writeMeta(ctx.meta(), out);
            writeNetwork(ctx.network(), out);
            writeTraffic(ctx.traffic(), out);
            writeMetric(ctx.measurement(), out);

            out << YAML::EndMap; // scenario
            out << YAML::EndMap; // root
        }

        /*-------------------------------------------- META */
        static void writeMeta(const meta::Config &metaCfg, YAML::Emitter &out) {
            out << YAML::Key << "meta" << YAML::Value;
            out << YAML::BeginMap;
            if (!metaCfg.name().empty()) {
                out << YAML::Key << "name" << YAML::Value << metaCfg.name();
            }
            out << YAML::EndMap;
        }

        /*-------------------------------------------- NETWORK */
        static void writeNetwork(const network::Config &netCfg, YAML::Emitter &out) {
            out << YAML::Key << "network" << YAML::Value;
            out << YAML::BeginMap;
            out << YAML::Key << "protocol" << YAML::Value << netCfg.protocol();

            const auto &ep = netCfg.endpoint();
            out << YAML::Key << "endpoint" << YAML::Value;
            out << YAML::BeginMap;
            out << YAML::Key << "ip" << YAML::Value << ep.address();
            out << YAML::Key << "port" << YAML::Value << ep.port();
            out << YAML::Key << "control_port" << YAML::Value << ep.controlPort();
            out << YAML::EndMap; // endpoint
            out << YAML::EndMap; // network
        }

        /*-------------------------------------------- TRAFFIC */
        static void writeTraffic(const traffic::Config &trCfg, YAML::Emitter &out) {
            out << YAML::Key << "traffic_pattern" << YAML::Value;
            out << YAML::BeginMap;
            out << YAML::Key << "duration_sec" << YAML::Value << trCfg.duration_sec();
            out << YAML::Key << "interval_sec" << YAML::Value << trCfg.interval_sec();
            out << YAML::Key << "packet_size" << YAML::Value << trCfg.packet_size();

            std::string type = trCfg.type();
            out << YAML::Key << "type" << YAML::Value << type;

            // ---------------------------------------------------------------------
            // pattern-specific parameters
            // ---------------------------------------------------------------------
            out << YAML::Key << "parameters" << YAML::Value;
            out << YAML::BeginMap;
            if (type == "constant") {
                out << YAML::Key << "rate_mbps" << YAML::Value << trCfg.constant().rate_mbps();
            } else if (type == "burst") {
                const auto &b = trCfg.burst();
                out << YAML::Key << "rate_mbps" << YAML::Value << b.rate_mbps();
                out << YAML::Key << "burst_size_mb" << YAML::Value << b.burst_size_mb();
                out << YAML::Key << "burst_duration_sec" << YAML::Value << b.burst_duration_sec();
                out << YAML::Key << "burst_interval_sec" << YAML::Value << b.burst_interval_sec();
            } else if (type == "pure_burst") {
                const auto &pb = trCfg.pure_burst();
                out << YAML::Key << "rate_mbps" << YAML::Value << pb.rate_mbps();
                out << YAML::Key << "burst_size_mb" << YAML::Value << pb.burst_size_mb();
                out << YAML::Key << "burst_duration_sec" << YAML::Value << pb.burst_duration_sec();
                out << YAML::Key << "burst_interval_sec" << YAML::Value << pb.burst_interval_sec();
            } else if (type == "random_burst") {
                const auto &rb = trCfg.random_burst();
                out << YAML::Key << "rate_mbps" << YAML::Value << rb.rate_mbps();
                out << YAML::Key << "min_burst_size_mb" << YAML::Value << rb.min_burst_size_mb();
                out << YAML::Key << "max_burst_size_mb" << YAML::Value << rb.max_burst_size_mb();
                out << YAML::Key << "min_burst_duration_ms" << YAML::Value << rb.min_burst_duration_ms();
                out << YAML::Key << "max_burst_duration_ms" << YAML::Value << rb.max_burst_duration_ms();
                out << YAML::Key << "min_burst_interval_ms" << YAML::Value << rb.min_burst_interval_ms();
                out << YAML::Key << "max_burst_interval_ms" << YAML::Value << rb.max_burst_interval_ms();
                out << YAML::Key << "random_seed" << YAML::Value << rb.random_seed();
            } else if (type == "sinusoidal") {
                const auto &s = trCfg.sinusoidal();
                out << YAML::Key << "rate_mbps" << YAML::Value << s.rate_mbps();
                out << YAML::Key << "amplitude" << YAML::Value << s.amplitude();
                out << YAML::Key << "frequency" << YAML::Value << s.frequency();
            } else if (type == "ramp_up") {
                const auto &r = trCfg.ramp_up();
                out << YAML::Key << "rate_mbps" << YAML::Value << r.rate_mbps();
                out << YAML::Key << "ramp_up_time" << YAML::Value << r.ramp_up_time();
            } else if (type == "ramp_down") {
                const auto &r = trCfg.ramp_down();
                out << YAML::Key << "rate_mbps" << YAML::Value << r.rate_mbps();
                out << YAML::Key << "ramp_down_time" << YAML::Value << r.ramp_down_time();
            }
            out << YAML::EndMap; // parameters
            out << YAML::EndMap; // traffic_pattern
        }

        /*-------------------------------------------- METRIC */
        static void writeMetric(const metric::Config &mCfg, YAML::Emitter &out) {
            out << YAML::Key << "metric_collection" << YAML::Value;
            out << YAML::BeginMap;
            out << YAML::Key << "interval_sec" << YAML::Value << mCfg.interval_sec();
            out << YAML::Key << "output_path" << YAML::Value << mCfg.output_path();
            out << YAML::Key << "interface" << YAML::Value << mCfg.interface();
            if (!mCfg.metric_names().empty()) {
                out << YAML::Key << "metrics" << YAML::Value << YAML::BeginSeq;
                for (const auto &n: mCfg.metric_names()) {
                    out << n;
                }
                out << YAML::EndSeq;
            }
            out << YAML::EndMap; // metric_collection
        }
    };
} // namespace kut::context::io
