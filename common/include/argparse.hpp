#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP

#include <optional>
#include "verb.hpp"

struct CliArgs {
    Verb verb;
    const char* langid;
    const char* guidProfile;
    std::optional<bool> keyboardOpenClose;
    const char* conversionMode;

    CliArgs() :
        verb(VERB_CURRENT),
        langid(nullptr),
        guidProfile(nullptr),
        keyboardOpenClose(),
        conversionMode(nullptr)
    {}

    int parse(int argc, const char *argv[]);
};

#endif /* ARGPARSE_HPP */
