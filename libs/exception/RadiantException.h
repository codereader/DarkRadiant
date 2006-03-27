#ifndef RADIANTEXCEPTION_H_
#define RADIANTEXCEPTION_H_

#include <iostream>

class RadiantException
{
protected:
	std::string text;
public:
	RadiantException();
	RadiantException(std::string inText) {
		text = inText;
	}
	virtual ~RadiantException() {}
	void printError() {
		std::cerr << "RadiantException: " << text << std::endl;
	}
};

#endif /*RADIANTEXCEPTION_H_*/
