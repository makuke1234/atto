CC=gcc
CDEFFLAGS=-std=c99 -Wall -Wextra -Wpedantic -Wconversion -Wunused-variable -municode -D PROFILING_ENABLE=1
CFLAGS=-O3 -Wl,--strip-all,--build-id=none -fno-ident
CFLAGSD=-g -O0

TARGET=pico
OBJ=obj
OBJD=objd
SRC=src

default: debug

$(OBJ):
	mkdir $(OBJ)
$(OBJD):
	mkdir $(OBJD)

srcs = $(wildcard $(SRC)/*.c)
#srcs += $(wildcard $(SRC)/*.rc)
srcs := $(subst $(SRC)/,,$(srcs))
objs_d = $(srcs:%=$(OBJD)/%.o)
objs_r = $(srcs:%=$(OBJ)/%.o)


$(OBJ)/%.c.o: $(SRC)/%.c
	$(CC) -c $^ -o $@ $(CDEFFLAGS) $(CFLAGS)
$(OBJD)/%.c.o: $(SRC)/%.c
	$(CC) -c $^ -o $@ $(CDEFFLAGS) $(CFLAGSD)

debug: $(OBJD) debug_obj
debug_obj: $(objs_d)
	$(CC) $^ -o deb$(TARGET).exe $(CDEFFLAGS) $(CFLAGSD)

release: $(OBJ) release_obj
release_obj: $(objs_r)
	$(CC) $^ -o $(TARGET).exe $(CDEFFLAGS) $(CFLAGS)

clean:
	IF EXIST $(OBJ) rd /s /q $(OBJ)
	IF EXIST $(OBJD) rd /s /q $(OBJD)
	del $(TARGET).exe
	del deb$(TARGET).exe
