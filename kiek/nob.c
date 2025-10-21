#define NOB_IMPLEMENTATION
#include "nob.h"

#define IS_ARG(a, v) (0 == strcmp((a), (v)))

#define OUT_DIR "out/"

#define CC      "gcc"
#define CFLAGS  "-I../../include", "-ggdb", "-Wall", "-std=gnu11", "-Wextra", "-pedantic", "-Werror", "-c"
#define LIBNAME "kiek"

#define AR      "ar"
#define ARFLAGS "rcs"

#ifdef LIB_SHARED
#   define LIB_EXT ".so"
#else
#   define LIB_EXT ".a"
#endif /* LIB_SHARED */

/* IGNORE LIST */
static const char *ignore_list[] = {
    __FILE__, /* obviously, we disregard this very file */
};

#define MIN_FILENAME_LEN 1
/* checks for file extensions */
bool is_type(const char *file, const char *type)
{
    int type_len = strlen(type);
    int file_len = strlen(file);
    if (type_len + 1 + MIN_FILENAME_LEN > file_len)
        return false;

    const char *ext = &file[file_len-type_len];
    if (ext[-1] != '.')
        return false;

    return 0 == strcmp(ext, type);
}

/* check if file path is in ignore list */
bool is_ignored(const char *file)
{
    for (int i = 0; i < NOB_ARRAY_LEN(ignore_list); ++i) {
        if (0 == strcmp(file, ignore_list[i]))
            return true;
    }

    return false;
}

const char *src_path_to_obj_path(const char *c_file)
{
    char *o_file = NULL;
    asprintf(&o_file, OUT_DIR"%.*s.o", strlen(c_file)-2, c_file);

    /* we  won't change the contents (and want to avoid warnings) */
    return (const char *)o_file;
}

void parse_options(int argc, char **argv)
{
    if (argc < 2)
        return;

    Nob_Cmd cmd = {0};
    nob_shift(argv, argc);
    if (IS_ARG("clean", *argv)) {
        nob_cmd_append(&cmd, "rm", "-rf", OUT_DIR);
        NOB_ASSERT(nob_cmd_run_sync(cmd));
        exit(EXIT_SUCCESS);
    } else {
        nob_log(NOB_ERROR, "%s: no such target", *argv);
        exit(EXIT_FAILURE);
    }
}

void build_library(void)
{
    Nob_Procs comp_threads = {0};
    Nob_Cmd comp_cmd = {0};
    Nob_File_Paths src = {0};
    Nob_File_Paths obj_files = {0};

    NOB_ASSERT(nob_mkdir_if_not_exists(OUT_DIR));

    /* build local sources */
    NOB_ASSERT(nob_read_entire_dir(".", &src));
    for (int i = 0; i < src.count; ++i) {
        const char *c_src = NULL;
        const char *obj_file = NULL;
        if (!is_type(src.items[i], "c") || is_ignored(src.items[i]))
            continue;
        c_src = src.items[i];
        nob_cmd_append(&comp_cmd, CC, CFLAGS);
        obj_file = src_path_to_obj_path(src.items[i]);
        nob_da_append(&obj_files, obj_file);
        nob_cmd_append(&comp_cmd, c_src, "-o", obj_file);
        nob_da_append(&comp_threads, nob_cmd_run_async_and_reset(&comp_cmd));
    }
    NOB_ASSERT(nob_procs_wait_and_reset(&comp_threads));

#ifdef LIB_SHARED
    /* finally, link object files into a shared object */
    nob_cmd_append(&comp_cmd, LD, LDFLAGS);
#else
    /* finally, link object files into a static archive */
    nob_cmd_append(&comp_cmd, AR, ARFLAGS);
#endif

    nob_cmd_append(&comp_cmd, OUT_DIR "lib" LIBNAME LIB_EXT);
    for (int i = 0; i < obj_files.count; ++i) {
        nob_cmd_append(&comp_cmd, obj_files.items[i]);
    }

    nob_cmd_run_sync(comp_cmd);
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    parse_options(argc, argv);
    build_library();

    return EXIT_SUCCESS;
}

