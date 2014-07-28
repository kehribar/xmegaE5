# try to do some autodetecting
UNAME := $(shell uname -s)

ifeq "$(UNAME)" "Darwin"
	OS=macosx
endif
ifeq "$(OS)" "Windows_NT"
	OS=windows
endif
ifeq "$(UNAME)" "Linux"
	OS=linux
endif


#################  Mac OS X  ##################################################
ifeq "$(OS)" "macosx"

EXE_SUFFIX=

# ARCHS=   -arch i386 -arch x86_64
# CFLAGS+= $(ARCHS)
# CFLAGS += -mmacosx-version-min=10.6
# LIBS+=	 $(ARCHS)

endif

#################  Windows  ##################################################
ifeq "$(OS)" "windows"

EXE_SUFFIX=.exe

endif

#################  Common  ##################################################

CFLAGS += $(INCLUDES) -O -Wall -std=gnu99

TARGET = main

all: $(TARGET)

$(TARGET): $(TARGET).o serial_lib.o
	$(CC) $(CFLAGS) -o $(TARGET)$(EXE_SUFFIX) $(TARGET).o serial_lib.o $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

clean:
	rm -f $(OBJ) $(TARGET)$(EXE_SUFFIX) *.o *.a

commit:
	make clean && git commit -a

run:
	make clean && make && ./main
