#define NOB_IMPLEMENTATION
#define NOB_WARN_DEPRECATED

#include "thirdparty/nob.h"

static bool collect(File_Paths *out, const char *ext) {

    File_Paths names = {0};

    if (!read_entire_dir("src", &names)) {
        return false;
    }

    da_foreach(const char *, name, &names) {
        if (sv_ends_with_cstr(sv_from_cstr(*name), ext)) {
            da_append(out, temp_sprintf("src/%s", *name));
        }
    }

    da_free(names);
    return true;
}

static bool build(Cmd *cmd) {
    File_Paths srcs = {0};

    if (!collect(&srcs, ".c")) {
        return false;
    }

    cmd_append(cmd, "gcc", "-Wall", "-Wextra", "-g",
               "-fsanitize=address,undefined");
    da_append_many(cmd, srcs.items, srcs.count);
    cmd_append(cmd, "-o", "dbg");

    da_free(srcs);
    return cmd_run(cmd);
}

static bool fmt(Cmd *cmd) {

    File_Paths files = {0};

    if (!collect(&files, ".c") || !collect(&files, ".h")) {
        return false;
    }

    da_append(&files, "nob.c");
    cmd_append(cmd, "clang-format", "-i");
    da_append_many(cmd, files.items, files.count);

    da_free(files);
    return cmd_run(cmd);
}

static bool check(Cmd *cmd) {

    if (!build(cmd)) {
        return false;
    }

    cmd_append(cmd, "bash", "scripts/compare.sh");

    if (!cmd_run(cmd)) {
        return false;
    }

    cmd_append(cmd, "bash", "scripts/negative.sh");
    return cmd_run(cmd);
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char *target = "build";
    Cmd         cmd = {0};

    if (argc > 1) {
        target = argv[1];
    }

    if (strcmp(target, "build") == 0) {
        return build(&cmd) ? 0 : 1;
    }

    if (strcmp(target, "fmt") == 0) {
        return fmt(&cmd) ? 0 : 1;
    }

    if (strcmp(target, "check") == 0) {
        return check(&cmd) ? 0 : 1;
    }

    if (strcmp(target, "clean") == 0) {
        return delete_file("dbg") ? 0 : 1;
    }

    nob_log(ERROR, "unknown target: %s", target);
    nob_log(INFO, "usage: %s [build|fmt|check|clean]", argv[0]);
    return 1;
}
