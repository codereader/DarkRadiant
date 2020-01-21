#pragma once

#include <map>
#include "inode.h"
#include "imapformat.h"
#include "iselectionset.h"
#include "parser/DefTokeniser.h"

namespace map 
{

namespace format
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

	typedef std::map<std::size_t, selection::ISelectionSetPtr> SelectionSets;
	SelectionSets _selectionSets;

public:
	PortableMapReader(IMapImportFilter& importFilter);

	// IMapReader implementation
	virtual void readFromStream(std::istream& stream);

	static bool CanLoad(std::istream& stream);

private:
	void readLayers(const xml::Node& mapNode);
	void readSelectionGroups(const xml::Node& mapNode);
	void readSelectionSets(const xml::Node& mapNode);
	void readMapProperties(const xml::Node& mapNode);
	void readEntities(const xml::Node& mapNode);
	scene::INodePtr readEntity(const xml::Node& entityNode);
	void readPrimitives(const xml::Node& primitivesNode, const scene::INodePtr& entity);
	void readBrush(const xml::Node& brushNode, const scene::INodePtr& entity);
	void readPatch(const xml::Node& patchNode, const scene::INodePtr& entity);
};

}

} // namespace map
