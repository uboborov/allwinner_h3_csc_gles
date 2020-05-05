#PATH=$PATH:/usr/local/arm/arm-linux-gnueabihf/bin/
#BIN_PATH = /home/ubobrov/develop/projects/intercom/rootfs/root

CROSS_COMPILE ?=

CC = gcc

CP = /usr/bin/sudo /bin/cp
DEL = /bin/rm -f

TARGET = csc_app

SRC = main.c color.c


CFLAGS = -Wall -O2 -I. -I./include/wayland -I/usr/include/drm

LIBDIR= -L/usr/lib/mali
LDFLAGS=
LIBS= -ldrm -lgbm -lEGL -lGLESv2

MAKEFLAGS += -rR --no-print-directory

DEP_CFLAGS = -MD -MP -MQ $@
DEFS = -DCPU_HAS_NEON
MAKEFILE = Makefile

OBJ = $(addsuffix .o,$(basename $(SRC)))
DEP = $(addsuffix .d,$(basename $(SRC)))

.PHONY: clean all

all: $(TARGET) $(MAKEFILE)
$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o $@ $(LIBDIR)
#	-$(CP) $(TARGET) $(BIN_PATH)

clean:
	$(DEL) $(OBJ)
	$(DEL) $(DEP)
	$(DEL) $(TARGET)

%.o: %.c
	$(CC) $(DEP_CFLAGS) $(DEFS) $(CFLAGS) -c $< -o $@

%.o: %.S $(MKFILE) $(LDSCRIPT)
	$(CC) -c $< -o $@	

include $(wildcard $(DEP))
