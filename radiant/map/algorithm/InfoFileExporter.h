#pragma once

#include <ostream>
#include <sstream>
#include "inode.h"
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

		// The node indices, which will be resolved during traversal
		std::set<std::size_t> nodeIndices;
	};

	// SelectionSet-related
	typedef std::vector<SelectionSetExportInfo> SelectionSetInfo;
	SelectionSetInfo _selectionSetInfo;

	// Zero-based counter for all exported nodes
	std::size_t _nodeIndex;

public:
	// The constructor prepares the output stream
	InfoFileExporter(std::ostream& stream);

	// Cleans up the scene on destruction
	~InfoFileExporter();

	// NodeVisitor implementation, is called by the owning MapExporter
	// Requirements: node must not be NULL and not a model/particle node.
	void visit(const scene::INodePtr& node);

private:
	// Writes the names of the layers existing in this map
	void writeLayerNames();

	void writeSelectionSetInfo();

	// Get SelectionSet node mapping
	void assembleSelectionSetInfo();
};
typedef boost::shared_ptr<InfoFileExporter> InfoFileExporterPtr;

} // namespace
