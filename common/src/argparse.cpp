#include <cstring>
#include "argparse.hpp"
#include "err.hpp"

int CliArgs::parse(int argc, const char *argv[]) {
    if (argc == 0) {
        return 0;
    }
    verb = VERB_SWITCH;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-langid") == 0) {
            if (i + 1 < argc) {
                if (argv[i + 1][0] == '-') {
                    return ERR_INVALID_ARGUMENTS;
                }
                langid = argv[i + 1];
                i++;
            } else {
                return ERR_INVALID_ARGUMENTS;
            }
        } else if (strcmp(argv[i], "-guidProfile") == 0) {
            if (i + 1 < argc) {
                if (argv[i + 1][0] == '-') {
                    return ERR_INVALID_ARGUMENTS;
                }
                guidProfile = argv[i + 1];
                i++;
            } else {
                return ERR_INVALID_ARGUMENTS;
            }
        } else if (strcmp(argv[i], "-keyboardOpen") == 0) {
            keyboardOpenClose = true;
        } else if (strcmp(argv[i], "-keyboardClose") == 0) {
            keyboardOpenClose = false;
        } else if (strcmp(argv[i], "-conversionMode") == 0) {
            if (i + 1 < argc) {
                if (argv[i + 1][0] == '-') {
                    return ERR_INVALID_ARGUMENTS;
                }
                conversionMode = argv[i + 1];
                i++;
            } else {
                return ERR_INVALID_ARGUMENTS;
            }
        } else if (strcmp(argv[i], "-list") == 0) {
            verb = VERB_LIST;
        } else if (strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "--version") == 0) {
            verb = VERB_PRINT_VERSION;
            return 0;
        } else {
            return ERR_INVALID_ARGUMENTS;
        }
    }
    if (langid == nullptr && guidProfile == nullptr && conversionMode == nullptr) {
        return ERR_INVALID_ARGUMENTS;
    }
    return 0;
}
