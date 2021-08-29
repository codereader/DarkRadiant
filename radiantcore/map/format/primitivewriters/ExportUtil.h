#pragma once

#include <ostream>
#include "math/FloatTools.h"

namespace map
{

// Writes a double to the given stream and checks for NaN and infinity
inline void writeDoubleSafe(const double d, std::ostream& os)
{
	if (isValid(d))
	{
		if (d == -0.0)
		{
			os << 0; // convert -0 to 0
		}
		else
		{
			os << d;
		}
	}
	else
	{
		// Is infinity or NaN, write 0
		os << "0";
	}
}

}
