
#include "bsysinfo.h"

void print_obj(sysobj *s) {
    static const gchar none[] = "--";

    const gchar *label = NULL;
    gchar *nice = NULL;

    label = sysobj_label(s);
    nice = sysobj_format(s, FMT_OPT_NONE);

    if (label == NULL)
        label = none;

    printf("%s\t%s\t%lu\t%s\n", s->name, label, s->data.len, nice);
    g_free(nice);
}

int main(int argc, char **argv) {
    GDir *dir;
    const gchar *fn;

    if (argc < 2)
        return -1;

    class_init();

    sysobj *ex_obj = sysobj_new_from_fn(argv[1], NULL);
    printf("requested: %s\n", ex_obj->path_req);
    printf("resolved: %s\n", ex_obj->path);
    printf("exists: %s\n", ex_obj->exists ? "yes" : "no");
    printf("\n");

    if (ex_obj->exists) {
        print_obj(ex_obj);

        if (ex_obj->is_dir) {
            dir = g_dir_open(ex_obj->path, 0 , NULL);
            if (dir) {
                printf("------------------\n");
                while((fn = g_dir_read_name(dir)) != NULL) {
                    sysobj *obj = sysobj_new_from_fn(ex_obj->path, fn);
                    print_obj(obj);
                    sysobj_free(obj);
                }
                g_dir_close(dir);
            }
        }
    }
    sysobj_free(ex_obj);

    if (DEBUG_BUILD)
        class_dump_list();
    class_cleanup();

    return 0;
}
