srcs = atomic32.cpp Condition.cpp DecimalDecNumber.cpp decContext.cpp decNumber.cpp Format.cpp InterruptibleSleep.cpp Logger.cpp Mutex.cpp StringBuilder.cpp StringTools.cpp Time.cpp Thread.cpp

CPPFLAGS = -std=c++11 -I/usr/include/apr-1.0
LDFLAGS = /usr/lib/x86_64-linux-gnu/libapr-1.a

objs = $(srcs:.cpp=.o)

base.a: $(objs)
	rm -rf $@
	ar cr $@ $(objs)
	ranlib $@

all: base.a

test_decimal: test_decimal.cpp base.a
	g++ $(CPPFLAGS) test_decimal.cpp -o test_decimal base.a $(LDFLAGS)

