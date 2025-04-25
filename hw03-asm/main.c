#include <stdio.h>
#include <stdlib.h>

struct list_element {
    long value;
    long prev_addr;
};

char int_format[] = "%ld ";

void
print_int(struct list_element* li) {
    printf(int_format, li->value);
    fflush(stdout);
}

int
p(long value) {
    return value & 1;
}

struct list_element*
add_element(long prev_addr, long value) {
    struct list_element* li = (struct list_element*)malloc(sizeof(struct list_element));
    if (!li)
        return NULL;
    li->value = value;
    li->prev_addr = prev_addr;
    return li;
}

// m equals for_each
void
m(struct list_element* li, void (*func)(struct list_element*)) {
    if (!li)
        return;

    struct list_element* prev_li = (struct list_element*)li->prev_addr;
    func(li);
    m(prev_li, func);
}

// память была выделена (malloc), но не освобождена!
void
free_element(struct list_element* li) {
    free(li);
}

// f equals copy_if
struct list_element*
f(struct list_element* src, int (*func)(long), struct list_element* dst) {
    if (!src)
        return dst;
    long value = src->value;
    if (func(value))
        dst = add_element(dst ? (long)dst : 0l, value);
    return f((struct list_element*)src->prev_addr, func, dst);
}

int main() {
    long data[] = { 4, 8, 15, 23, 42 };
    size_t data_length = sizeof(data) / sizeof(data[0]);

    struct list_element* first_le1 = NULL;
    for (size_t i = data_length; i > 0; i--) {
        long value = data[i - 1];
        first_le1 = add_element(first_le1 ? (long)first_le1 : 0l, value);
    }

    m(first_le1, print_int);
    puts("");

    struct list_element* fisrt_le2 = NULL;
    fisrt_le2 = f(first_le1, p, fisrt_le2);
    m(fisrt_le2, print_int);
    puts("");

    // память была выделена (malloc), но не освобождена!
    m(first_le1, free_element);
    m(fisrt_le2, free_element);

    return 0;
}
