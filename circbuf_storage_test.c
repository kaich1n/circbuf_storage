#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "circbuf_storage.h"

struct test_data {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
};

void test_write(circbuf_storage_handle_t *handle, int from, int to)
{
    struct test_data data;

    for (int i = from; i < to; i++) {
        data.a = i;
        data.b = i + 1;
        data.c = i + 2;
        data.d = i + 3;

        printf("write %d, data.a = %d\n", i, data.a);
        assert(circbuf_storage_push(handle, &data) == 0);
    }
}

void verify_read(circbuf_storage_handle_t *handle, int from, int to)
{
    struct test_data data = { 0 };

    for (int i = from; i < to; i++) {
        assert(circbuf_storage_peek(handle, &data) == 0);
        printf("read %d, data.a = %d\n", i, data.a);
        assert(data.a == i);
        assert(data.b == i + 1);
        assert(data.c == i + 2);
        assert(data.d == i + 3);
        circbuf_storage_consume(handle, 1);
    }
}


void test_write_read(circbuf_storage_handle_t *handle, int from, int to)
{
    struct test_data data;

    for (int i = from; i < to; i++) {
        // write
        data.a = i;
        data.b = i + 1;
        data.c = i + 2;
        data.d = i + 3;

        printf("write %d, data.a = %d\n", i, data.a);
        assert(circbuf_storage_push(handle, &data) == 0);

        // read
        memset(&data, 0x0, sizeof(data));
        assert(circbuf_storage_peek(handle, &data) == 0);
        printf("read %d, data.a = %d\n", i, data.a);
        assert(data.a == i);
        assert(data.b == i + 1);
        assert(data.c == i + 2);
        assert(data.d == i + 3);
        circbuf_storage_consume(handle, 1);
    }
}

void test_push_pop(circbuf_storage_handle_t *handle, int from, int to)
{
    struct test_data data;

    for (int i = from; i < to; i++) {
        // write
        data.a = i;
        data.b = i + 1;
        data.c = i + 2;
        data.d = i + 3;

        printf("write %d, data.a = %d\n", i, data.a);
        assert(circbuf_storage_push(handle, &data) == 0);

        // read
        memset(&data, 0x0, sizeof(data));
        assert(circbuf_storage_pop(handle, &data) == 0);
        printf("read %d, data.a = %d\n", i, data.a);
        assert(data.a == i);
        assert(data.b == i + 1);
        assert(data.c == i + 2);
        assert(data.d == i + 3);
    }
}

#define NMEMB 1000

const char *datafile = "./testdata";

int main(int argc, char **argv)
{
    unlink(datafile);

    circbuf_storage_handle_t *handle = circbuf_storage_handle_create(datafile, NMEMB, sizeof(struct test_data));
    assert(handle);

    // empty
    struct test_data data;
    assert(circbuf_storage_peek(handle, &data) < 0);

    test_write(handle, 0, NMEMB);
    verify_read(handle, 0, NMEMB);

    printf("%d: ----\n", __LINE__);

    test_write(handle, 0, NMEMB * 2);
    verify_read(handle, NMEMB, NMEMB * 2);

    printf("%d: ----\n", __LINE__);

    test_push_pop(handle, 100, 1000);

    printf("%d: ----\n", __LINE__);

    test_write_read(handle, 0, 1234);

    printf("%d: ----\n", __LINE__);

    test_write(handle, 0, 1234);

    circbuf_storage_handle_destroy(handle);

    printf("%d: ----\n", __LINE__);

    circbuf_storage_handle_t *handle2 = circbuf_storage_handle_create(datafile, NMEMB, sizeof(struct test_data));
    assert(handle2);

    verify_read(handle2, (((1234 - NMEMB) / NMEMB) * NMEMB) + 1234 % NMEMB, 1234);

    printf("%d: ----\n", __LINE__);

    // empty
    assert(circbuf_storage_peek(handle2, &data) < 0);

    circbuf_storage_handle_destroy(handle2);
    return 0;
}
