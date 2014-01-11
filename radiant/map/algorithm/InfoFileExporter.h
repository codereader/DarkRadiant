#pragma once

#include <ostream>
#include <sstream>
#include "inode.h"
#include "iselectionset.h"
#include <map>

namespace map
{

class InfoFileExporter
{
private:
	// The stream we're writing to
	std::ostream& _stream;

	// Number of node-to-layer mappings written
	std::size_t _layerInfoCount;

	struct SelectionSetExportInfo
	{
		// The set we're working with
		selection::ISelectionSetPtr set;

		// The nodes in this set
		std::set<scene::INodePtr> nodes;

		// The entity and primitive number pair
		typedef std::pair<std::size_t, std::size_t> IndexPair;

		// The node indices, which will be resolved during traversal
		std::set<IndexPair> nodeIndices;
	};

	// SelectionSet-related
	typedef std::vector<SelectionSetExportInfo> SelectionSetInfo;
	SelectionSetInfo _selectionSetInfo;

public:
	// The constructor prepares the output stream
	InfoFileExporter(std::ostream& stream);

	// Cleans up the scene on destruction
	~InfoFileExporter();

	// Is called by the owning MapExporter
	// Requirements: node must not be NULL and not a model/particle node.
	void visitEntity(const scene::INodePtr& node, std::size_t entityNum);
	void visitPrimitive(const scene::INodePtr& node, std::size_t entityNum, std::size_t primitiveNum);

private:
	// General handling of map nodes
	void handleNode(const scene::INodePtr& node);

	// Writes the names of the layers existing in this map
	void writeLayerNames();

	void writeSelectionSetInfo();

	// Get SelectionSet node mapping
	void assembleSelectionSetInfo();
};
typedef boost::shared_ptr<InfoFileExporter> InfoFileExporterPtr;

} // namespace
