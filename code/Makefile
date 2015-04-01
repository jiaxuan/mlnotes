srcs = atomic32.cpp Condition.cpp DecimalDecNumber.cpp decContext.cpp decNumber.cpp Format.cpp InterruptibleSleep.cpp Logger.cpp Mutex.cpp StringBuilder.cpp StringTools.cpp Time.cpp Thread.cpp

CPPFLAGS = -std=c++11 -I/usr/include/apr-1.0 -I$(BOOST_INSTALL_DIR)/include -I$(APR_INSTALL_DIR)/include/apr-1 -m32
# LDFLAGS = /usr/lib/x86_64-linux-gnu/libapr-1.a
LDFLAGS = /home/fxdev/apps/gcc-4.9.1-compliant/apr/apr-1.5.1/lib/libapr-1.a -lpthread -m32

objs = $(srcs:.cpp=.o)

base.a: $(objs)
	rm -rf $@
	ar cr $@ $(objs)
	ranlib $@

all: base.a

test_decimal: test_decimal.cpp base.a
	g++ $(CPPFLAGS) test_decimal.cpp -o test_decimal base.a $(LDFLAGS)

