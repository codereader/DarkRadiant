#ifndef _COUT_REDIRECTOR_H_
#define _COUT_REDIRECTOR_H_

#include "LogStream.h"
#include <boost/shared_ptr.hpp>

namespace applog {

// shared_ptr forward declaration
class COutRedirector;
typedef boost::shared_ptr<COutRedirector> COutRedirectorPtr;

class COutRedirector
{
	std::streambuf* _oldCOutStreamBuf;
	std::streambuf* _oldCErrStreamBuf;
public:
	COutRedirector();
	~COutRedirector();

	// A call to init() will redirect the std::cout output to the log
	static void init();

	// A call to destroy() will stop redirecting std::cout
	static void destroy();

private:
	// Contains the static singleton instance
	static COutRedirectorPtr& InstancePtr();
};

} // namespace applog

#endif /* _COUT_REDIRECTOR_H_ */
