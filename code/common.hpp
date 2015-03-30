#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#define PACKAGE "InstinctFX"
#define PACKAGE_VERSION "1"

#include <sys/types.h>
#include <boost/current_function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/type_traits.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/ref.hpp>
#include <cerrno>


/** ----------------------------------------------------------------------------------
 * Types we can use throughout the system to guard against unforseen errors if we
 * move to 64bit architecture
 */
typedef  __uint8_t		byte;
typedef  __uint8_t		uint8;

typedef  __int16_t		int16;
typedef  __uint16_t		uint16;

typedef  __int32_t		int32;
typedef  __uint32_t		uint32;

typedef  __int64_t		int64;
typedef  __uint64_t		uint64;

/** ----------------------------------------------------------------------------------
 * Macro used predominantly by other macros to concat two tokens at pre-process time
 *
 * Sample usage:
 * <code>
 * 		int CONCAT(myint, 1) = 0;
 * </code>
 *
 * ... might expand to:
 *
 * <code>
 * 		int myint1 = 0;
 * </code>
 *
 * NOTE: Multiple levels of pre-processor indirection allow support for macros as marco arguments
 */

#define __CONCAT_1(a,b)	a##b
#define __CONCAT_2(a,b)	__CONCAT_1(a,b)
#define CONCAT(a,b) 	__CONCAT_2(a,b)

/** ----------------------------------------------------------------------------------
 * Macro used predominantly by other macros to ensure a variable name
 * they introduce is unique. It cats the line number to the name provided
 * in the macro argument.
 *
 * Sample usage:
 * <code>
 * 	 	int UNIQUE_NAME(myint) = 0;
 * 		int UNIQUE_NAME(myint) = 0;
 * </code>
 *
 * 	... might expand to:
 *
 * <code>
 * 		int _myint10 = 0;
 * 		int _myint11 = 0;
 * </code>
 */

#define __UNIQUE_NAME_1(a,b)	CONCAT(_, CONCAT(a,b))
#define __UNIQUE_NAME_2(a,b)	__UNIQUE_NAME_1(a,b)
#define UNIQUE_NAME(a)			__UNIQUE_NAME_2(a,__LINE__)

/** ----------------------------------------------------------------------------------
 * Macro used predominantly by other macros to turn an arbitrary name into its
 * string representation:
 *
 * Sample usage:
 * <code>
 * 		const char * mystring = TO_STRING(mystring);
 * </code>
 *
 * ... might expand to
 *
 * <code>
 * 		const char * mystring = "mystring";
 * </code>
 */

#define __TO_STRING_1(x) 	#x
#define __TO_STRING_2(x) 	__TO_STRING_1(x)
#define TO_STRING(x) 		__TO_STRING_2(x)

/** ----------------------------------------------------------------------------------
 * Macro to let us know if we should assume RTTI is enabled
 *
 * This is only a best effort as not all compilers expose an rtti flag. For some, it may be
 * necessary to -D__RTTI__ on the command line to turn on rtti-enabled helper routines
 */

#if !defined(__RTTI__)
#if defined(__RTTI) || defined(_CPPRTTI) || defined(__INTEL_RTTI__)
	#define __RTTI__
#endif
#endif // __RTTI__

/** ----------------------------------------------------------------------------------
 * Macro used predominantly by other macros to obtain the fully namespace qualified name of a
 * given TypeName argument.
 *
 * Sample usage:
 * <code>
 * 		using ml::common::Exception;
 * 		std::string strName = TYPE_TO_STRING(Exception);
 * 		std::cout << "\"" << strName << "\"" << std::endl;
 * </code>
 *
 * ... might produce
 *
 * <code>
 * 		"ml::common::Exception"
 * </code>
 *
 * @warning <code>TYPE_TO_STRING(...)</code> returns a temporary value and must not be
 * cached directly or used across sequence points without taking a copy
 */

#if defined(__RTTI__)

#include <typeinfo>

#if defined(__ABI__) && defined(__GNUC__)

	#include <common/diagnostics/Demangle.hpp>

	#define TYPE_TO_STRING( T ) 	ml::common::diagnostics::Demangle::convert( typeid(T).name() ).c_str()

#else

	/** This may be a mangled name on some compilers. */
	#define TYPE_TO_STRING( T ) 	typeid( T ).name()

#endif // __ABI__

#else

	#define TYPE_TO_STRING( T ) 	TO_STRING( T )

#endif // __RTTI__

/** ----------------------------------------------------------------------------------
 * Helper macro that return the name, and potentially signature of the current function.
 *
 * Sample usage:
 * <code>
 * 		int main() {
 * 			std::cout << __FUNCTION_SIGNATURE__ << std::endl;
 * 		}
 * </code>
 *
 * ... might output
 *
 * <code>
 *  	int main()
 * </code>
 */

#define __FUNCTION_SIGNATURE__		BOOST_CURRENT_FUNCTION

/** ----------------------------------------------------------------------------------
 * Helper macro returning a string representing the current file & line location
 *
 * Sample usage:
 * <code>
 * 		const char * mystring = __LOCATION__;
 * </code>
 *
 * ... might expand to
 *
 * <code>
 * 		const char * mystring = "Main.cpp (line 3)";
 * </code>
 */

#define __LOCATION__		__FILE__ " (line " TO_STRING(__LINE__) ")"

/** ----------------------------------------------------------------------------------
 * Helper macro returning a string representing the last compilation date & time of
 * the module containing the current file
 *
 * Sample usage:
 * <code>
 * 		const char * compiled_at = __DATE_TIME__;
 * </code>
 *
 * ... might expand to
 *
 * <code>
 * 		const char * compiled_at = "Oct 30 2005 17:11:00";
 * </code>
 */

//#ifndef __DATE_TIME__
//#define __DATE_TIME__		__DATE__ " " __TIME__
//#endif

/** ----------------------------------------------------------------------------------
 * Nice little foreach macro
 *
 * Sample usage:
 * <code>
 * 		std::vector<int> v;
 * 		v.push_back(10);
 * 		v.push_back(20);
 *
 * 		foreach(it, v) {
 * 			std::cout << *it << std::endl;
 * 		}
 * </code>
 *
 * ... might output
 *
 * <code>
 * 		10
 * 		20
 * </code>
 */

#if defined(__GNUC__) || defined(__CDT_PARSER__)
	#define foreach(it, cont)	\
		for( const BOOST_TYPEOF(cont) & UNIQUE_NAME(_c) = cont, *UNIQUE_NAME(_pt) = &UNIQUE_NAME(_c); UNIQUE_NAME(_pt) != NULL; UNIQUE_NAME(_pt) = NULL )	\
			for ( BOOST_TYPEOF(UNIQUE_NAME(_c).begin()) it=UNIQUE_NAME(_c).begin(), UNIQUE_NAME(_itEnd)=UNIQUE_NAME(_c).end(); it != UNIQUE_NAME(_itEnd); ++it )

	#define foreach_m(it, cont)	\
		for( BOOST_TYPEOF(cont) & UNIQUE_NAME(_c) = cont, *UNIQUE_NAME(_pt) = &UNIQUE_NAME(_c); UNIQUE_NAME(_pt) != NULL; UNIQUE_NAME(_pt) = NULL )	\
			for ( BOOST_TYPEOF(UNIQUE_NAME(_c).begin()) it=UNIQUE_NAME(_c).begin(), UNIQUE_NAME(_itEnd)=UNIQUE_NAME(_c).end(); it != UNIQUE_NAME(_itEnd); ++it )

	#define foreach_ref(ref, cont)	\
		for( const BOOST_TYPEOF(cont) & UNIQUE_NAME(_c) = cont, *UNIQUE_NAME(_pt) = &UNIQUE_NAME(_c); UNIQUE_NAME(_pt) != NULL; UNIQUE_NAME(_pt) = NULL )	\
			for ( BOOST_TYPEOF(UNIQUE_NAME(_c).begin()) UNIQUE_NAME(_it)=UNIQUE_NAME(_c).begin(), UNIQUE_NAME(_itEnd)=UNIQUE_NAME(_c).end(); UNIQUE_NAME(_it) != UNIQUE_NAME(_itEnd); ++UNIQUE_NAME(_it) ) \
                for ( const BOOST_TYPEOF(*UNIQUE_NAME(_it)) & ref = *UNIQUE_NAME(_it), *UNIQUE_NAME(_pt2) = &(ref); UNIQUE_NAME(_pt2) != NULL; UNIQUE_NAME(_pt2) = NULL )

	#define foreach_mref(mref, cont)	\
		for( BOOST_TYPEOF(cont) & UNIQUE_NAME(_c) = cont, *UNIQUE_NAME(_pt) = &UNIQUE_NAME(_c); UNIQUE_NAME(_pt) != NULL; UNIQUE_NAME(_pt) = NULL )	\
			for ( BOOST_TYPEOF(UNIQUE_NAME(_c).begin()) UNIQUE_NAME(_it)=UNIQUE_NAME(_c).begin(), UNIQUE_NAME(_itEnd)=UNIQUE_NAME(_c).end(); UNIQUE_NAME(_it) != UNIQUE_NAME(_itEnd); ++UNIQUE_NAME(_it) ) \
                for ( BOOST_TYPEOF(*UNIQUE_NAME(_it)) & mref = *UNIQUE_NAME(_it), *UNIQUE_NAME(_pt2) = &(mref); UNIQUE_NAME(_pt2) != NULL; UNIQUE_NAME(_pt2) = NULL )
#else
	#error "TODO: Implement foreach() for this compiler"
#endif // __GNUC__

#endif // __COMMON_HPP__
