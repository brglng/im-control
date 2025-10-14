#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP

#include <optional>
#include "verb.hpp"

struct CliArgs {
    Verb                    verb;
    const char*             id;
    std::optional<bool>     keyboardOpenClose;
    const char*             conversionMode;

    CliArgs() :
        verb(VERB_CURRENT),
        id(nullptr),
        keyboardOpenClose(),
        conversionMode(nullptr)
    {}

    int parse(int argc, const char *argv[]);
};

#endif /* ARGPARSE_HPP */
