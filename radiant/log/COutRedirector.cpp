#include "COutRedirector.h"

#include <iostream>

namespace applog {

COutRedirector::COutRedirector(ILogWriter& logWriter)
{
	// Remember the old std::cout buffer
	_oldCOutStreamBuf = std::cout.rdbuf();
	_oldCErrStreamBuf = std::cerr.rdbuf();

	std::cout.rdbuf(logWriter.getLogStream(LogLevel::Standard).rdbuf());
	std::cerr.rdbuf(logWriter.getLogStream(LogLevel::Error).rdbuf());
}

COutRedirector::~COutRedirector()
{
	std::cout.rdbuf(_oldCOutStreamBuf);
	std::cerr.rdbuf(_oldCErrStreamBuf);
}

// A call to init() will redirect the std::cout output to the log
void COutRedirector::init(ILogWriter& logWriter)
{
	if (!InstancePtr())
	{
		InstancePtr().reset(new COutRedirector(logWriter));
	}
}

// A call to destroy() will stop redirecting std::cout
void COutRedirector::destroy()
{
	// Clear the shared_ptr, this will disable all redirects
	InstancePtr().reset();
}

// Contains the static singleton instance
COutRedirectorPtr& COutRedirector::InstancePtr()
{
	static COutRedirectorPtr _instancePtr;
	return _instancePtr;
}

} // namespace applog
