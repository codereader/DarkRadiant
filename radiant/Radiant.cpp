#include "iradiant.h"
#include "module/CoreModule.h"

#include "log/LogStream.h"
#include "log/LogWriter.h"

namespace radiant
{

class Radiant :
	public IRadiant
{
private:
	ApplicationContext& _context;

public:
	Radiant(ApplicationContext& context) :
		_context(context)
	{
		// Set the stream references for rMessage(), redirect std::cout, etc.
		applog::LogStream::InitialiseStreams(getLogWriter());
	}

	~Radiant()
	{
		applog::LogStream::ShutdownStreams();
	}

	applog::ILogWriter& getLogWriter() override
	{
		return applog::LogWriter::Instance();
	}
};

}

extern "C" DARKRADIANT_DLLEXPORT radiant::IRadiant* SYMBOL_CREATE_RADIANT(ApplicationContext& context)
{
	return new radiant::Radiant(context);
}

extern "C" DARKRADIANT_DLLEXPORT void SYMBOL_DESTROY_RADIANT(radiant::IRadiant* radiant)
{
	delete radiant;
}
