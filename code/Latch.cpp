#include <common/concurrent/Latch.hpp>

#include <common/logging/Logger.hpp>

using namespace ml::common::concurrent;

Latch::Latch()
	: m_mutex()
	, m_cond()
	, m_unlatched(eCLOSE_LATCH)
{
}

Latch::~Latch()
{
	LTRCE("Tearing down latch %p", this);
	reset(); // This is going to segfault if anyone is still waiting...
}

bool Latch::attempt(int32 timeout)
{
	LTRCE("Attempting latch %p with timeout %d", this, timeout);
	
	bool success = false;
	if (m_unlatched == eOPEN_LATCH)
    {
        success = true;
    }
	else if (timeout == NO_WAIT)
    {
        success = false;
    }
	else synchronized (m_mutex)
	{
		if (timeout < 0) m_cond.wait(m_mutex);          // WAIT_FOREVER
		else             m_cond.wait(m_mutex, timeout); // Timeout must be positive here
		
        if(m_unlatched == eOPEN_LATCH)
            success = true;

		//success = m_unlatched;
	}
	LTRCE("%successfully awaited latch %p", success ? "S" : "Uns", this);
	return success;
}

void Latch::open()
{
	LTRCE("Opening latch %p", this);
	m_unlatched = eOPEN_LATCH;
	synchronized( m_mutex )
	{
		m_cond.notifyAll();
	}
}

void Latch::close()
{
	LTRCE("Closing latch %p", this);
	m_unlatched = eCLOSE_LATCH;
}

void Latch::reset()
{
	LTRCE("Resetting latch %p", this);
	m_unlatched = eCLOSE_LATCH;
	synchronized( m_mutex )
	{
		m_cond.notifyAll();
	}
}
