#include <cstdio>
#include <cstring>
#include "argparse.hpp"
#include "err.hpp"

int parse_args(int argc, const char *argv[], CliArgs* args) {
    memset(args, 0, sizeof(CliArgs));
    if (argc < 2) {
        return ERR_INVALID_ARGUMENTS;
    }
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-langid") == 0) {
            if (i + 1 < argc) {
                args->langid = argv[i + 1];
                i++;
            } else {
                return ERR_INVALID_ARGUMENTS;
            }
        } else if (strcmp(argv[i], "-guidProfile") == 0) {
            if (i + 1 < argc) {
                args->guidProfile = argv[i + 1];
                i++;
            } else {
                return ERR_INVALID_ARGUMENTS;
            }
        } else {
            return ERR_INVALID_ARGUMENTS;
        }
    }
    if (args->langid == nullptr && args->guidProfile == nullptr) {
        return ERR_INVALID_ARGUMENTS;
    }
    return 0;
}
