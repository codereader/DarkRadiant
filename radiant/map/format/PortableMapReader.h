#pragma once

#include <map>
#include "inode.h"
#include "imapformat.h"
#include "parser/DefTokeniser.h"

namespace map 
{

class PortableMapReader :
	public IMapReader
{
protected:
	IMapImportFilter& _importFilter;

	// The map type for one entity's keyvalues (spawnargs)
	typedef std::map<std::string, std::string> EntityKeyValues;

	// The number of entities found in this map file so far
	std::size_t _entityCount;

	// The number of primitives of the currently parsed entity
	std::size_t _primitiveCount;

public:
	PortableMapReader(IMapImportFilter& importFilter);

	// IMapReader implementation
	virtual void readFromStream(std::istream& stream);

	static bool CanLoad(std::istream& stream);
};

} // namespace map
