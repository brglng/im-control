#include <cstring>
#include "argparse.hpp"
#include "err.hpp"
#include "log.hpp"

int CliArgs::parse(int argc, const char *argv[]) {
    if (argc == 0) {
        return 0;
    }
    verb = VERB_SWITCH;
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] != '-') {
            id = argv[i];
        } else if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--keyboard") == 0) {
            if (i + 1 < argc) {
                if (strcmp(argv[i + 1], "open") == 0) {
                    keyboardOpenClose = true;
                } else if (strcmp(argv[i + 1], "close") == 0) {
                    keyboardOpenClose = false;
                } else {
                    return ERR_INVALID_ARGUMENTS;
                }
                i++;
            } else {
                return ERR_INVALID_ARGUMENTS;
            }
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--conversion-mode") == 0) {
            if (i + 1 < argc) {
                if (argv[i + 1][0] == '-') {
                    return ERR_INVALID_ARGUMENTS;
                }
                conversionMode = argv[i + 1];
                i++;
            } else {
                return ERR_INVALID_ARGUMENTS;
            }
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0) {
            verb = VERB_LIST;
            return 0;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                if (argv[i + 1][0] == '-') {
                    return ERR_INVALID_ARGUMENTS;
                }
                outputFile = argv[i + 1];
                i++;
            } else {
                return ERR_INVALID_ARGUMENTS;
            }
        } else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0) {
            verb = VERB_VERSION;
            return 0;
        } else {
            return ERR_INVALID_ARGUMENTS;
        }
    }
    if (id == nullptr && !keyboardOpenClose && conversionMode == nullptr) {
        verb = VERB_CURRENT;
    }
    return 0;
}
