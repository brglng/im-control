#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP

#include <optional>

struct CliArgs {
    const char* langid;
    const char* guidProfile;
    std::optional<bool> keyboardOpenClose;
    const char* conversionMode;

    CliArgs() :
        langid(nullptr),
        guidProfile(nullptr),
        keyboardOpenClose(),
        conversionMode(nullptr)
    {}
};

int parse_args(int argc, const char *argv[], CliArgs* args);

#endif /* ARGPARSE_HPP */
