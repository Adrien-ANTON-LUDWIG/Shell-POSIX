#ifndef UTILS_H
#define UTILS_H

#include <ctype.h>
#include <err.h>
#include <stddef.h>
#include <stdlib.h>

#include "structures.h"

/* MAJOR */
void major_init(void);
void major_free(struct major *mj);

/* MY_ERR */
void my_err(int err_code, struct major *mj, char *message);

/* LIST */
struct list
{
    struct list_item *head;
    struct list_item *tail;
    size_t size;
};

struct list_item
{
    struct list_item *next;
    char *data;
};

#endif /* UTILS_H */