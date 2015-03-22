#ifndef __COMMON_MATH_DECIMALDECNUMBER_HPP__
#define __COMMON_MATH_DECIMALDECNUMBER_HPP__

#if !defined(CORRECT_PLACE_TO_INCLUDE_DECIMAL_DECNUMBER)
#error "Please include common/math/Decimal.hpp instead to allow change of implementation in future"
#endif 
#undef CORRECT_PLACE_TO_INCLUDE_DECIMAL_DECNUMBER

#include <boost/operators.hpp>
#include <common/common.hpp>
#include "decNumber.h"

namespace ml {
namespace common {
namespace math {

class DecimalMPFR;
class DecimalDecNumber
	: boost::field_operators<DecimalDecNumber> 
	, boost::unit_steppable<DecimalDecNumber>
{
public:
   DecimalDecNumber();
	DecimalDecNumber(const DecimalDecNumber &other);
   DecimalDecNumber(const DecimalMPFR &other);
   DecimalDecNumber(int value);
	explicit DecimalDecNumber(double value);
   explicit DecimalDecNumber(int64 value);
	explicit DecimalDecNumber(uint64 value);
	explicit DecimalDecNumber(const char *value);
	explicit DecimalDecNumber(const std::string &value);
	~DecimalDecNumber();

    DecimalDecNumber &operator=(double rhs);
    DecimalDecNumber &operator=(int rhs);
	DecimalDecNumber &operator=(const DecimalDecNumber &other);
	DecimalDecNumber &operator=(const DecimalMPFR &other);

	const DecimalDecNumber &operator +=(const DecimalDecNumber &other);
	const DecimalDecNumber &operator -=(const DecimalDecNumber &other);
	const DecimalDecNumber &operator *=(const DecimalDecNumber &other);
	const DecimalDecNumber &operator /=(const DecimalDecNumber &other);

	const DecimalDecNumber &operator ++(); // pre-increment
	const DecimalDecNumber &operator --(); // pre-decrement

	DecimalDecNumber operator +() const; // unary plus
	DecimalDecNumber operator -() const; // unary minus

	bool operator < (const DecimalDecNumber &) const;
	bool operator <=(const DecimalDecNumber &) const;
	bool operator ==(const DecimalDecNumber &) const;
	bool operator !=(const DecimalDecNumber &) const;
	bool operator >=(const DecimalDecNumber &) const;
	bool operator > (const DecimalDecNumber &) const;

	bool equalsDelta(const DecimalDecNumber &other, const DecimalDecNumber &delta) const;
	DecimalDecNumber distance(const DecimalDecNumber &other) const;

	int  sign() const;
	bool isNegative() const;
	bool isPositive() const;
	int  isInfinity() const;
	bool isZero() const;
	bool isValid() const;

	enum RoundingMode
	{
		ROUND_HALF_TO_POSITIVE_INFINITY, // Always round half up                          floor(this + 0.5)
		ROUND_HALF_TO_NEGATIVE_INFINITY, // Always round half down                        floor(this + 0.49999...)
		ROUND_TO_POSITIVE_INFINITY,      // Always round up                               ceil(this)
		ROUND_TO_NEGATIVE_INFINITY,      // Always round down                             floor(this)
		ROUND_AWAY_FROM_ZERO,            // Always round to maximise the absolute value   ceil(abs(this)) * sign(this)
		ROUND_TO_ZERO,                   // Always round to minimise the absolute value   floor(abs(this)) * sign(this)
		ROUND_HALF_AWAY_FROM_ZERO,       // Round half to maximise the absolute value     floor(abs(this) + 0.5) * sign(this)
		ROUND_HALF_TO_ZERO,              // Round half to minimise the absolute value     floor(abs(this) + 0.49999...) * sign(this)
		ROUND_HALF_TO_EVEN               // Statistical rounding                          round(this)
	};

    // Mirrors spec::CurrencyPair::spotPrecisionDecimalIncr
    enum class RoundIncrement
    {
        TENTH,
        HALF,
        QUARTER
    };

    struct RoundParameters
    {
       RoundParameters(uint32 decimalPlaces);
       RoundParameters(uint32 decimalPlaces, RoundingMode roundMode);
       RoundParameters(uint32 decimalPlaces, RoundingMode roundMode, RoundIncrement roundIncrement);
       
       const uint32 decimalPlaces;
       const RoundingMode roundMode;
       const RoundIncrement roundIncrement;
    };

	const DecimalDecNumber round(const RoundParameters& roundParameters);
	const DecimalDecNumber round(uint32 decimalPlaces, RoundingMode mode = RoundingMode::ROUND_HALF_AWAY_FROM_ZERO, RoundIncrement roundIncrement = RoundIncrement::TENTH);

	std::string toString(uint32 decimalPlaces = 10) const;
    std::string toStringAllowScientific(uint32 decimalPlaces = 10) const;

	double toDoubleWithPossiblePrecisionLoss() const;

	///@return integer representation of DecimalDecNumber (kind of '(int)value', means 'floor()' will be used to get integer)
	///@throw an exception if value is not a valid number or can't be represented by int
	int32 toInt32() const;
	int64 toInt64() const;

	friend DecimalDecNumber avg(const DecimalDecNumber &lhs, const DecimalDecNumber &rhs);
	friend DecimalDecNumber sqrt(const DecimalDecNumber &val);
	friend DecimalDecNumber power(const DecimalDecNumber &val, const DecimalDecNumber &exponent);
	friend DecimalDecNumber power(const DecimalDecNumber &val, int32 exponent);
	friend DecimalDecNumber exp(const DecimalDecNumber &val);

	static DecimalDecNumber infinity(bool positive);
	static DecimalDecNumber nan();

	static DecimalDecNumber fromDecimalComponents(const int64 significand, const int32 exp);
	static bool toDecimalComponents(const DecimalDecNumber &val, int64& significand, int32& exp);

private:
	int comparedTo(const DecimalDecNumber &other) const;
	void setValue(const char *);

public:
   static const DecimalDecNumber MINUS_ONE;
   static const DecimalDecNumber ZERO;
   static const DecimalDecNumber ONE;
   static const DecimalDecNumber TWO;
   static const DecimalDecNumber FOUR;
   static const DecimalDecNumber TEN;
   static const DecimalDecNumber HUNDRED;
   static const DecimalDecNumber THOUSAND;
   static const DecimalDecNumber TEN_THOUSAND;
   static const DecimalDecNumber HUNDRED_THOUSAND;
   static const DecimalDecNumber MILLION;
   static const DecimalDecNumber POSITIVE_INFINITY;
   static const DecimalDecNumber NEGATIVE_INFINITY;

private:
   mutable decContext m_context;
   decNumber m_value;

   template<typename T> const T& toIntX(T& result) const;
};

void unpackScientificFormat(char * b, char *& e, int bufferSize, int decimalPlaces);

// PAN-17342: Resolve gcc3.4 'friend injection' issue
DecimalDecNumber avg(const DecimalDecNumber &lhs, const DecimalDecNumber &rhs);
DecimalDecNumber sqrt(const DecimalDecNumber &val);
DecimalDecNumber power(const DecimalDecNumber &val, const DecimalDecNumber &exponent);
DecimalDecNumber power(const DecimalDecNumber &val, int32 exponent);
DecimalDecNumber exp(const DecimalDecNumber &val);


std::ostream & operator<<(std::ostream &os, const ml::common::math::DecimalDecNumber &d);


} // math
} // common
} // ml

typedef mlc::math::DecimalDecNumber::RoundParameters RoundParameters;
const char * roundingModeToString(mlc::math::DecimalDecNumber::RoundingMode mode);

inline ml::common::math::DecimalDecNumber abs(const ml::common::math::DecimalDecNumber &value)
{
	return (value.isNegative() ? -value : value);
}

#endif // __COMMON_MATH_DECIMALDECNUMBER_HPP__

