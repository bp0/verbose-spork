
#include "bsysinfo.h"

int fmt_opts = FMT_OPT_ATERM | FMT_OPT_NO_JUNK;

void print_obj(sysobj *s) {
    static const gchar none[] = "--";

    const gchar *label = NULL;
    gchar *nice = NULL;

    label = sysobj_label(s);
    nice = sysobj_format(s, fmt_opts);

    if (label == NULL)
        label = none;

    printf("%s%s{%s}\t%s\t(%lu)\t%s\n",
        s->req_is_link ? "!" : "",
        s->name_req,
        s->cls ? (s->cls->tag ? s->cls->tag : s->cls->pattern) : "none",
        label, s->data.len, nice);
    g_free(nice);
}

int main(int argc, char **argv) {
    const gchar *altroot = NULL;
    const gchar *query = NULL;

    if (argc < 2)
        return -1;

    if (argc == 2) {
        query = argv[1];
    }

    if (argc > 2) {
        altroot = argv[1];
        query = argv[2];
    }

    sysobj_init();

    if (DEBUG_BUILD)
        class_dump_list();

    if (altroot)
        sysobj_root_set(altroot);

    sysobj *ex_obj = sysobj_new_from_fn(query, NULL);
    printf("root: %s/\n", sysobj_root);
    printf("requested: %s\n", ex_obj->path_req);
    printf("resolved: %s\n", ex_obj->path);
    printf("exists: %s\n", ex_obj->exists ? "yes" : "no");
    printf("\n");

    if (ex_obj->exists) {
        print_obj(ex_obj);
        if (ex_obj->is_dir) {
            GSList *l = NULL, *childs = sysobj_children(ex_obj);
            int len = g_slist_length(childs);
            printf("---[%d items]------------------------ \n", len);
            l = childs;
            while(l) {
                sysobj *obj = sysobj_new_from_fn(ex_obj->path, l->data);
                print_obj(obj);
                sysobj_free(obj);
                l = l->next;
            }
            g_slist_free_full(childs, g_free);
        }
    }
    sysobj_free(ex_obj);

    sysobj_cleanup();

    return 0;
}
