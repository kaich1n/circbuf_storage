#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "circbuf_storage.h"

#ifndef ERROR
#define ERROR printf
#endif

#define FILE_STORAGE_HEAD_LOC   0x00
#define FILE_STORAGE_HEAD_MAGIC 0x44414548

#define FILE_STORAGE_ITEM_LOC   0x1000

typedef struct circbuf_storage_head {
    uint32_t magic;
    uint32_t cap;
    uint32_t memb_size;
    uint32_t push_count;
    uint32_t pop_count;
} circbuf_storage_head_t;

typedef struct circbuf_storage_handle {
    int fd;
    bool loopback;
    circbuf_storage_head_t head;
} circbuf_storage_handle_t;

#define print_error() 

#define RETURN_ERROR(rval) {                            \
    fprintf(stderr, "%s %d: %s\n",                      \
            __FUNCTION__, __LINE__, strerror(errno));   \
    return rval;                                        \
}                                                       \

static inline int file_open(const char *path)
{
    int fd;

    if (!path)
        return -1;

    if ((fd = open(path, O_RDWR | O_CREAT, 0644)) < 0)
        RETURN_ERROR(-1);

    return fd;
}

static inline int file_trunc(const char *path, int fd)
{
    if (!path)
        return -1;

    if (fd > 0)
        close(fd);

    if ((fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0)
        RETURN_ERROR(-1);

    return fd;
}

static inline int file_read(int fd, size_t pos, void *data, size_t size)
{
    if (lseek(fd, pos, SEEK_SET) == -1)
        RETURN_ERROR(-1);

    return read(fd, data, size);
}

static inline int file_write(int fd, size_t pos, void *data, size_t size)
{
    if (lseek(fd, pos, SEEK_SET) == -1)
        RETURN_ERROR(-1);

    return write(fd, data, size);
}

circbuf_storage_handle_t *circbuf_storage_handle_create(const char *path, size_t cap, size_t memb_size)
{
    if (!path)
        return NULL;

    circbuf_storage_handle_t *handle = (circbuf_storage_handle_t *) calloc(1, sizeof(circbuf_storage_handle_t));
    assert(handle);

    if ((handle->fd = file_open(path)) > 0) {
        uint8_t reset = 1;

        do {
            if (file_read(handle->fd, FILE_STORAGE_HEAD_LOC, &handle->head, sizeof(circbuf_storage_head_t)) != sizeof(circbuf_storage_head_t))
                break;

            if (handle->head.magic != FILE_STORAGE_HEAD_MAGIC) {
                ERROR("illegal head magic\n");
                break;
            }

            if (handle->head.cap != cap) {
                ERROR("unmatched capabilities\n");
                break;
            }

            if (handle->head.memb_size != memb_size) {
                ERROR("unmatched member element size\n");
                break;
            }

            reset = 0;
        } while (0);
 
        if (reset) {
            handle->fd = file_trunc(path, handle->fd);
            handle->head.magic = FILE_STORAGE_HEAD_MAGIC;
            handle->head.cap = cap;
            handle->head.memb_size = memb_size;
            handle->head.push_count = handle->head.pop_count = 0;
            if (file_write(handle->fd, FILE_STORAGE_HEAD_LOC, &handle->head, sizeof(circbuf_storage_head_t)) != sizeof(circbuf_storage_head_t)) {
                ERROR("failed to initial file storage\n");
                circbuf_storage_handle_destroy(handle);
                return NULL;
            }
        }
    };

    return handle;
}

void circbuf_storage_handle_destroy(circbuf_storage_handle_t *handle)
{
    if (handle) {
        if (handle->fd)
            close(handle->fd);
   
        free(handle);
    }
}

static inline int _circbuf_storage_pop(circbuf_storage_handle_t *handle, void *elem, int read_only)
{
    int total;
    uint32_t tail;

    total = handle->head.push_count - handle->head.pop_count;
    // total %= handle->head.cap;
    if (total < 0)
        total += (2 * handle->head.cap);

    if (total == 0)
        return -1; // Empty

    tail = FILE_STORAGE_ITEM_LOC + ((handle->head.pop_count % handle->head.cap) * handle->head.memb_size);

    if (elem) {
        if (file_read(handle->fd, tail, elem, handle->head.memb_size) != handle->head.memb_size)
            RETURN_ERROR(-1);
    }

    if (!read_only) {
        // memset(tail, 0, handle->head.memb_size);
        handle->head.pop_count++;
        if (handle->head.pop_count >= (2 * handle->head.cap))
            handle->head.pop_count = 0;
    }
    return 0;
}

int circbuf_storage_push(circbuf_storage_handle_t *handle, void *data)
{
    int total;
    uint32_t head;

    if (!handle || !data)
        return -1;

    total = handle->head.push_count - handle->head.pop_count;
    if (total < 0)
        total += (2 * handle->head.cap);

    if (total >=  handle->head.cap) {
        // full, pop last element first
        _circbuf_storage_pop(handle, NULL, 0);
    }

    head = FILE_STORAGE_ITEM_LOC + ((handle->head.push_count % handle->head.cap) * handle->head.memb_size);

    file_write(handle->fd, head, data, handle->head.memb_size);

    handle->head.push_count++;
    if (handle->head.push_count >= (2 * handle->head.cap))
        handle->head.push_count = 0;

    file_write(handle->fd, FILE_STORAGE_HEAD_LOC, &handle->head, sizeof(circbuf_storage_head_t));
    return 0;
}

int circbuf_storage_pop(circbuf_storage_handle_t *handle, void *data)
{
    return _circbuf_storage_pop(handle, data, 0);
}

int circbuf_storage_peek(circbuf_storage_handle_t *handle, void *data)
{
    return _circbuf_storage_pop(handle, data, 1);
}

void circbuf_storage_consume(circbuf_storage_handle_t *handle, size_t n)
{
    // pop and discard the elem data
    for (size_t i = 0; i < n; i++)
        _circbuf_storage_pop(handle, NULL, 0);
}

