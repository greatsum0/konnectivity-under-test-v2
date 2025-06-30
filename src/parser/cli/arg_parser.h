#pragma once
#include <getopt.h>
#include <stdexcept>
#include <string>
/**
 *  namespace   : kut::io::cli
 *  class name  : ArgumentParser
 *
 *  example
 *  -------
 *      int main(int argc,char* argv[])
 *      {
 *          using kut::cli::io::ArgumentParser;
 *          ArgumentParser args(argc,argv);
 *
 *          if (args.shouldShowHelp())
 *              std::cout << ArgumentParser::usageMessage() << '\n';
 *
 *          std::cout << "mode   = " << args.getMode()   << '\n'
 *                    << "config = " << args.getConfig() << '\n';
 *      }
 */
namespace kut::cli::io {
    class ArgumentParser {
    public:
        ArgumentParser(int argc, char *argv[]) // may throw std::invalid_argument
        {
            constexpr char short_opts[] = "m:c:h";
            constexpr ::option long_opts[]{
                {"mode", required_argument, nullptr, 'm'},
                {"config", required_argument, nullptr, 'c'},
                {"help", no_argument, nullptr, 'h'},
                {nullptr, 0, nullptr, 0}
            };

            int opt;
            int long_index = 0;
            while ((opt = ::getopt_long(argc, argv, short_opts,
                                        long_opts, &long_index)) != -1) {
                switch (opt) {
                    case 'm':
                        mode_ = optarg;
                        if (mode_ != "server" && mode_ != "client")
                            throw std::invalid_argument("Invalid mode: " + mode_ +
                                                        ". Must be 'server' or 'client'.\n" +
                                                        usageMessage());
                        break;

                    case 'c':
                        config_ = optarg;
                        break;

                    case 'h':
                        showHelp_ = true;
                        break;

                    default:
                        throw std::invalid_argument("Unknown argument.\n" +
                                                    usageMessage());
                }
            }

            /* required parameters */
            if (mode_.empty())
                throw std::invalid_argument("Missing required argument: --mode.\n" +
                                            usageMessage());
            if (config_.empty())
                throw std::invalid_argument("Missing required argument: --config.\n" +
                                            usageMessage());
        }

        [[nodiscard]] std::string getMode() const { return mode_; }
        [[nodiscard]] std::string getConfig() const { return config_; }
        [[nodiscard]] bool shouldShowHelp() const { return showHelp_; }


        static inline std::string usageMessage() {
            return
                    "Usage: program_name -m <server|client> -c <file> [options]\n"
                    "Options:\n"
                    "  -m, --mode   <server|client>   Mode of operation (required)\n"
                    "  -c, --config <file>            Configuration file (required)\n"
                    "  -h, --help                     Show this help message";
        }

    private:
        std::string mode_;
        std::string config_;
        bool showHelp_{false};
    };
}
