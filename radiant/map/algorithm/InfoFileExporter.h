#pragma once

#include <ostream>
#include "inode.h"

namespace map
{

class InfoFileExporter
{
private:
	// The stream we're writing to
	std::ostream& _stream;

	// Number of node-to-layer mappings written
	std::size_t _layerInfoCount;

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
};
typedef boost::shared_ptr<InfoFileExporter> InfoFileExporterPtr;

} // namespace
