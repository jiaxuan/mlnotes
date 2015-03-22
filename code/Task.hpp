
#ifndef __COMMON_CONCURRENT_TASK_HPP__
#define __COMMON_CONCURRENT_TASK_HPP__

#include <common/common.hpp>
#include <common/concurrent/Thread.hpp>
#include <common/exception/Exception.hpp>

// ----------------------------------------------------------------------------------- 
// ml::common::concurrent
// ----------------------------------------------------------------------------------- 
 
namespace ml {
namespace common {
namespace concurrent {

/** ----------------------------------------------------------------------------------
 * Simple threaded task implementation.
 * 
 * @warning The destructor of this object will signal a stop on the background thread
 * 			then wait for it to complete before returning.
 * 
 * Sample code:
 * <code>
 * 
 * struct Foo {
 * 	bool operator()() {
 * 		std::cerr << "Boo::operator()\n";
 * 		return true;
 *  }
 *  bool myFunc() {
 * 		std::cerr << "Boo::myFunc()\n";
 * 		return true;
 *  }
 * }
 * 
 * bool bar(int i = 0)
 * {
 * 		std::cerr << "bar(" << i << ")\n";
 * 		return true;
 * }
 * 
 * int main() {
 * 		Foo foo;
 * 		concurrent::Task mytask1(foo);
 * 		concurrent::Task mytask2(&bar);
 * 		concurrent::Task mytask3(boost::bind(&bar, 2));
 * 		concurrent::Task mytask4;
 * 		concurrent::Task mytask5(boost::bind(&Foo::myFunc, &foo));
 * 		mytask4.run(foo);
 * 		return 0;
 * }
 * 	
 * </code>
 * 
 * ... might produce
 * 
 * <code>
 * 		Foo::operator()
 * 		bar(0)
 * 		bar(2)
 * </code>
 */
class Task : private Thread
{
public:
	
	typedef boost::shared_ptr<Task> Ptr;
	
public:

	Task(const char * desc = "ml::common::concurrent::Task") :
		Thread(desc)
	{
	}

	template <typename Func>
	Task(const Func &func, const char * desc = "ml::common::concurrent::Task") :
		Thread(desc),
		m_func(func)
	{
		start();
	}

	template <typename Func>
	Task(const Func &func, const char * desc, const boost::function<void()> funcOnStart) :
		Thread(desc),
		m_func(func),
		m_funcOnStart(funcOnStart)
	{
		start();
	}


	~Task()
	{
		Thread::stop();
		Thread::join();
	}

	template <typename Func>
	void run(const Func &func)
	{
		if( state() != Thread::TS_READY )
			THROW(ml::common::exception::InvalidStateException, desc() + " : Task has already been run");
		
		m_func = func;
		start();
	}
	
	void stop()
	{
		Thread::stop();
	}

	void wait()
	{
		Thread::join();
	}

private:

	bool onProcess() 
	{
		return m_func();
	}

	virtual void onThreadStart() {
		if (m_funcOnStart)
			m_funcOnStart();
	}
		
	boost::function<bool ()> m_func;
	boost::function<void()> m_funcOnStart;
};

}; // concurrent
}; // common
}; // ml

#endif // __COMMON_CONCURRENT_TASK_HPP__
