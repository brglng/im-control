#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP

struct CliArgs {
    const char* langid;
    const char* guidProfile;
};

int parse_args(int argc, const char *argv[], CliArgs* args);

#endif /* ARGPARSE_HPP */
