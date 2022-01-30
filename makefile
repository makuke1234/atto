CC=gcc
WARN=-Wall -Wextra -Wpedantic -Wconversion -Wunused-variable -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wdouble-promotion -Waggregate-return
CDEFFLAGS=-std=c99 $(WARN) -municode -m32 -D UNICODE -D _UNICODE
CFLAGS=-O3 -Wl,--strip-all,--build-id=none,--gc-sections -fno-ident -D NDEBUG
CFLAGSD=-g -O0 -D PROFILING_ENABLE=1

TARGET=atto
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
	rm -r -f $(OBJ)
	rm -r -f $(OBJD)
	rm -f $(TARGET).exe
	rm -f deb$(TARGET).exe
