#include "InterruptibleSleep.hpp"

bool InterruptibleSleep::sleep(int milliseconds)
{
	bool result;
	
	synchronized(m_mutex)
	{
		result = m_condition.wait(m_mutex, milliseconds);
	}
	
	return result;
}

bool InterruptibleSleep::wait(int milliseconds)
{
	return sleep(milliseconds);
}

void InterruptibleSleep::wakeOne()
{
	synchronized(m_mutex)
	{
		m_condition.notify();
	}
}

void InterruptibleSleep::wakeAll()
{
	synchronized(m_mutex)
	{
		m_condition.notifyAll();
	}
}

	
