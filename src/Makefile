
all: libotio.a otio_test

clean:
	rm libotio.a OTIO_io.o OTIO_main.o assemble_json.o otio_test

libotio.a: OTIO_io.c OTIO_main.c OTIO_internal.h assemble_json.h assemble_json.c
	cc -c OTIO_io.c OTIO_main.c assemble_json.c
	ar rcs libotio.a OTIO_io.o OTIO_main.o assemble_json.o

otio_test: OTIO_test.c libotio.a
	cc -o otio_test otio_test.c -L. -lotio

test: otio_test
	./otio_test
