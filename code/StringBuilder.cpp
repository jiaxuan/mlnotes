#include "StringBuilder.hpp"
#include "DecimalDecNumber.hpp"
// #include <common/math/DecimalMPFR.hpp>
#include <iostream>
#include "Logger.hpp"

using namespace std;

namespace {
	char *createBuffer(uint32 size)
	{
		try
		{
			return new char[size];
		}
		catch (std::bad_alloc &)
		{
			LCRIT("std::bad_alloc constructing StringBuilder buffer of size %u", size);
			throw;
		}
	}
}

StringBuilder::StringBuilder(uint32 initial_size) :
	m_buffer_size(initial_size <= 0 ? 1 : initial_size),
	m_buffer(::createBuffer(m_buffer_size)),
	m_insertion_pos(0)
{
}

StringBuilder::StringBuilder(const StringBuilder &sb) :
	m_buffer_size(sb.m_insertion_pos+1),
	m_buffer(::createBuffer(m_buffer_size)),
	m_insertion_pos(0)
{
	if (!sb.empty())
		append(sb.m_buffer.get(), sb.m_insertion_pos);
}

StringBuilder::~StringBuilder()
{
}

void StringBuilder::append(const std::string & str)
{
	if( !str.empty() )
		append( str.data(), str.length() );
}

void StringBuilder::append(const char *sz)
{
	if( sz )
		append(sz, strlen(sz));
}

void StringBuilder::append(const char c)
{
	append(&c, 1);
}

void StringBuilder::append(const char *data, uint32 length)
{
	// Note we always leave at least one extra spot for the terminating null we need in the str() method...
	
	
	if (m_insertion_pos + length + 1 > m_buffer_size)
	{
		expand(length + 1);
	}
	
	memcpy(m_buffer.get()+m_insertion_pos, data, length);
	m_insertion_pos += length;
}

void StringBuilder::chopRight(uint32 length)
{
  if(m_insertion_pos > length)
      m_insertion_pos -= length;
}

StringBuilder & StringBuilder::operator<<(const bool value)
{
	append( value ? "true" : "false" );
	return *this;
}

StringBuilder & StringBuilder::operator<<(const byte value)
{
	append( static_cast<const char>(value) );
	return *this;
}

StringBuilder & StringBuilder::operator<<(const char value)
{
	append(value);
	return *this;
}

StringBuilder & StringBuilder::operator<<(const int16 value)
{
	char buf[512] = {0};
	sprintf(buf, "%hd", value);
	append(buf, strlen(buf));
	return *this;
}

StringBuilder & StringBuilder::operator<<(const uint16 value)
{
	char buf[512] = {0};
	sprintf(buf, "%hu", value);
	append(buf, strlen(buf));
	return *this;
}

StringBuilder & StringBuilder::operator<<(const int32 value)
{
	char buf[512] = {0};
	sprintf(buf, "%d", value);
	append(buf, strlen(buf));
	return *this;
}

StringBuilder & StringBuilder::operator<<(const uint32 value)
{
	char buf[512] = {0};
	sprintf(buf, "%u", value);
	append(buf, strlen(buf));
	return *this;
}

StringBuilder & StringBuilder::operator<<(const int64 value)
{
	char buf[512] = {0};
	sprintf(buf, "%ld", value);
	append(buf, strlen(buf));
	return *this;
}

StringBuilder & StringBuilder::operator<<(const uint64 value)
{
	char buf[512] = {0};
	sprintf(buf, "%lu", value);
	append(buf, strlen(buf));
	return *this;
}

// StringBuilder & StringBuilder::operator<<(const mlc::math::DecimalMPFR &value)
// {
// 	append(value.toString());
// 	return *this;
// }

StringBuilder & StringBuilder::operator<<(const DecimalDecNumber &value)
{
	append(value.toString());
	return *this;
}

StringBuilder & StringBuilder::operator<<(const float value)
{
	char buf[512] = {0};
	sprintf(buf, "%g", value);
	append(buf, strlen(buf));
	return *this;
}

StringBuilder & StringBuilder::operator<<(const double value)
{
	char buf[512] = {0};
	sprintf(buf, "%g", value);
	append(buf, strlen(buf));
	return *this;
}

StringBuilder & StringBuilder::operator<<(const char *value)
{
	append(value);
	return *this;
}

StringBuilder & StringBuilder::operator<<(const std::string &value)
{
	append(value);
	return *this;
}

std::string StringBuilder::str() const
{
	return std::string(m_buffer.get(), m_insertion_pos);
}

std::string StringBuilder::strFromRight(uint32 length) const
{
	if (m_insertion_pos > length)
	{
		const uint32 nDisp = m_insertion_pos - length;
		return std::string(m_buffer.get() + nDisp, length);
	}
	return str();
}

const char * StringBuilder::c_str() const
{
	m_buffer.get()[m_insertion_pos] = 0;
	return m_buffer.get();	
}

uint32 StringBuilder::count() const
{
	return m_insertion_pos;
}

bool StringBuilder::empty() const
{
	return m_insertion_pos == 0;
}

void StringBuilder::clear()
{
	m_insertion_pos = 0;	
}

void StringBuilder::expand(const uint32 expand_by)
{
	// Pick a reasonable amount by which to increase the buffer
	uint32 new_buffer_size = (expand_by > m_buffer_size) ? (m_buffer_size + expand_by) : (m_buffer_size * 2);
	
	// Copy the old data into the new buffer
	boost::scoped_array<char> new_buffer(new char[new_buffer_size]);
	memcpy(new_buffer.get(), m_buffer.get(), m_insertion_pos);
	
	// Remember the new buffer (and automatically deallocate the old buffer)
	m_buffer_size = new_buffer_size;
	m_buffer.swap(new_buffer);
}

// --------------------------------------------------------------------------------------------------------------------------------------------

void StringBuilder::TestHarness()
{
	char RANDOM_DATA[1024*256];
	
	cout << "---Some initial size tests, with adding random data" << endl;
	for (int i = 1; i <= 32768; i*=2)
	{
		cout << "Testing " << i << endl;
		
		StringBuilder sb(i);
		for (int j = 0; j < 1024; ++j)
		{
			sb.append(RANDOM_DATA, rand() % 4096);
		}
	}

	cout << "---Default size, 1024 same-size additions" << endl;
	for (int i = 1; i < 100000; i = (2*i + rand()%2))
	{
		cout << "Testing " << i << endl;
		
		StringBuilder sb;
		for (int j = 0; j < 1024; ++j)
		{
			sb.append(RANDOM_DATA, i);
		}
	}
}


