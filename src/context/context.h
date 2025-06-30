#pragma once
/**
 * @file client_context.hpp
 * @brief Header‑only data model for kut client scenarios using
 *        encapsulated classes with getters & setters.
 *        The classes are organised into sub‑namespaces
 *        `network` and `traffic` below `kut::context::client` to
 *        group related concepts logically.
 *
 * Rationale for sub‑namespaces:
 *   • `network`  – Connection‑related entities (endpoint, protocol).
 *   • `traffic`  – Traffic pattern configurations.
 *
 * This keep the global identifier space tidy while still allowing
 * terse names like `network::Endpoint` when the enclosing namespace
 * is already in scope.
 */

#include <string>
#include <utility>
#include <vector>
namespace kut::context {

    namespace meta {
        class Config {
        public:
            const std::string &name() const noexcept { return name_; }
            void name(std::string v) { name_ = std::move(v); }

        private:
            std::string name_;
        };
    } // namespace meta

    namespace network {
        class Endpoint {
        public:
            Endpoint() = default;

            Endpoint(std::string address, int port)
                : address_(std::move(address)), port_{port} {
            }

            [[nodiscard]] const std::string &address() const noexcept { return address_; }
            void address(std::string addr) { address_ = std::move(addr); }

            [[nodiscard]] int port() const noexcept { return port_; }
            void port(int p) noexcept { port_ = p; }

            [[nodiscard]] int controlPort() const noexcept { return controlPort_; }
            void controlPort(int p) noexcept { controlPort_ = p; }

        private:
            std::string address_{""};
            int port_{0};
            int controlPort_{0};
        };

        class Config {
        public:
            Config() = default;

            Config(std::string protocol, Endpoint endpoint)
                : protocol_(std::move(protocol)), endpoint_(std::move(endpoint)) {
            }

            [[nodiscard]] const std::string &protocol() const noexcept { return protocol_; }
            void protocol(std::string p) { protocol_ = std::move(p); }

            [[nodiscard]] const Endpoint &endpoint() const noexcept { return endpoint_; }
            Endpoint &endpoint() noexcept { return endpoint_; }
            void endpoint(Endpoint ep) { endpoint_ = std::move(ep); }

        private:
            std::string protocol_{"tcp"};
            Endpoint endpoint_{};
        };
    } // namespace network

    namespace traffic {
        class Constant {
        public:
            double rate_mbps() const noexcept { return rate_mbps_; }
            void rate_mbps(double v) noexcept { rate_mbps_ = v; }

        private:
            double rate_mbps_{10.0};
        };

        class Burst {
        public:
            double rate_mbps() const noexcept { return rate_mbps_; }
            void rate_mbps(double v) noexcept { rate_mbps_ = v; }

            double burst_size_mb() const noexcept { return burst_size_mb_; }
            void burst_size_mb(double v) noexcept { burst_size_mb_ = v; }

            double burst_duration_sec() const noexcept { return burst_duration_sec_; }
            void burst_duration_sec(double v) noexcept { burst_duration_sec_ = v; }

            double burst_interval_sec() const noexcept { return burst_interval_sec_; }
            void burst_interval_sec(double v) noexcept { burst_interval_sec_ = v; }

        private:
            double rate_mbps_{};
            double burst_size_mb_{};
            double burst_duration_sec_{};
            double burst_interval_sec_{};
        };

        class PureBurst {
        public:
            double rate_mbps() const noexcept { return rate_mbps_; }
            void rate_mbps(double v) noexcept { rate_mbps_ = v; }

            double burst_size_mb() const noexcept { return burst_size_mb_; }
            void burst_size_mb(double v) noexcept { burst_size_mb_ = v; }

            double burst_duration_sec() const noexcept { return burst_duration_sec_; }
            void burst_duration_sec(double v) noexcept { burst_duration_sec_ = v; }

            double burst_interval_sec() const noexcept { return burst_interval_sec_; }
            void burst_interval_sec(double v) noexcept { burst_interval_sec_ = v; }

        private:
            double rate_mbps_{0.0};
            double burst_size_mb_{};
            double burst_duration_sec_{};
            double burst_interval_sec_{};
        };

        class RandomBurst {
        public:
            double rate_mbps() const noexcept { return rate_mbps_; }
            void rate_mbps(double v) noexcept { rate_mbps_ = v; }

            double min_burst_size_mb() const noexcept { return min_burst_size_mb_; }
            void min_burst_size_mb(double v) noexcept { min_burst_size_mb_ = v; }

            double max_burst_size_mb() const noexcept { return max_burst_size_mb_; }
            void max_burst_size_mb(double v) noexcept { max_burst_size_mb_ = v; }

            double min_burst_duration_ms() const noexcept { return min_burst_duration_ms_; }
            void min_burst_duration_ms(double v) noexcept { min_burst_duration_ms_ = v; }

            double max_burst_duration_ms() const noexcept { return max_burst_duration_ms_; }
            void max_burst_duration_ms(double v) noexcept { max_burst_duration_ms_ = v; }

            double min_burst_interval_ms() const noexcept { return min_burst_interval_ms_; }
            void min_burst_interval_ms(double v) noexcept { min_burst_interval_ms_ = v; }

            double max_burst_interval_ms() const noexcept { return max_burst_interval_ms_; }
            void max_burst_interval_ms(double v) noexcept { max_burst_interval_ms_ = v; }

            int random_seed() const noexcept { return random_seed_; }
            void random_seed(int v) noexcept { random_seed_ = v; }

        private:
            double rate_mbps_{0.0};
            double min_burst_size_mb_{};
            double max_burst_size_mb_{};
            double min_burst_duration_ms_{};
            double max_burst_duration_ms_{};
            double min_burst_interval_ms_{};
            double max_burst_interval_ms_{};
            int random_seed_{0};
        };

        class Sinusoidal {
        public:
            double rate_mbps() const noexcept { return rate_mbps_; }
            void rate_mbps(double v) noexcept { rate_mbps_ = v; }

            double amplitude() const noexcept { return amplitude_; }
            void amplitude(double v) noexcept { amplitude_ = v; }

            double frequency() const noexcept { return frequency_; }
            void frequency(double v) noexcept { frequency_ = v; }

        private:
            double rate_mbps_{0.0};
            double amplitude_{};
            double frequency_{}; // Hz
        };

        class RampUp {
        public:
            double rate_mbps() const noexcept { return rate_mbps_; }
            void rate_mbps(double v) noexcept { rate_mbps_ = v; }

            double ramp_up_time() const noexcept { return ramp_up_time_; }
            void ramp_up_time(double v) noexcept { ramp_up_time_ = v; }

        private:
            double rate_mbps_{0.0};
            double ramp_up_time_{}; // seconds
        };

        class RampDown {
        public:
            double rate_mbps() const noexcept { return rate_mbps_; }
            void rate_mbps(double v) noexcept { rate_mbps_ = v; }

            double ramp_down_time() const noexcept { return ramp_down_time_; }
            void ramp_down_time(double v) noexcept { ramp_down_time_ = v; }

        private:
            double rate_mbps_{0.0};
            double ramp_down_time_{}; // seconds
        };

        class Config {
        public:
            double duration_sec() const noexcept { return duration_sec_; }
            void duration_sec(double v) noexcept { duration_sec_ = v; }

            double interval_sec() const noexcept { return interval_sec_; }
            void interval_sec(double v) noexcept { interval_sec_ = v; }

            int packet_size() const noexcept { return packet_size_; }
            void packet_size(int v) noexcept { packet_size_ = v; }

            Constant &constant() noexcept { return constant_; }
            const Constant &constant() const noexcept { return constant_; }

            Burst &burst() noexcept { return burst_; }
            const Burst &burst() const noexcept { return burst_; }

            PureBurst &pure_burst() noexcept { return pure_burst_; }
            const PureBurst &pure_burst() const noexcept { return pure_burst_; }

            RampUp &ramp_up() noexcept { return ramp_up_; }
            const RampUp &ramp_up() const noexcept { return ramp_up_; }

            RandomBurst &random_burst() noexcept { return random_burst_; }
            const RandomBurst &random_burst() const noexcept { return random_burst_; }

            Sinusoidal &sinusoidal() noexcept { return sinusoidal_; }
            const Sinusoidal &sinusoidal() const noexcept { return sinusoidal_; }

            RampDown &ramp_down() noexcept { return ramp_down_; }
            const RampDown &ramp_down() const noexcept { return ramp_down_; }

            std::string type() const noexcept { return type_; }
            void type(std::string v) noexcept { type_ = std::move(v); }

        private:
            double duration_sec_{10.0};
            double interval_sec_{1.0};
            std::string type_ {"constant"}; // Default traffic type
            int packet_size_{1250}; // Default packet size in bytes

            Constant constant_{};
            Burst burst_{};
            PureBurst pure_burst_{};
            RampUp ramp_up_{};
            RandomBurst random_burst_{};
            Sinusoidal sinusoidal_{};
            RampDown ramp_down_{};
        };
    } // namespace traffic

    namespace metric {
        class Config {
        public:
            double interval_sec() const noexcept { return interval_sec_; }
            void interval_sec(double v) noexcept { interval_sec_ = v; }

            const std::string &output_path() const noexcept { return output_path_; }
            void output_path(std::string v) { output_path_ = std::move(v); }

            const std::string &interface() const noexcept { return interface_; }
            void interface(std::string v) { interface_ = std::move(v); }

            const std::vector<std::string> &metric_names() const noexcept { return metrics_; }
            void metric_names(std::vector<std::string> v) { metrics_ = std::move(v); }


        private:
            double interval_sec_{1.0};
            std::string output_path_{"metrics.csv"};
            std::string interface_ {"eth0"};
            std::vector<std::string> metrics_;
        };
    } // namespace metric


    namespace client {

        class Context {
        public:
            meta::Config &meta() noexcept { return meta_; }
            const meta::Config &meta() const noexcept { return meta_; }

            network::Config &network() noexcept { return network_; }
            const network::Config &network() const noexcept { return network_; }

            traffic::Config &traffic() noexcept { return traffic_; }
            const traffic::Config &traffic() const noexcept { return traffic_; }

            metric::Config &measurement() noexcept { return measurement_; }
            const metric::Config &measurement() const noexcept { return measurement_; }

        private:
            meta::Config meta_{};
            network::Config network_{};
            traffic::Config traffic_{};
            metric::Config measurement_{};
        };
    } // namespace kut::context::client

    namespace server {
        class Context {
        public:
            meta::Config &meta() noexcept { return meta_; }
            const meta::Config &meta() const noexcept { return meta_; }

            network::Config &network() noexcept { return network_; }
            const network::Config &network() const noexcept { return network_; }

            metric::Config &measurement() noexcept { return measurement_; }
            const metric::Config &measurement() const noexcept { return measurement_; }

        private:
            meta::Config meta_{};
            network::Config network_{};
            metric::Config measurement_{};
        };
    }
}

