#include "COutRedirector.h"

#include <iostream>

namespace applog {

COutRedirector::COutRedirector() {
	// Remember the old std::cout buffer
	_oldCOutStreamBuf = std::cout.rdbuf();
	_oldCErrStreamBuf = std::cerr.rdbuf();

	std::cout.rdbuf(getGlobalOutputStream().rdbuf());
	std::cerr.rdbuf(getGlobalErrorStream().rdbuf());
}

COutRedirector::~COutRedirector() {
	std::cout.rdbuf(_oldCOutStreamBuf);
	std::cerr.rdbuf(_oldCErrStreamBuf);
}

// A call to init() will redirect the std::cout output to the log
void COutRedirector::init() {
	if (InstancePtr() == NULL) {
		InstancePtr() = COutRedirectorPtr(new COutRedirector);
	}
}

// A call to destroy() will stop redirecting std::cout
void COutRedirector::destroy() {
	// Clear the shared_ptr, this will 
	InstancePtr() = COutRedirectorPtr();
}

// Contains the static singleton instance
COutRedirectorPtr& COutRedirector::InstancePtr() {
	static COutRedirectorPtr _instancePtr;
	return _instancePtr;
}

} // namespace applog
