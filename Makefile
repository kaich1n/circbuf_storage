circbuf_storage_test: circbuf_storage_test.c circbuf_storage.c
	gcc -Wall -g -o $@ $^

clean:
	-@rm -rf *.o circbuf_storage_test testdata*

.PHONY: circbuf_storage_test clean
