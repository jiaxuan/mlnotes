#ifndef __COMMON_TEXT_STRINGBUILDER_HPP__
#define __COMMON_TEXT_STRINGBUILDER_HPP__

#include "common.hpp"
#include "DecimalDecNumber.hpp"
#include <cstring>
#include <memory>

#include <boost/shared_array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_array.hpp>

/**
 * Use this class to build up a string significantly more quickly than ostringstream. 
 */
class StringBuilder : private virtual boost::noncopyable
{
public:
	StringBuilder(uint32 initial_size = 1024);
	StringBuilder(const StringBuilder &sb);
	~StringBuilder();
	
	void append(const std::string & str);
	void append(const char *sz);
	void append(const char *data, uint32 length);
	void append(const char);
	void chopRight(uint32 length);
	std::string str() const;
	///@brief create string using 'right-most' chars only, with maximal specified length
	std::string strFromRight(uint32 length) const;
	const char * c_str() const;
	void clear();
	uint32 count() const;
	bool empty() const;
	
	StringBuilder & operator<<(const bool);
	StringBuilder & operator<<(const byte);
	StringBuilder & operator<<(const char);
	StringBuilder & operator<<(const int16);
	StringBuilder & operator<<(const uint16);
	StringBuilder & operator<<(const int32);
	StringBuilder & operator<<(const uint32);
	StringBuilder & operator<<(const int64);
	StringBuilder & operator<<(const uint64);
	// StringBuilder & operator<<(const mlc::math::DecimalMPFR &);
	StringBuilder & operator<<(const DecimalDecNumber &);
	StringBuilder & operator<<(const float);
	StringBuilder & operator<<(const double);
	StringBuilder & operator<<(const char *);
	StringBuilder & operator<<(const std::string &);
	
public:
	static void TestHarness();
	
private:
	void expand(const uint32 expand_by);

private:
	uint32 m_buffer_size;
	boost::scoped_array<char> m_buffer;
	uint32 m_insertion_pos;
};

#endif // __COMMON_TEXT_STRINGBUILDER_HPP__
