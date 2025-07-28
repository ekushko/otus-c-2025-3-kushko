#include "topn_container.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct TopNContainer {
    GSList *list;
    size_t capacity;
    pthread_mutex_t mutex;
};

typedef struct {
    gchar* key;
    gint value;
} ListItem;

ListItem* li_init(gchar* key, gint value) {
    ListItem *item = (ListItem*)g_malloc(sizeof(ListItem));
    item->key = g_strdup(key);
    item->value = value;
    return item;
}

void li_free(ListItem* item) {
    g_free(item->key);
    g_free(item);
}

TopNContainer* topn_cont_init(int capacity) {
    TopNContainer *cont = (TopNContainer*)malloc(sizeof(TopNContainer));
    pthread_mutex_init(&cont->mutex, NULL);
    cont->list = NULL;
    cont->capacity = capacity;
    return cont;
}

void topn_cont_free(TopNContainer* cont) {
    cont->capacity = 0;
    GSList *iter = NULL;
    for (iter = cont->list; iter != NULL; iter = iter->next) {
        ListItem *item = (ListItem*)iter->data;
        li_free(item);
    }
    g_slist_free(cont->list);
    pthread_mutex_destroy(&cont->mutex);
    free(cont);
}

gint topn_value_compare(gconstpointer a, gconstpointer b) {
    ListItem *item_a = (ListItem*)a;
    ListItem *item_b = (ListItem*)b;

    if (item_a->value < item_b->value) {
        return 1;
    } else if (item_a->value > item_b->value) {
        return -1;
    }

    return 0;
}

void topn_cont_add_item(TopNContainer* cont, gchar* key, gint value) {
    GSList *iter = NULL;

    if (cont->list != NULL) {
        for (iter = cont->list; iter != NULL; iter = iter->next) {
            ListItem *item = (ListItem*)iter->data;
            if (g_strcmp0(item->key, key) == 0) {
                item->value += value;
                break;
            }
        }
    }

    if (iter == NULL) {
        ListItem *item = li_init(key, value);
        cont->list = g_slist_insert_sorted(cont->list, item, topn_value_compare);
    } else {
        cont->list = g_slist_sort(cont->list, topn_value_compare);
    }
}

void topn_cont_prep_capacity(TopNContainer* cont) {
    while (g_slist_length(cont->list) > cont->capacity) {
        gpointer last = g_slist_nth_data(cont->list, cont->capacity);
        cont->list = g_slist_remove(cont->list, last);
        ListItem *last_item = (ListItem*)last;
        li_free(last_item);
    }
}

void topn_cont_add(TopNContainer* cont, gchar* key, gint value) {
    pthread_mutex_lock(&cont->mutex);

    topn_cont_add_item(cont, key, value);

    topn_cont_prep_capacity(cont);

    pthread_mutex_unlock(&cont->mutex);
}

void topn_cont_merge(TopNContainer *dst, TopNContainer *src) {
    pthread_mutex_lock(&dst->mutex);

    GSList *src_copy = g_slist_copy(src->list);

    size_t capacity = dst->capacity;
    dst->capacity += src->capacity;

    GSList *iter;
    for (iter = src_copy; iter != NULL; iter = iter->next) {
        ListItem *item = (ListItem*)iter->data;
        topn_cont_add_item(dst, item->key, item->value);
    }

    dst->capacity = capacity;

    topn_cont_prep_capacity(dst);

    pthread_mutex_unlock(&dst->mutex);
}

void topn_cont_print(TopNContainer* cont, size_t count) {
    if (count > cont->capacity)
        count = cont->capacity;

    size_t length = g_slist_length(cont->list);
    if (count > length)
        count = length;

    pthread_mutex_lock(&cont->mutex);

    size_t i = 0;
    GSList *iter;
    for (iter = cont->list; iter != NULL; iter = iter->next) {
        ListItem *item = (ListItem*)iter->data;
        printf("%s => %d\n", item->key, item->value);
        if (i++ == count) {
            break;
        }
    }

    pthread_mutex_unlock(&cont->mutex);
}

void topn_test() {
    {   // sorting test
        TopNContainer *cont = topn_cont_init(5);
        topn_cont_add(cont, "n0", 1);
        topn_cont_add(cont, "n1", 2);
        topn_cont_add(cont, "n2", 3);
        topn_cont_add(cont, "n3", 4);
        topn_cont_add(cont, "n4", 5);
        topn_cont_add(cont, "n5", 6);
        assert(g_slist_length(cont->list) == 5);

        ListItem *item = NULL;
        item = (ListItem*)g_slist_nth(cont->list, 0)->data;
        assert(g_strcmp0(item->key, "n5") == 0);
        item = (ListItem*)g_slist_nth(cont->list, 1)->data;
        assert(g_strcmp0(item->key, "n4") == 0);
        item = (ListItem*)g_slist_nth(cont->list, 2)->data;
        assert(g_strcmp0(item->key, "n3") == 0);
        item = (ListItem*)g_slist_nth(cont->list, 3)->data;
        assert(g_strcmp0(item->key, "n2") == 0);
        item = (ListItem*)g_slist_nth(cont->list, 4)->data;
        assert(g_strcmp0(item->key, "n1") == 0);

        printf("sorting test passed\n");
        topn_cont_free(cont);
    }

    {   // equal values test
        TopNContainer *cont = topn_cont_init(5);
        topn_cont_add(cont, "n0", 1);
        topn_cont_add(cont, "n1", 1);
        topn_cont_add(cont, "n2", 1);
        topn_cont_add(cont, "n3", 6);
        topn_cont_add(cont, "n4", 6);
        topn_cont_add(cont, "n5", 6);
        assert(g_slist_length(cont->list) == 5);

        ListItem *item = NULL;
        item = (ListItem*)g_slist_nth(cont->list, 0)->data;
        assert(g_strcmp0(item->key, "n5") == 0);
        item = (ListItem*)g_slist_nth(cont->list, 1)->data;
        assert(g_strcmp0(item->key, "n4") == 0);
        item = (ListItem*)g_slist_nth(cont->list, 2)->data;
        assert(g_strcmp0(item->key, "n3") == 0);
        item = (ListItem*)g_slist_nth(cont->list, 3)->data;
        assert(g_strcmp0(item->key, "n2") == 0);
        item = (ListItem*)g_slist_nth(cont->list, 4)->data;
        assert(g_strcmp0(item->key, "n1") == 0);

        printf("equal values test passed\n");
        topn_cont_free(cont);
    }

    {   // merge test
        TopNContainer *dst = topn_cont_init(3);
        topn_cont_add(dst, "n0", 2);
        topn_cont_add(dst, "n1", 3);
        topn_cont_add(dst, "n2", 4);

        TopNContainer *src = topn_cont_init(4);
        topn_cont_add(src, "n0", 4);
        topn_cont_add(src, "n1", 3);
        topn_cont_add(src, "n2", 2);
        topn_cont_add(src, "n3", 7);

        topn_cont_merge(dst, src);

        assert(g_slist_length(dst->list) == 3);
        ListItem *item = NULL;
        item = (ListItem*)g_slist_nth(dst->list, 0)->data;
        assert(g_strcmp0(item->key, "n3") == 0);
        item = (ListItem*)g_slist_nth(dst->list, 1)->data;
        assert(g_strcmp0(item->key, "n0") == 0);
        item = (ListItem*)g_slist_nth(dst->list, 2)->data;
        assert(g_strcmp0(item->key, "n1") == 0);

        printf("merge test passed\n");
        topn_cont_free(dst);
        topn_cont_free(src);
    }

    {   // memory test
        TopNContainer *cont = topn_cont_init(20);

        for (size_t i = 0; i < 10000000; ++i) {
            char *key = malloc(2048);
            memset(key, g_random_int_range('a', 'z'), 2048);
            topn_cont_add(cont, key, g_random_int_range(0, 999999));
            free(key);
        }

        assert(g_slist_length(cont->list) == 20);

        printf("memory test passed\n");
        topn_cont_free(cont);
    }
}
