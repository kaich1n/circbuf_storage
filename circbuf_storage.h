#ifndef _CIRCBUF_STORAGE_H_
#define _CIRCBUF_STORAGE_H_

#include <stdbool.h>

typedef struct circbuf_storage_handle circbuf_storage_handle_t;

circbuf_storage_handle_t *circbuf_storage_handle_create(const char *path, size_t cap, size_t memb_size);

int circbuf_storage_push(circbuf_storage_handle_t *handle, void *data);

int circbuf_storage_pop(circbuf_storage_handle_t *handle, void *data);

int circbuf_storage_peek(circbuf_storage_handle_t *handle, void *data);

void circbuf_storage_consume(circbuf_storage_handle_t *handle, size_t n);

void circbuf_storage_handle_destroy(circbuf_storage_handle_t *handle);

#endif // _CIRCBUF_STORAGE_H_

