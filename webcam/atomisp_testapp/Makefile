CC=gcc -g
CC_ARCH=-m32
override CFLAGS+=-c -Wall -Wno-write-strings -DAPP_BUILD_DATE=$(shell date +"%Y-%m-%d")
override INCLUDES+=-I./src -I/usr/include/libdrm -I/usr/include
override LIBS+= -lEGL -lGLESv2 -lm -ldrm -ldrm_intel
EXECUTABLE=isp-mipi-test

override SOURCES+= \
src/utilities.c \
src/str_struct.c \
src/log.c \
src/video.c \
src/shader.c

OBJECTS+=$(SOURCES:.c=.o)

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CC_ARCH) $(INCLUDES) -o $@ $(OBJECTS) $(LIBS)

.c.o:
	$(CC) $(CC_ARCH) $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -fR src/*o $(EXECUTABLE)
