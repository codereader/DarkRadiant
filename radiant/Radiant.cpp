#include "iradiant.h"
#include "module/CoreModule.h"

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
	{}
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
