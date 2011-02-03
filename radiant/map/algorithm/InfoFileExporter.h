#pragma once

#include <ostream>
#include "inode.h"

namespace map
{

class InfoFileExporter :
	public scene::NodeVisitor
{
private:
	// The stream we're writing to
	std::ostream& _stream;

	// The root node of the subgraph to be exported
	scene::INodePtr _root;

	// Number of node-to-layer mappings written
	std::size_t _layerInfoCount;

public:
	// The constructor prepares the scene and the output stream
	InfoFileExporter(const scene::INodePtr& root, std::ostream& stream);

	// Cleans up the scene on destruction
	~InfoFileExporter();

	// NodeVisitor implementation, is called by the traversal func passed to MapResource
	bool pre(const scene::INodePtr& node);

private:
	// Writes the names of the layers existing in this map
	void writeLayerNames();
};

} // namespace
