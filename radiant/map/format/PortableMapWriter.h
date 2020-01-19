#pragma once

#include "imapformat.h"
#include "iselectionset.h"

#include "xmlutil/Document.h"

namespace map
{

/**
 * Exporter class writing the map data into an XML-based file format.
 */
class PortableMapWriter :
	public IMapWriter
{
protected:
	// The counters for numbering the comments
	std::size_t _entityCount;
	std::size_t _primitiveCount;

	xml::Document _document;

	xml::Node _map;
	xml::Node _curEntityPrimitives;

	struct SelectionSetExportInfo
	{
		std::size_t index;

		// The nodes in this set
		std::set<scene::INodePtr> nodes;
	};

	// SelectionSet-related
	typedef std::vector<SelectionSetExportInfo> SelectionSetInfo;
	SelectionSetInfo _selectionSets;

public:
	PortableMapWriter();

	virtual void beginWriteMap(const scene::IMapRootNodePtr& root, std::ostream& stream) override;
	virtual void endWriteMap(const scene::IMapRootNodePtr& root, std::ostream& stream) override;

	// Entity export methods
	virtual void beginWriteEntity(const IEntityNodePtr& entity, std::ostream& stream) override;
	virtual void endWriteEntity(const IEntityNodePtr& entity, std::ostream& stream) override;

	// Brush export methods
	virtual void beginWriteBrush(const IBrushNodePtr& brush, std::ostream& stream) override;
	virtual void endWriteBrush(const IBrushNodePtr& brush, std::ostream& stream) override;

	// Patch export methods
	virtual void beginWritePatch(const IPatchNodePtr& patch, std::ostream& stream) override;
	virtual void endWritePatch(const IPatchNodePtr& patch, std::ostream& stream) override;

private:
	void appendLayerInformation(xml::Node& xmlNode, const scene::INodePtr& sceneNode);
	void appendSelectionGroupInformation(xml::Node& xmlNode, const scene::INodePtr& sceneNode);
	void appendSelectionSetInformation(xml::Node& xmlNode, const scene::INodePtr& sceneNode);
};

} // namespace
