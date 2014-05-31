CC = gcc

REQUIRED_PKG = vconf ecore
CFLAGS = -g -Wall
LDFLAGS =
ifdef REQUIRED_PKG
	CFLAGS += `pkg-config --cflags $(REQUIRED_PKG)`
	LDFLAGS += `pkg-config --libs $(REQUIRED_PKG)`
endif

SRCS = vconf_notitest.c vconf_timecheck.c vconf_unit_test.c
OBJECTS = $(SRCS:.c=.o)
TARGETS = $(OBJECTS:.o=)


all: $(TARGETS)

% : %.o
	$(CC) $(LDFLAGS) -o $@ $<

clean:
	rm -rf $(OBJECTS) $(TARGETS)

