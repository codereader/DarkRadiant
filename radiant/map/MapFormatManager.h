#ifndef _MapFormatManager_h__
#define _MapFormatManager_h__

#include "imapformat.h"

namespace map
{

class MapFormatManager :
	public IMapFormatManager
{
public:
	MapFormatPtr getMapFormat(const std::string& name);

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
};

}

#endif // _MapFormatManager_h__
