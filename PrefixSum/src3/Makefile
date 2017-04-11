ifndef NUM_THREADS
	NUM_THREADS = 16
endif

ifndef SIZE
	SIZE = 8192
endif

CXXFLAGS := $(CXXFLAGS)  -I/home/wvf2/lib/wireless/ -L/home/wvf2/lib/wireless/ -static -m32
CXXFLAGS += -DNUM_THREADS=${NUM_THREADS} -DSIZE=${SIZE}

LDFLAGS := $(LDFLAGS) -lpthread -lwireless

prefix : PrefixSum.cpp
	clang++ ${CXXFLAGS} $<  -o $@ $(LDFLAGS)

clean :
	rm -f prefix
