#include <cstring>
#include "argparse.hpp"
#include "err.hpp"

int parse_args(int argc, const char *argv[], CliArgs* args) {
    memset(args, 0, sizeof(CliArgs));
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-langid") == 0) {
            if (i + 1 < argc) {
                if (argv[i + 1][0] == '-') {
                    return ERR_INVALID_ARGUMENTS;
                }
                args->langid = argv[i + 1];
                i++;
            } else {
                return ERR_INVALID_ARGUMENTS;
            }
        } else if (strcmp(argv[i], "-guidProfile") == 0) {
            if (i + 1 < argc) {
                if (argv[i + 1][0] == '-') {
                    return ERR_INVALID_ARGUMENTS;
                }
                args->guidProfile = argv[i + 1];
                i++;
            } else {
                return ERR_INVALID_ARGUMENTS;
            }
        } else if (strcmp(argv[i], "-keyboardOpen") == 0) {
            args->keyboardOpenClose = true;
        } else if (strcmp(argv[i], "-keyboardClose") == 0) {
            args->keyboardOpenClose = false;
        } else if (strcmp(argv[i], "-conversionMode") == 0) {
            if (i + 1 < argc) {
                if (argv[i + 1][0] == '-') {
                    return ERR_INVALID_ARGUMENTS;
                }
                args->conversionMode = argv[i + 1];
                i++;
            } else {
                return ERR_INVALID_ARGUMENTS;
            }
        } else {
            return ERR_INVALID_ARGUMENTS;
        }
    }
    if (args->langid == nullptr && args->guidProfile == nullptr && args->conversionMode == nullptr) {
        return ERR_INVALID_ARGUMENTS;
    }
    return 0;
}
