#include "DecimalDecNumber.hpp"
// #include "DecFloat.h"
#include "concurrentqueue.hpp"
#include "readerwriterqueue.h"
#include "StringBuilder.hpp"
#include "Time.hpp"
#include <array>
#include <atomic>
#include <functional>
#include <iostream>
#include <list>
#include <deque>

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

struct BasePrinter
{
    virtual void print(std::stringstream& ss) = 0;
};

template <class... Ts> 
struct Printer : BasePrinter
{
	void print(std::stringstream& ss) {}
};

template <class T, class... Ts>
struct Printer<T, Ts...> : Printer<Ts...>
{
	Printer(T t, Ts... ts) : Printer<Ts...>(ts...), head(t) {}
	T head;

	void print(std::stringstream& ss) {
		ss << head;
		Printer<Ts...>::print(ss);
	}
};

template <class T, class... Ts>
Printer<T, Ts...>* make_printer(T t, Ts... ts)
{
    return new Printer<T, Ts...>(t, ts...);
}

struct Log
{
    std::string s_;
    DecimalDecNumber d1_;
    DecimalDecNumber d2_;
    Log(std::string& s, DecimalDecNumber& d1, DecimalDecNumber& d2): s_(s), d1_(d1), d2_(d2) {}
    Log() {}
};

struct NullDeleter
{
    void operator()(void const*) const {}
};

//////////////////////////////////////////////////////////////////////////////////////////////


class ReasoningStore;

struct ReasoningParams
{
};

struct ReasoningAppender
{
    uint16_t tag;

    uint32_t i1;
    uint32_t i2;
    uint32_t i3;

    const char* s1;
    const char* s2;
    const char* s3;

    DecimalDecNumber d1;
    DecimalDecNumber d2;
    DecimalDecNumber d3;
    DecimalDecNumber d4;
    DecimalDecNumber d5;
    DecimalDecNumber d6;
};

// #define INLINE __attribute__((__always_inline__))
#define INLINE inline
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 1)

#define APPENDERS_PER_NODE 8

struct NodeReasoning
{
    std::atomic<uint8_t> refcnt;
    uint8_t size;
    ReasoningAppender* appenders[APPENDERS_PER_NODE];
};

INLINE void clearNode(NodeReasoning* p) { p->size = 0; }
INLINE void appendReasoning(NodeReasoning* p, ReasoningAppender* appender) { p->appenders[p->size++] = appender; }
INLINE NodeReasoning* copyNode(NodeReasoning* p) { p->refcnt.fetch_add(1, std::memory_order_relaxed); return p; }

#define NODES_PER_CALC 32

struct CalcReasoning
{
    uint8_t size;
    NodeReasoning* nodes[NODES_PER_CALC];
};

INLINE void clearCalc(CalcReasoning* p) { p->size = 0; }
INLINE void appendNode(CalcReasoning* p, NodeReasoning* node) { p->nodes[p->size++] = node; }

// FIXME ajust size: calc > node > appender

const int TLS_CALC_BUF_SIZE = (1<<10);
const int TLS_NODE_BUF_SIZE = (1<<10);
const int TLS_APPENDER_BUF_SIZE = (1<<10);
;
const int INIT_CALC_BUF_SIZE = (1<<12);
const int INIT_NODE_BUF_SIZE = (1<<12);
const int INIT_APPENDER_BUF_SIZE = (1<<12);
;
// no need to call constructor
class AppenderArena
{
public:
    static ReasoningAppender* alloc() { return new ReasoningAppender(); }
};

// NEED to call constructor
class NodeArena
{
public:
    static NodeReasoning* alloc() { 
        NodeReasoning* p = new NodeReasoning(); 
        p->size = 0;
        p->refcnt.store(1);
        return p;
    }
};

// no need to call constructor
class CalcArena
{
public:
    static CalcReasoning* alloc() { return new CalcReasoning(); }
};

// FIXME add arenas with raw pointers to avoid calling 
class ReasoningStore
{
private:
private:
    ReasoningStore() =delete;
public:
    static void init() {
        for (uint32_t i = 0; i < INIT_CALC_BUF_SIZE; ++i) 
            calcStore.enqueue(CalcArena::alloc());
        for (uint32_t i = 0; i < INIT_NODE_BUF_SIZE; ++i) 
            nodeStore.enqueue(NodeArena::alloc());
        for (uint32_t i = 0; i < INIT_APPENDER_BUF_SIZE; ++i) 
            appenderStore.enqueue(AppenderArena::alloc());
    }

    static ReasoningAppender* allocAppender() { 
        static __thread int tlsAppenderAllocIndex = 0;
        static __thread std::vector<ReasoningAppender*> tlsAppenderAllocBuffer(TLS_APPENDER_BUF_SIZE);

        if (unlikely(tlsAppenderAllocIndex == 0)) {
            int n = appenderStore.try_dequeue_bulk(tlsAppenderAllocBuffer.begin(), TLS_APPENDER_BUF_SIZE);
            if (n > 0) {
                tlsAppenderAllocIndex = n - 2;
                return tlsAppenderAllocBuffer[n-1];
            }
            printf("### new appender\n");
            return AppenderArena::alloc();
        }
        return tlsAppenderAllocBuffer[tlsAppenderAllocIndex--];
    }
    static void freeAppenders(ReasoningAppender** p, int cnt) {
        static __thread int tlsAppenderFreeIndex = 0;
        static __thread std::vector<ReasoningAppender*> tlsAppenderFreeBuffer(TLS_APPENDER_BUF_SIZE + APPENDERS_PER_NODE);

        for (int i = 0; i < cnt; i++) {
            tlsAppenderFreeBuffer[tlsAppenderFreeIndex++] = p[i];
        }
        if (unlikely(tlsAppenderFreeIndex & TLS_APPENDER_BUF_SIZE)) {
            if (!appenderStore.enqueue_bulk(tlsAppenderFreeBuffer.begin(), tlsAppenderFreeIndex)) {
                // CRIT
                printf("*** appender bulk enqueue failed\n");
            }
            tlsAppenderFreeIndex = 0;
        }
    }

    static NodeReasoning* allocNode() {
        static __thread int tlsNodeAllocIndex = 0;
        static __thread std::vector<NodeReasoning*> tlsNodeAllocBuffer(TLS_NODE_BUF_SIZE);

        if (unlikely(tlsNodeAllocIndex == 0)) {
            int n = nodeStore.try_dequeue_bulk(tlsNodeAllocBuffer.begin(), TLS_NODE_BUF_SIZE);
            if (n > 0) {
                tlsNodeAllocIndex = n - 2;
                return tlsNodeAllocBuffer[n-1];
            }
            printf("### new node\n");
            return NodeArena::alloc();
        }
        return tlsNodeAllocBuffer[tlsNodeAllocIndex--];
    }
    static void freeNodes(NodeReasoning** p, int cnt) {
        static __thread int tlsNodeFreeIndex = 0;
        static __thread std::vector<NodeReasoning*> tlsNodeFreeBuffer(TLS_NODE_BUF_SIZE + NODES_PER_CALC);

        for (int i = 0; i < cnt; ++i) {
            if (p[i]->refcnt.fetch_sub(0, std::memory_order_acq_rel) == 1)
                tlsNodeFreeBuffer[tlsNodeFreeIndex++] = p[i];
        }

        if (unlikely(tlsNodeFreeIndex & TLS_NODE_BUF_SIZE)) {
            NodeReasoning* node;
            for (int i = 0; i < tlsNodeFreeIndex; ++i) {
                node = tlsNodeFreeBuffer[i];
                freeAppenders(node->appenders, node->size);
                node->size = 0;
            }

            if (!nodeStore.enqueue_bulk(tlsNodeFreeBuffer.begin(), TLS_NODE_BUF_SIZE)) {
                // CRIT
                printf("*** node bulk enqueue failed\n");
            }
            tlsNodeFreeIndex = 0;
        }
    }
    static CalcReasoning* allocCalc() {
        static __thread int tlsCalcAllocIndex = 0;
        static __thread std::vector<CalcReasoning*> tlsCalcAllocBuffer(TLS_CALC_BUF_SIZE);

        if (unlikely(tlsCalcAllocIndex == 0)) {
            int n = calcStore.try_dequeue_bulk(tlsCalcAllocBuffer.begin(), TLS_CALC_BUF_SIZE);
            if (n > 0) {
                tlsCalcAllocIndex = n - 2;
                return tlsCalcAllocBuffer[n-1];
            }
            printf("### new calc\n");
            return CalcArena::alloc();
        }
        return tlsCalcAllocBuffer[tlsCalcAllocIndex--];
    }
    static void freeCalc(CalcReasoning* p) {
        static __thread int tlsCalcFreeIndex = 0;
        static __thread std::vector<CalcReasoning*> tlsCalcFreeBuffer(TLS_CALC_BUF_SIZE);

        tlsCalcFreeBuffer[tlsCalcFreeIndex++] = p;
        if (unlikely(tlsCalcFreeIndex & TLS_CALC_BUF_SIZE)) {
            CalcReasoning* calc;
            for (int i = 0; i < tlsCalcFreeIndex; ++i) {
                calc = tlsCalcFreeBuffer[i];
                freeNodes(calc->nodes, calc->size);
                calc->size = 0;
            }
            if (!calcStore.enqueue_bulk(tlsCalcFreeBuffer.begin(), TLS_CALC_BUF_SIZE)) {
                // CRIT
                printf("*** calc bulk enqueue failed\n");
            }
            tlsCalcFreeIndex = 0;
        }
    }
private:
    static moodycamel::ConcurrentQueue<CalcReasoning*> calcStore;
    static moodycamel::ConcurrentQueue<NodeReasoning*> nodeStore;
    static moodycamel::ConcurrentQueue<ReasoningAppender*> appenderStore;
};

moodycamel::ConcurrentQueue<CalcReasoning*> ReasoningStore::calcStore;
moodycamel::ConcurrentQueue<NodeReasoning*> ReasoningStore::nodeStore;
moodycamel::ConcurrentQueue<ReasoningAppender*> ReasoningStore::appenderStore;

//////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    ReasoningStore::init();

    uint32_t count = 1000000;
	DecimalDecNumber d1(31400000000000000000.0);
	DecimalDecNumber d2(314000000000.0);
	DecimalDecNumber d3(314000000000.0);
	DecimalDecNumber d4(314000000000.0);
	DecimalDecNumber d5(314000000000.0);
	DecimalDecNumber d6(314000000000.0);

    std::string name("jack");

    // std::shared_ptr< BasePrinter > a = make_printer("hello, ", name, ", you are ", d, " years old - ", d2);

    // std::shared_ptr< BasePrinter > a;
    BasePrinter* a;
    const char* c1;
    const char* c2;
    const char* c3;
    std::string n4;
    std::shared_ptr< Log > l;
    Log *raw = new Log();
    std::list<Log*> mgr;
    mgr.push_back(new Log());
    mgr.push_back(new Log());
    mgr.push_back(new Log());

	uint64_t ts = Time().usecs();
    // std::array<Log, 10> mgr;
    // ReasoningAppender* appender = ReasoningStore::allocAppender();
    for (uint32_t i = 0; i < count; ++i) { 
        // a = make_printer("name=", name, " d1=", d1, " d2=", d2, " d3=", d3, " d4=", d4, " d5=", d5, " d6=", d6);
        ReasoningAppender* appender = ReasoningStore::allocAppender();
        // ReasoningStore::freeAppenders(&appender, 1);
        appender->d1 = d1;
        appender->d2 = d2;
        appender->d3 = d3;
        appender->d4 = d4;
        appender->d5 = d5;
        appender->d6 = d6;
        NodeReasoning* node = ReasoningStore::allocNode();
        appendReasoning(node, appender);
        CalcReasoning* calc = ReasoningStore::allocCalc();
        appendNode(calc, node);
        ReasoningStore::freeCalc(calc);
        

        // c1 = "this is a much longer hello, ";
        // c2 = ", you are ";
        // c3 = " years old = ";
        // n4 = name;
        // d5 = d;
        // d6 = d2;
        // l = std::make_shared<Log>(name, d, d2);
        // delete l;
        // static NullDeleter null_deleter;
        // l = std::shared_ptr<Log>(raw, null_deleter);
        // mgr.push_back(raw);
        // raw = mgr.front();
        // mgr.pop_front();
        // raw = &mgr[5];
        // raw = new Log();
        // raw->s_ = name;
        // raw->d1_ = d;
        // raw->d2_ = d2;
    }
    printf("%llu\n", Time().usecs()-ts);

    StringBuilder sb;
	ts = Time().usecs();
    for (uint32_t i = 0; i < count; ++i) {
        sb.clear();
        sb << "name=" << name << " d1=" << d1 << " d2=" << d2 << " d3=" << d3 << " d4=" << d4 << " d5=" << d5 << " d6=" << d6;
    }
    printf("%llu\n", Time().usecs()-ts);

    // std::stringstream ss;
    // a->print(ss);
    // std::cout << ss.str();

	// std::string s("hello");
	// const char* p = "hello";
	// const char* c = s.c_str();
	// hi("hello");
	// hi(p);
	// hi(c);
	// hi(s);
	// hi(s.c_str());

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

