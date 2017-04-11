ifndef NUM_THREADS
	NUM_THREADS = 16
endif

ifndef SIZE
	SIZE = 100
endif

CXXFLAGS := $(CXXFLAGS)  -I../../libwireless/ -L../../libwireless/ -static -m32
CXXFLAGS += -DNUM_THREADS=${NUM_THREADS} -DSIZE=${SIZE}

LDFLAGS := $(LDFLAGS) -lpthread -lwireless

is : is.c
	clang++ ${CXXFLAGS} $<  -o $@ $(LDFLAGS)

clean :
	rm -f prefix