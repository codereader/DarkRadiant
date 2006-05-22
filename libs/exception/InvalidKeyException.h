#ifndef INVALIDKEYEXCEPTION_H_
#define INVALIDKEYEXCEPTION_H_

#include "RadiantException.h"

class InvalidKeyException:
	public RadiantException
{
public:
	InvalidKeyException();
	virtual ~InvalidKeyException();
};

#endif /*INVALIDKEYEXCEPTION_H_*/
