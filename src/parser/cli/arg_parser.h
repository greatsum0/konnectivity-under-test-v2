# pragma once

#include <string>

class ArgumentParser {
public:
    ArgumentParser(int argc, char *argv[]);

    [[nodiscard]] std::string getMode() const;

    [[nodiscard]] std::string getConfig() const;

    [[nodiscard]] bool shouldShowHelp() const;

    static std::string usageMessage();

private:
    std::string mode;
    std::string config;
    bool showHelp = false;
};
