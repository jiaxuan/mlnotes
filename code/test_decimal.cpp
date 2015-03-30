#include "DecimalDecNumber.hpp"
#include "DecFloat.h"
#include "Time.hpp"
#include <functional>



int doSomething(int flag, int flag2, int n) { return n+1; }
int doSomethingElse(int n) { return n+2; }

typedef int (*PF)(int);

struct Test
{
	void setFlag(int flag) {
		m_flag = flag;
		if (m_flag & 1)
			m_func = std::bind(&Test::doSomething, this, std::placeholders::_1);
			// m_func = doSomething;
		else
			m_func = std::bind(&Test::doSomethingElse, this, std::placeholders::_1);
			// m_func = doSomethingElse;
	}

	int fun1(int n) {
		if (m_flag & 1) {
			return doSomething(n);
		} else {
			return doSomethingElse(n);
		}
	}

	int doSomething(int n) { return n+1; }
	int doSomethingElse(int n) { return n+2; }


	int fun2(int n) {
		return m_func(n);
	}

private:
	int m_flag;
	std::function<int(int)> m_func;
	// PF m_func;
};

template <typename T>
void hi(T&& v) 
{
	if (std::is_convertible<decltype(v), const char *>::value 
		&& !std::is_rvalue_reference<decltype(v)>::value 
		&& !std::is_pointer<decltype(v)>::value 
		&& !std::is_array<decltype(v)>::value 
		&& !std::is_class<decltype(v)>::value
		) {
		printf("const string\n");
	} else {
		printf("value\n");
	}
}

template <class... Ts> 
struct Printer 
{
	void print(std::stringstream& ss) {}
};

template <class T, class... Ts>
struct Printer<T, Ts...> : Printer<Ts...>
{
	Printer(T t, Ts... ts) : Printer<Ts...>(ts...), tail(t) {}
	T tail;

	void print(std::stringstream& ss) {
		Printer<Ts...>::print(ss);
		ss << tail;
	}
};

int main(int argc, char* argv[])
{
	std::string s("hello");
	const char* p = "hello";
	const char* c = s.c_str();
	hi("hello");
	hi(p);
	hi(c);
	hi(s);
	hi(s.c_str());

	// printf("%lu\n", sizeof("hello"));
    //
	// Test t;
	// int flag = 2;
	// t.setFlag(flag);
	// uint32_t total = 0;
	// uint32_t count = 100000000;
    //
	// uint64_t ts = Time().usecs();
	// for (uint32_t i = 0; i < count; ++i) {
	// 	total += doSomething(flag, flag, i);
	// }
	// printf("%lu\n", Time().usecs() - ts);
    //
	// ts = Time().usecs();
	// total = 0;
	// auto f = boost::bind(&Test::doSomething, t, _1);
	// for (uint32_t i = 0; i < count; ++i) {
	// 	total += f(i);
	// }
	// printf("%lu\n", Time().usecs() - ts);


	// DecimalDecNumber.toString has a lot of space for improvement
	// DecimalDecNumber d(31400000000000000000.0);
	// std::string s = d.toString();
	// printf("== s: %s\n", s.c_str());


	// DecimalDecNumber d2;
    //
	// uint32_t count = 1000000;
	// uint64_t ts = Time().usecs();
	// for (uint32_t i = 0; i < count; ++i) {
	// 	d = d2;
	// }
	// printf("%lu\n", Time().usecs()-ts);
    //
	// std::string s;
	// ts = Time().usecs();
	// for (uint32_t i = 0; i < count; ++i) {
	// 	s = d2.toString();
	// }
	// printf("%lu\n", Time().usecs()-ts);
    //
	// DecFloat_ d3("3.1415926");
	// DecFloat_ d4;
	// std::string s3;
	// ts = Time().usecs();
	// for (uint32_t i = 0; i < count; ++i) {
	// 	// s3 = d3.toStr();
	// 	d4 = d3;
	// }
	// printf("%lu\n", Time().usecs()-ts);


	return 0;
}

