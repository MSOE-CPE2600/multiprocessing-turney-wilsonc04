CC       = gcc
CFLAGS   = -c -Wall -g -pthread
LDFLAGS  = -ljpeg -lm -lpthread

SOURCES  = mandel.c mandelmovie.c jpegrw.c
OBJECTS  = $(SOURCES:.c=.o)
EXECUTABLES = mandel movie

all: $(EXECUTABLES)

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

# link mandel (uses mandel.c + jpegrw.c)
mandel: mandel.o jpegrw.o
	$(CC) $^ $(LDFLAGS) -o $@

# link movie (mandelmovie.c + jpegrw.c)
movie: mandelmovie.o jpegrw.o
	$(CC) $^ $(LDFLAGS) -o $@

# compile .c -> .o and generate .d dependency file
.c.o:
	$(CC) $(CFLAGS) $< -o $@
	$(CC) -MM $< > $*.d

clean:
	rm -rf $(OBJECTS) $(EXECUTABLES) *.d
