#define CORRECT_PLACE_TO_INCLUDE_DECIMAL_DECNUMBER
#include "DecimalDecNumber.hpp"
#include "Logger.hpp"
#include "Format.hpp"
// #include <common/pattern/AutoInvoke.hpp>
#include <algorithm>
#include <string>
#include <stdlib.h>
#include <boost/lexical_cast.hpp>
#include "decContext.h"

namespace
{
	const char * DEC_NUMBER = "decNumber";

   const unsigned int precisionDigitsWhenConvertingFromOldDecimal = 30;

   // Users of this class should not compare against these values directly. Instead they should use isInfinity(), isNan(), etc.
   const DecimalDecNumber NOT_A_NUMBER(DecimalDecNumber::nan());

   const DecimalDecNumber comparisonThreshold(0.0000000000000000000000000000000000001);
   const DecimalDecNumber comparisonThresholdNegative(-0.0000000000000000000000000000000000001);

   void int64_to_string(int64 q, char * buffer)
   {
      const bool negative = q < 0;
      if (negative)
         q = -q;

      char * p = buffer;
      do
      {    
         *p++ = "0123456789"[ q % 10 ]; 
         q /= 10; 
      } while ( q );

      if (negative)
         *p++ = '-';
      *p = '\0';

      std::reverse(buffer, p);
   } 

   void uint64_to_string(uint64 q, char * buffer)
   {
      char * p = buffer;
      do
      {
         *p++ = "0123456789"[ q % 10 ];
         q /= 10;
      } while ( q );
      *p = '\0';

      std::reverse(buffer, p);
   } 
}

const DecimalDecNumber DecimalDecNumber::MINUS_ONE(-1);
const DecimalDecNumber DecimalDecNumber::ZERO(0);
const DecimalDecNumber DecimalDecNumber::ONE(1);
const DecimalDecNumber DecimalDecNumber::TWO(2);
const DecimalDecNumber DecimalDecNumber::FOUR(4);
const DecimalDecNumber DecimalDecNumber::TEN(10);
const DecimalDecNumber DecimalDecNumber::HUNDRED(100);
const DecimalDecNumber DecimalDecNumber::THOUSAND(1000);
const DecimalDecNumber DecimalDecNumber::TEN_THOUSAND(10000);
const DecimalDecNumber DecimalDecNumber::HUNDRED_THOUSAND(100000);
const DecimalDecNumber DecimalDecNumber::MILLION(1000000);
const DecimalDecNumber DecimalDecNumber::POSITIVE_INFINITY(DecimalDecNumber::infinity(true));
const DecimalDecNumber DecimalDecNumber::NEGATIVE_INFINITY(DecimalDecNumber::infinity(false));

DecimalDecNumber::DecimalDecNumber()
{
   decContextDefault(&m_context, DEC_INIT_BASE); // initialize
   m_context.traps = 0;                     // no traps, thank you
   m_context.digits = DECNUMDIGITS;         // set precision
   decNumberZero(&m_value);
}

DecimalDecNumber::DecimalDecNumber(int rhs)
{
   decContextDefault(&m_context, DEC_INIT_BASE); // initialize
   m_context.traps = 0;                     // no traps, thank you
   m_context.digits = DECNUMDIGITS;         // set precision
   decNumberFromInt32(&m_value, rhs);
}

DecimalDecNumber::DecimalDecNumber(const DecimalDecNumber &rhs)
 : m_value(rhs.m_value)
{
   decContextDefault(&m_context, DEC_INIT_BASE); // initialize
   m_context.traps = 0;                     // no traps, thank you
   m_context.digits = DECNUMDIGITS;         // set precision
}

// DecimalDecNumber::DecimalDecNumber(const DecimalMPFR &rhs)
// {
//    decContextDefault(&m_context, DEC_INIT_BASE); // initialize
//    m_context.traps = 0;                     // no traps, thank you
//    m_context.digits = DECNUMDIGITS;         // set precision
//    setValue(rhs.toString(precisionDigitsWhenConvertingFromOldDecimal).c_str());
// }

DecimalDecNumber::DecimalDecNumber(double rhs)
{
   decContextDefault(&m_context, DEC_INIT_BASE); // initialize
   m_context.traps = 0;                     // no traps, thank you
   m_context.digits = DECNUMDIGITS;         // set precision

   char buffer[512] = {0}; // Stack allocated string
   snprintf(buffer, 511, "%0.15lg", rhs);
   decNumberFromString(&m_value, buffer, &m_context);
}

DecimalDecNumber::DecimalDecNumber(int64 rhs)
{
   decContextDefault(&m_context, DEC_INIT_BASE); // initialize
   m_context.traps = 0;                     // no traps, thank you
   m_context.digits = DECNUMDIGITS;         // set precision

   if (   (rhs >= 0 && rhs <= std::numeric_limits<int>::max())
       || (rhs < 0  && rhs >= std::numeric_limits<int>::min()))
   {
      decNumberFromInt32(&m_value, static_cast<int>(rhs));
   }
   else
   {
      char buffer[64];
      int64_to_string(rhs, buffer);
      decNumberFromString(&m_value, buffer, &m_context);
   }
}

DecimalDecNumber::DecimalDecNumber(uint64 rhs)
{
   decContextDefault(&m_context, DEC_INIT_BASE); // initialize
   m_context.traps = 0;                     // no traps, thank you
   m_context.digits = DECNUMDIGITS;         // set precision

   if (rhs <= std::numeric_limits<uint>::max())
   {
      decNumberFromUInt32(&m_value, static_cast<uint>(rhs));
   }
   else
   {
      char buffer[64];
      uint64_to_string(rhs, buffer);
      decNumberFromString(&m_value, buffer, &m_context);
   }
}

DecimalDecNumber::DecimalDecNumber(const char *str)
{
   decContextDefault(&m_context, DEC_INIT_BASE); // initialize
   m_context.traps = 0;                     // no traps, thank you
   m_context.digits = DECNUMDIGITS;         // set precision
	setValue(str);
}

DecimalDecNumber::DecimalDecNumber(const std::string &str)
{
   decContextDefault(&m_context, DEC_INIT_BASE); // initialize
   m_context.traps = 0;                     // no traps, thank you
   m_context.digits = DECNUMDIGITS;         // set precision
	setValue(str.c_str());
}

DecimalDecNumber::~DecimalDecNumber()
{
}


/// Convert the string presented into a form acceptable to the dec number class
/// @throws exception when the format causes a problem in the number conversion.
///
void DecimalDecNumber::setValue(const char *rhs)
{
   decNumberFromString(&m_value, rhs, &m_context);

   const unsigned int state = decContextGetStatus(&m_context);
   if (state)
   {
      // See decContext.h for definition of DEC_Errors and DEC_Information
      if (state & DEC_Errors)
      {
         // GLCRIT(DEC_NUMBER, "decNumberFromString conversion from [%s] generated error status [%d:%s]",
         //        rhs, state, decContextStatusToString(&m_context));
         // FTHROW(InvalidArgumentException, "Attempt to create decimal from invalid string: [%s]", rhs);
         throw("Attempt to create decimal from invalid string");
      }

      // GLDBUG(DEC_NUMBER, "decNumberFromString conversion from [%s] generated info status [%d:%s]",
      //        rhs, state, decContextStatusToString(&m_context));
      decContextClearStatus(&m_context, 0U);
   }
}

DecimalDecNumber DecimalDecNumber::infinity(bool positive)
{
   DecimalDecNumber retval;
   retval.m_value.digits = 1;
   retval.m_value.exponent = 0;
   // TODO: set lsu to 0 using memset?
   retval.m_value.lsu[0] = 0;
   if (positive)
      retval.m_value.bits = DECINF;
   else
      retval.m_value.bits = DECINF | DECNEG;
	return retval;
}

DecimalDecNumber DecimalDecNumber::nan()
{
   DecimalDecNumber retval;
   retval.m_value.bits = DECNAN;
   return retval;
}

DecimalDecNumber &DecimalDecNumber::operator=(double rhs)
{
    char buffer[512] = {0}; // Stack allocated string
    snprintf(buffer, 511, "%0.15lg", rhs);
    decNumberFromString(&m_value, buffer, &m_context);
    return *this;
}

DecimalDecNumber &DecimalDecNumber::operator=(int rhs)
{
   decNumberFromInt32(&m_value, rhs);
   return *this;
}

DecimalDecNumber &DecimalDecNumber::operator=(const DecimalDecNumber & rhs)
{
	if (this != &rhs)
	{
      m_value = rhs.m_value;
	}
	return *this;
}

// DecimalDecNumber &DecimalDecNumber::operator=(const DecimalMPFR & rhs)
// {
//    setValue(rhs.toString(precisionDigitsWhenConvertingFromOldDecimal).c_str());
// 	return *this;
// }

const DecimalDecNumber &DecimalDecNumber::operator +=(const DecimalDecNumber &rhs)
{
	if (decNumberIsNaN(&m_value) || decNumberIsNaN(&rhs.m_value))
	{
		// FTHROW(InvalidStateException, "Performing arithmetic on uninitialised decimal [Nan]");
		throw("Performing arithmetic on uninitialised decimal [Nan]");
	}

	decNumberAdd(&m_value, &m_value, &rhs.m_value, &m_context);
	return *this;
}
const DecimalDecNumber &DecimalDecNumber::operator -=(const DecimalDecNumber &rhs)
{
	if (decNumberIsNaN(&m_value) || decNumberIsNaN(&rhs.m_value))
	{
		// FTHROW(InvalidStateException, "Performing arithmetic on uninitialised decimal [Nan]");
		throw("Performing arithmetic on uninitialised decimal [Nan]");
	}

   decNumberSubtract(&m_value, &m_value, &rhs.m_value, &m_context);
	return *this;
}
const DecimalDecNumber &DecimalDecNumber::operator *=(const DecimalDecNumber &rhs)
{
	if (decNumberIsNaN(&m_value) || decNumberIsNaN(&rhs.m_value))
	{
		// FTHROW(InvalidStateException, "Performing arithmetic on uninitialised decimal [Nan]");
		throw("Performing arithmetic on uninitialised decimal [Nan]");
	}

   decNumberMultiply(&m_value, &m_value, &rhs.m_value, &m_context);
	return *this;
}
const DecimalDecNumber &DecimalDecNumber::operator /=(const DecimalDecNumber &rhs)
{
	if (decNumberIsNaN(&m_value) || decNumberIsNaN(&rhs.m_value))
	{
		// FTHROW(InvalidStateException, "Performing arithmetic on uninitialised decimal [Nan]");
		throw("Performing arithmetic on uninitialised decimal [Nan]");
	}

   if (decNumberIsZero(&rhs.m_value))
	{
		// FTHROW(LogicError, "Division by zero");
		throw("Division by zero");

	}
	
	if (decNumberIsInfinite(&m_value) || decNumberIsInfinite(&rhs.m_value))
	{
		throw("Cannot divide infinity by infinity");
	}

   decNumberDivide(&m_value, &m_value, &rhs.m_value, &m_context);
	return *this;
}

const DecimalDecNumber &DecimalDecNumber::operator ++()
{
	if (decNumberIsNaN(&m_value))
	{
		throw("Performing arithmetic on uninitialised decimal [Nan]");
	}

   decNumberAdd(&m_value, &m_value, &ONE.m_value, &m_context);
	return *this;
}

const DecimalDecNumber &DecimalDecNumber::operator --()
{
	if (decNumberIsNaN(&m_value))
	{
		throw("Performing arithmetic on uninitialised decimal [Nan]");
	}

   decNumberSubtract(&m_value, &m_value, &ONE.m_value, &m_context);
	return *this;
}

DecimalDecNumber DecimalDecNumber::operator +() const
{
	if (decNumberIsNaN(&m_value))
	{
		throw("Performing arithmetic on uninitialised decimal [Nan]");
	}

	return DecimalDecNumber(*this);
}

DecimalDecNumber DecimalDecNumber::operator -() const
{
	if (decNumberIsNaN(&m_value))
	{
		throw("Performing arithmetic on uninitialised decimal [Nan]");
	}

	DecimalDecNumber temp(*this);
   decNumberCopyNegate(&temp.m_value, &m_value);
	return temp;
}

int DecimalDecNumber::comparedTo(const DecimalDecNumber &rhs) const
{
	if (decNumberIsNaN(&m_value) || decNumberIsNaN(&rhs.m_value))
	{
		throw("Performing comparison on uninitialised decimal [Nan]");
	}

   // TODO: the commented out lines below do a strict comparison between two numbers.
   // However, the unit tests assert that comparison should be done within a tolerance
   //decNumberCompare(&result, &m_value, &rhs.m_value, &contextComparison);
   //return decNumberToInt32(&result, &m_context);

   decContext contextComparison;
   decContextDefault(&contextComparison, DEC_INIT_BASE); // initialize
   contextComparison.traps=0;                            // no traps, thank you
   contextComparison.digits=DECNUMCOMPARISONDIGITS;      // set precision

   decNumber tmp(m_value);
   decNumberSubtract(&tmp, &m_value, &rhs.m_value, &m_context);

   decNumber result;
   decNumberCompare(&result, &tmp, &comparisonThreshold.m_value, &contextComparison);
   if (decNumberToInt32(&result, &m_context) >= 1)
   {
      return 1;
   }

   decNumberCompare(&result, &tmp, &comparisonThresholdNegative.m_value, &contextComparison);
   if (decNumberToInt32(&result, &m_context) <= -1)
   {
      return -1;
   }
   return 0;
}

bool DecimalDecNumber::operator < (const DecimalDecNumber &rhs) const
{
   return comparedTo(rhs) < 0;
}
bool DecimalDecNumber::operator <=(const DecimalDecNumber &rhs) const
{
   return comparedTo(rhs) <= 0;
}
bool DecimalDecNumber::operator ==(const DecimalDecNumber &rhs) const
{
   return comparedTo(rhs) == 0;
}
bool DecimalDecNumber::operator !=(const DecimalDecNumber &rhs) const
{
   return comparedTo(rhs) != 0;
}
bool DecimalDecNumber::operator >=(const DecimalDecNumber &rhs) const
{
   return comparedTo(rhs) >= 0;
}
bool DecimalDecNumber::operator > (const DecimalDecNumber &rhs) const
{
   return comparedTo(rhs) > 0;
}

bool DecimalDecNumber::equalsDelta(const DecimalDecNumber &rhs, const DecimalDecNumber &delta) const
{
	DecimalDecNumber dist = distance(rhs);
	return dist <= delta;
}

DecimalDecNumber DecimalDecNumber::distance(const DecimalDecNumber &rhs) const
{
   decContext contextComparison;
   decContextDefault(&contextComparison, DEC_INIT_BASE); // initialize
   contextComparison.traps=0;                            // no traps, thank you
   contextComparison.digits=DECNUMCOMPARISONDIGITS;      // set precision

	DecimalDecNumber distance;
   decNumberSubtract(&distance.m_value, &m_value, &rhs.m_value, &m_context);
   return abs(distance);
}

int DecimalDecNumber::sign() const
{
   if (isNegative())
      return -1;
   else if (isZero())
      return 0;
   return 1;
}

bool DecimalDecNumber::isNegative() const
{
   return decNumberIsNegative(&m_value);
}

bool DecimalDecNumber::isPositive() const
{
	return sign() > 0;
}

int DecimalDecNumber::isInfinity() const
{
   if (decNumberIsInfinite(&m_value))
      return isNegative() ? -1 : 1;
   return 0;
}

bool DecimalDecNumber::isValid() const
{
   return !decNumberIsNaN(&m_value) && !decNumberIsInfinite(&m_value);
}

bool DecimalDecNumber::isZero() const
{
   return decNumberIsZero(&m_value);
}

// enum RoundingMode
// {
//    ROUND_HALF_TO_POSITIVE_INFINITY, // Always round half up                          floor(this + 0.5)
//    ROUND_HALF_TO_NEGATIVE_INFINITY, // Always round half down                        floor(this + 0.49999...)
//    ROUND_TO_POSITIVE_INFINITY,      // Always round up                               ceil(this)
//    ROUND_TO_NEGATIVE_INFINITY,      // Always round down                             floor(this)
//    ROUND_AWAY_FROM_ZERO,            // Always round to maximise the absolute value   ceil(abs(this)) * sign(this)
//    ROUND_TO_ZERO,                   // Always round to minimise the absolute value   floor(abs(this)) * sign(this)
//    ROUND_HALF_AWAY_FROM_ZERO,       // Round half to maximise the absolute value     floor(abs(this) + 0.5) * sign(this)
//    ROUND_HALF_TO_ZERO,              // Round half to minimise the absolute value     floor(abs(this) + 0.49999...) * sign(this)
//    ROUND_HALF_TO_EVEN               // Statistical rounding                          round(this)
// };
//
// enum RoundIncrement
// {
//    TENTH
//    HALF     - the digit *after* the last decimalPlace should be either 0 or 5
//    QUARTER  - the digits *after* the last decimalPlace should be 0, 25, 50, or 75
// };

const DecimalDecNumber DecimalDecNumber::round(uint32 decimalPlaces, RoundingMode mode, RoundIncrement roundIncrement)
{
   if (isZero())
      return *this;

   if (roundIncrement == RoundIncrement::HALF)
      decNumberMultiply(&m_value, &m_value, &DecimalDecNumber::TWO.m_value, &m_context);
   else if (roundIncrement == RoundIncrement::QUARTER)
      decNumberMultiply(&m_value, &m_value, &DecimalDecNumber::FOUR.m_value, &m_context);

   static const int BUFFER_SIZE = 256;
   char buffer[BUFFER_SIZE];
   decNumberToString(&m_value, buffer);

   char * b = buffer;
   char * e = buffer + strlen(buffer);
   char * f = std::find(b, e, 'E');
   if (f != e && (f+1) != e)
   {
      unpackScientificFormat(b, e, BUFFER_SIZE, 48);
      e = buffer + strlen(buffer);
   }

   const char * point = std::find(buffer, e, '.');

   const char * firstNonZeroDigitAfterDecimal = point;
   while (firstNonZeroDigitAfterDecimal != e && (*firstNonZeroDigitAfterDecimal < '1' || *firstNonZeroDigitAfterDecimal > '9'))
   {
      ++firstNonZeroDigitAfterDecimal;
   }

   if (firstNonZeroDigitAfterDecimal != e)
   {
      decContext tempContext;
      decContextDefault(&tempContext, DEC_INIT_BASE); 
      tempContext.traps = 0;

      // DecNumber rounding is expressed in significant figures; we want to round to a fixed number of decimal places.
      const char * firstDigit = *buffer == '-' ? (buffer+1) : buffer;
      const bool absValueLessThanOne = *firstDigit == '0';
      if (absValueLessThanOne)
      {
         tempContext.digits = decimalPlaces - (firstNonZeroDigitAfterDecimal - point - 1);

      }
      else
      {
         tempContext.digits = decimalPlaces + (point - firstDigit);
      }
      if (tempContext.digits < 0)
      {
         decNumberFromInt32(&m_value, 0);
         return *this;
      }
		 
      switch (mode)
      {
         case RoundingMode::ROUND_HALF_TO_POSITIVE_INFINITY:
            tempContext.round = isNegative() ? DEC_ROUND_HALF_DOWN : DEC_ROUND_HALF_UP;
         break;
         case RoundingMode::ROUND_HALF_TO_NEGATIVE_INFINITY: 
            tempContext.round = isNegative() ? DEC_ROUND_HALF_UP : DEC_ROUND_HALF_DOWN;
         break;
         case RoundingMode::ROUND_TO_POSITIVE_INFINITY:
            tempContext.round = isNegative() ? DEC_ROUND_DOWN : DEC_ROUND_UP;
         break;
         case RoundingMode::ROUND_TO_NEGATIVE_INFINITY:
            tempContext.round = isNegative() ? DEC_ROUND_UP : DEC_ROUND_DOWN;
         break;
         case RoundingMode::ROUND_AWAY_FROM_ZERO:
            tempContext.round = DEC_ROUND_UP;
         break;
         case RoundingMode::ROUND_TO_ZERO:
            tempContext.round = DEC_ROUND_DOWN;
         break;
         case RoundingMode::ROUND_HALF_AWAY_FROM_ZERO: 
            tempContext.round = DEC_ROUND_HALF_UP;
         break;
         case RoundingMode::ROUND_HALF_TO_ZERO: 
            tempContext.round = DEC_ROUND_HALF_DOWN; 
         break;
         case RoundingMode::ROUND_HALF_TO_EVEN:
            tempContext.round = DEC_ROUND_HALF_EVEN; 
         break;
         default :
            throw("Rounding mode is not supported - rounding using default mode which is DEC_ROUND_HALF_AWAY_FROM_ZERO");
            // LCRIT("Rounding mode[%d] is not supported - rounding using default mode which is DEC_ROUND_HALF_AWAY_FROM_ZERO", mode);
            // tempContext.round = DEC_ROUND_HALF_UP;
      }

      decNumberFromString(&m_value, buffer, &tempContext);
   }

   if (roundIncrement == RoundIncrement::HALF)
      decNumberDivide(&m_value, &m_value, &DecimalDecNumber::TWO.m_value, &m_context);
   else if (roundIncrement == RoundIncrement::QUARTER)
      decNumberDivide(&m_value, &m_value, &DecimalDecNumber::FOUR.m_value, &m_context);

   return *this;
}

DecimalDecNumber::RoundParameters::RoundParameters(uint32 decimalPlaces)
 : decimalPlaces(decimalPlaces),
   roundMode(RoundingMode::ROUND_HALF_AWAY_FROM_ZERO),
   roundIncrement(RoundIncrement::TENTH)
{
}

DecimalDecNumber::RoundParameters::RoundParameters(uint32 decimalPlaces, RoundingMode roundMode)
 : decimalPlaces(decimalPlaces),
   roundMode(roundMode),
   roundIncrement(RoundIncrement::TENTH)
{
}

DecimalDecNumber::RoundParameters::RoundParameters(uint32 decimalPlaces, RoundingMode roundMode, RoundIncrement roundIncrement)
 : decimalPlaces(decimalPlaces),
   roundMode(roundMode),
   roundIncrement(roundIncrement)
{
}

const DecimalDecNumber DecimalDecNumber::round(const RoundParameters& rhs)
{
   return round(rhs.decimalPlaces, rhs.roundMode, rhs.roundIncrement);
}

void unpackScientificFormat(char * b, char *& e, int bufferSize, int decimalPlaces)
{
  char * f = std::find(b, e, 'E');
  if (f != e && (f+1) != e)
  {
	 const bool isNegative = *b == '-';
	 if (isNegative)
	 {
		++b;
		--bufferSize;
	 }

	 const bool negativeExponent = *(f+1) == '-';
	 // Positive can be indicated by either a plus or no sign at all.
	 const bool signPresent = (*(f+1) == '-' || *(f+1) == '+');
	 const int magnitude = signPresent ? atoi(f+2) : atoi(f+1);
	 char * decimalPoint = 0;
	 int significantFigures = *(b+1) == '.' ? f - b - 1 : f - b;

	 if (negativeExponent)
	 {
		// The following algorithm is deployed         - example 1.23456789E-5
		// 1. remove the decimal place                 - example _123456789_________
		// 2. shift the normalised portion             - example ______123456789____
		// 3. add the decimal place and leading zeros  - example 0.0000123456789____
		// 4. truncate if necessary to decimalPlaces

		if (*(b+1) == '.')
		{
		   *(b+1) = *b;
		}

		e = b + 2 + (magnitude-1) + significantFigures;
		if (e - b > bufferSize)
		{
		   if (isNegative)
			  --b;
		   strcpy(b, "0.0");
		   e = b + 3;
		   return;
		}

		std::copy_backward(b, f, e);

		memset(b, '0', magnitude+1);
		decimalPoint = b + 1;
	 }
	 else 
	 {
		decimalPoint = b + magnitude + 1;

		if (significantFigures <= magnitude)
		{
		   // The following algorithm is deployed         - example 1.23456789E+15
		   // 1. remove the decimal place                 - example 123456789_________
		   // 2. add trailing zeros if required           - example 12345678900000
		   // 3. insert the new decimal place             - example 12345678900000.0
		   // 4. truncate if necessary to decimalPlaces
		   
		   e = decimalPoint + 2;
		   if (e - b > bufferSize)
		   {
			  // TODO: we could return a valid number here instead of NaN
			  //       - assuming decNumberFromString understands scientific notation,
			  //         we can reduce the number of significant figures and form it in scientific notation
			  //       unfortunately the design instruction for this class is to reason in terms of d.p. NOT s.f.
			  const int buffSz = 64;
			  char buffer[buffSz];
			  strncpy(buffer,b,buffSz-1);
			  buffer[buffSz-1] = 0;
			  // GLCRIT(DEC_NUMBER, "unpack scientific format converted[%s] to[NaN]", buffer);
			  if (isNegative)
				 --b;
			  strcpy(b, "NaN");
			  e = b + 3;
			  return;
		   }

		   // Remove the decimal place
		   if (*(b+1) == '.')
		   {
			  std::copy(b+2, f, b+1);
		   }

		   // Add trailing zeros (and also the zero after the decimal point)
		   memset(b + significantFigures, '0', magnitude - significantFigures + 3);
		}
		else
		{
		   // The following algorithm is deployed         - example 1.23456789E+5
		   // 1. remove the decimal place                 - example 12345_6789_________
		   // 2. add the new decimal place                - example 12345.6789_________
		   // 3. truncate if necessary to decimalPlaces

		   std::copy(b+2, b+2+magnitude, b+1);

		   e = f;
		}
	 }

	 if (decimalPoint != 0)
	 {
		*decimalPoint = '.';
		e = std::min(e, decimalPoint + decimalPlaces + 1);
		*e = 0;
	 }

	 // Check if we're zero (and if we are then remove the negative if necessary)
	 bool isZero = true;
	 for (char * it = b; it != e; ++it)
	 {
		if (isdigit(*it) && *it != '0')
		{
		   isZero = false;
		   break;
		}
	 }

	 if (isZero)
	 {
		if (isNegative)
		   --b;

		strcpy(b, "0.0");
		e = b + 3;
		return;
	 }
  }
}

std::string DecimalDecNumber::toStringAllowScientific(uint32 decimalPlaces) const
{
   if (!isZero() && !isInfinity() && !decNumberIsNaN(&m_value))
   {
      static const int BUFFER_SIZE = 256;
      char buffer[BUFFER_SIZE];
      {
         DecimalDecNumber rounded(*this);
         rounded.round(decimalPlaces, RoundingMode::ROUND_TO_ZERO);
         decNumberToString(&rounded.m_value, buffer);
      }

      char * e = buffer + strlen(buffer);
      char * f = std::find(buffer, e, '+');
      if (f != e)
         return buffer;
   }
   return toString(decimalPlaces);
}

// Lalit: toString should "truncate" values rather than round
std::string DecimalDecNumber::toString(uint32 decimalPlaces) const
{
   // Remove these statics if we ever get a short string optimisation in place (gcc 4.1)
   static const std::string zero = "0.0";
   static const std::string posinf = "+INF";
   static const std::string neginf = "-INF";
   static const std::string nan = "NaN";

   if (isZero())
      return zero;
   else if (isInfinity())
   {
      return isNegative() ? neginf : posinf;
   }
   else if (decNumberIsNaN(&m_value))
   {
      // TODO: is it possible to have positive/negative nans?
      return nan;
   }
  
   static const int BUFFER_SIZE = 256;
   char buffer[BUFFER_SIZE];
   {
      DecimalDecNumber rounded(*this);
      rounded.round(decimalPlaces, RoundingMode::ROUND_TO_ZERO);
      decNumberToString(&rounded.m_value, buffer);
   }

   printf("after round: %s\n", buffer);

   char * b = buffer;
   char * e = buffer + strlen(buffer);
   unpackScientificFormat(b, e, BUFFER_SIZE, decimalPlaces);

   if ((e-b) == 3 && strcmp(buffer, "NaN") == 0)
   {
      return buffer;
   }
   else if (std::find(b, e, '.') == e)
   {
      strcpy(e, ".0");
      return buffer;
   }

   {
      // This code makes the unit tests pass (which were based on the old Decimal implementation)
      // Ensure that there are no trailing zeros, except when the value is an integer, in which case there should be one trailing zero.
      char * lastNonZeroDigitAfterDecimal = e - 1;
      while (*lastNonZeroDigitAfterDecimal == '0')
      {
         --lastNonZeroDigitAfterDecimal;
      }

      if (*lastNonZeroDigitAfterDecimal == '.')
      {
         *(++lastNonZeroDigitAfterDecimal) = '0';
      }
      *(lastNonZeroDigitAfterDecimal + 1) = 0;
   }

   printf("last return\n");
   return buffer;
}

double DecimalDecNumber::toDoubleWithPossiblePrecisionLoss() const
{
   // TODO: this needs to be a lot quicker...
   return boost::lexical_cast<double>(toString(15));
}

///@brief convert number, it expects that number will be with 'zero' exponent (otherwise it will throw)
template<typename T> void convertToIntX(const decNumber* pDecNum, decContext* pContext, T& result);

///@brief implementation for int32 (just use library function)
template<>
void convertToIntX<int32>(const decNumber* pDecNum, decContext* pContext, int32& result)
{
	result = decNumberToInt32(pDecNum, pContext);
	if (pContext->status & DEC_Errors)
	{
		char sNumber[1024];
		throw("decNumberToInt32() set 'status' 0x.. during conversion of");
	}
}

///@brief implementation for int64 (no such a function in library, so do conversion 'manually')
template<>
void convertToIntX<int64>(const decNumber* pDecNum, decContext* pContext, int64& result)
{
	static const uint64 s_arrPowerOfTen[19] = {
			1, 10, 100, 1000, 10000, 100000, 1000000,
			10000000, 100000000, 1000000000, 10000000000ULL,
			100000000000ULL, 1000000000000ULL, 10000000000000ULL,
			100000000000000ULL, 1000000000000000ULL, 10000000000000000ULL,
			100000000000000000ULL, 1000000000000000000ULL };

	if (decNumberIsSpecial(pDecNum))
	{
		char sNumber[1024];
		throw("Attempt to get an integer from invalid number");
	}
	else if (pDecNum->exponent != 0)
	{
		char sNumber[1024];
		// FTHROW(InvalidArgumentException, "Unsupported exponent %d, only zero exponent is supported, number %s", pDecNum->exponent, decNumberToString(pDecNum, sNumber));
		throw("Unsupported exponent , only zero exponent is supported, number ");
	}
	else if (pDecNum->digits > (int)(sizeof(s_arrPowerOfTen)/sizeof(*s_arrPowerOfTen)))
	{
		char sNumber[1024];
		// FTHROW(InvalidArgumentException, "Overflow during conversion, number of digits is %d, number %s", pDecNum->digits, decNumberToString(pDecNum, sNumber));
		throw("Overflow during conversion, number of digits is number ");
	}
	const decNumberUnit* pDigit = pDecNum->lsu;
	uint64 nUnsignedResult = 0;
	const bool isNegative = decNumberIsNegative(pDecNum);
	const uint64 nMaxValue = isNegative ? -std::numeric_limits<int64>::min() : std::numeric_limits<int64>::max();

	for (int i = 0; i < pDecNum->digits; ++pDigit, i += DECDPUN)
	{
			const uint64 nPrev = nUnsignedResult;														// to be able to check on 'overflow'
			nUnsignedResult += *pDigit * s_arrPowerOfTen[i];
			if ((nUnsignedResult < nPrev) || (nUnsignedResult > nMaxValue))
			{
				char sNumber[1024];
				throw("Overflow during conversion, step");
				// FTHROW(InvalidArgumentException, "Overflow during conversion, step %d (%llu, %llu, %llu), number %s",
				// 		i, nUnsignedResult, nPrev, nMaxValue, decNumberToString(pDecNum, sNumber));
			}
	}
	result = isNegative ? -((int64)nUnsignedResult) : (int64)nUnsignedResult;
}

template<typename T>
const T& DecimalDecNumber::toIntX(T& result) const
{
	if (! isValid())
	{
		// FTHROW(InvalidArgumentException, "Attempt to get an integer from invalid number: %s", toString().c_str());
		throw("wwwww");
	}
	if (m_value.exponent)
	{
		static const DecimalDecNumber valueWithZeroExponent(DecimalDecNumber::ZERO);

		decNumber roundedValue;
		decContext context4rounding = m_context;
		// 'floor' our value and make it have zero exponent
		context4rounding.round = DEC_ROUND_DOWN;
		decNumberQuantize(&roundedValue, &m_value, &valueWithZeroExponent.m_value, &context4rounding);
		convertToIntX(&roundedValue, &m_context, result);
	}
	else
	{
		convertToIntX(&m_value, &m_context, result);
	}
	return result;
}

int32 DecimalDecNumber::toInt32() const
{
	int32 result;
	return toIntX(result);
}

int64 DecimalDecNumber::toInt64() const
{
	int64 result;
	return toIntX(result);
}

std::ostream & operator<<(std::ostream &os, const DecimalDecNumber &d)
{
	return os << d.toString();
}

DecimalDecNumber avg(const DecimalDecNumber &lhs, const DecimalDecNumber &rhs)
{
	return (lhs + rhs) / DecimalDecNumber::TWO;
}

DecimalDecNumber sqrt(const DecimalDecNumber &val)
{
	DecimalDecNumber result;
   decNumberSquareRoot(&result.m_value, &val.m_value, &val.m_context);
	return result;
}

DecimalDecNumber power(const DecimalDecNumber &val, const DecimalDecNumber &exponent)
{
	DecimalDecNumber result;
   decNumberPower(&result.m_value, &val.m_value, &exponent.m_value, &val.m_context);
	return result;
}

DecimalDecNumber power(const DecimalDecNumber &val, int32 exponent)
{
   decNumber dExponent;
   decNumberFromInt32(&dExponent, exponent);
   DecimalDecNumber result;
   decNumberPower(&result.m_value, &val.m_value, &dExponent, &val.m_context);
   return result;
}

DecimalDecNumber exp(const DecimalDecNumber &val)
{
   decContext contextMath;
   decContextDefault(&contextMath, DEC_INIT_BASE); // initialize
   contextMath.traps=0;                            // no traps, thank you
   contextMath.digits=DECNUMDIGITS;                // set precision
   contextMath.emax = 1000000 - 1;                 // Max for mathematical functions like exp
   contextMath.emin = -contextMath.emax;           // Max for mathematical functions

	DecimalDecNumber result;
   decNumberExp(&result.m_value, &val.m_value, &contextMath);
	return result;
}

DecimalDecNumber DecimalDecNumber::fromDecimalComponents(const int64 significand, const int32 exponent)
{
	DecimalDecNumber number(significand);
	if ( exponent != 0 )
	{
		decNumber numExponent;
		decNumberFromInt32( &numExponent, exponent );
		decNumberScaleB( &number.m_value, &number.m_value, &numExponent, &number.m_context );
	}
	return number;
}

bool DecimalDecNumber::toDecimalComponents(const DecimalDecNumber &val, int64& significand, int32& exponent)
{
	DecimalDecNumber valCopy = val;
	valCopy.m_context.digits = 18; // maximum number of digits which can be represented by int64 significand

	// Check if decimal is NaN or infinite - we can't represent these as components
	if (decNumberIsSpecial( &valCopy.m_value ))
	{
		exponent = 0;
		significand = 0;
		return false;
	}

	// Minimize the size of the number the significand needs to represent
	decNumberReduce( &valCopy.m_value, &valCopy.m_value, &valCopy.m_context );

	exponent = valCopy.m_value.exponent;

	decNumber numExponent;
	decNumberFromInt32( &numExponent, -valCopy.m_value.exponent );
	decNumberScaleB( &valCopy.m_value, &valCopy.m_value, &numExponent, &valCopy.m_context );

	if (::sscanf(valCopy.toString(0).c_str(), "%lld", &significand) != 1)
	{
		exponent = 0;
		significand = 0;
		return false;
	}

	return true;
}

const char * roundingModeToString( DecimalDecNumber::RoundingMode mode)
{
  switch (mode)
  {
    case DecimalDecNumber::RoundingMode::ROUND_HALF_TO_POSITIVE_INFINITY:
      // FALLTHROUGH
    case DecimalDecNumber::RoundingMode::ROUND_TO_POSITIVE_INFINITY:
      return "rounded up";

    case DecimalDecNumber::RoundingMode::ROUND_HALF_TO_NEGATIVE_INFINITY:
      // FALLTHROUGH
    case DecimalDecNumber::RoundingMode::ROUND_TO_NEGATIVE_INFINITY:
      return "rounded down";

    case DecimalDecNumber::RoundingMode::ROUND_AWAY_FROM_ZERO:
      return "rounded away from zero";
    case DecimalDecNumber::RoundingMode::ROUND_TO_ZERO:
      return "rounded towards zero";
    case DecimalDecNumber::RoundingMode::ROUND_HALF_AWAY_FROM_ZERO:
      return "rounded half from zero";
    case DecimalDecNumber::RoundingMode::ROUND_HALF_TO_ZERO:
      return "rounded half towards zero";
    case DecimalDecNumber::RoundingMode::ROUND_HALF_TO_EVEN:
      return "rounded half even";
  };

  return "rounded";
}

