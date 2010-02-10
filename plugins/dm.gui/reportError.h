#include <iostream>
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