#ifndef REPORT_ERROR_H
#define REPORT_ERROR_H

#include <iostream>

//Enable printf debug-output
#define printfDebug

namespace readable
{
	inline void reportError(std::string ErrorMessage)
	{
		std::cerr << ErrorMessage;
		#ifdef printfDebug
			printf( ErrorMessage.c_str() );
		#endif
		throw std::runtime_error(ErrorMessage);
	}
}

#endif	//REPORT_ERROR_H