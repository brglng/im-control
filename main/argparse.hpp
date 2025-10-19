#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP

#include <optional>
#include "verb.hpp"

struct CliArgs {
    Verb                    verb;
    const char*             key;
    std::optional<bool>     keyboardOpenClose;
    const char*             conversionMode;
    const char*             ifKey;
    const char*             elseKey;
    const char*             outputFile;

    CliArgs() :
        verb(VERB_CURRENT),
        key(nullptr),
        keyboardOpenClose(),
        conversionMode(nullptr),
        ifKey(nullptr),
        elseKey(nullptr),
        outputFile(nullptr)
    {}

    int parse(int argc, const char *argv[]);
};

#endif /* ARGPARSE_HPP */
