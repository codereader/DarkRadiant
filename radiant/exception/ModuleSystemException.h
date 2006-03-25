#ifndef MODULESYSTEMEXCEPTION_H_
#define MODULESYSTEMEXCEPTION_H_

#include "RadiantException.h"

class ModuleSystemException: public RadiantException
{
public:
	ModuleSystemException(std::string inText) {
		text = inText;
	}
};

#endif /*MODULESYSTEMEXCEPTION_H_*/
