#pragma once

#include "inode.h"
#include "imapformat.h"
#include "igame.h"

namespace map
{

/**
 * Walker class which passes the visited scene nodes to the
 * attached MapExporter class, for writing it to the given
 * string output stream. The exporter gets a chance to catch
 * various events during scene traversal, like PreMap(), 
 * PostMap(), PreEntity(), PostEntity(), PrePrimitive(), etc.
 */
class MapExporter :
	public scene::NodeVisitor
{
private:
	// The actual map format for writing nodes to the stream
	IMapWriter& _writer;

	// The stream we're writing to
	std::ostream& _mapStream;

	// The root node of the subgraph to be exported
	scene::INodePtr _root;

public:
	// The constructor prepares the scene and the output stream
	MapExporter(IMapWriter& writer, const scene::INodePtr& root, std::ostream& mapStream);

	// Cleans up the scene on destruction
	~MapExporter();

	// NodeVisitor implementation, is called by the traversal func passed to MapResource
	bool pre(const scene::INodePtr& node);
	void post(const scene::INodePtr& node);

private:
	// Is called before exporting the scene to prepare func_* groups.
	void prepareScene();

	// Called after all the writing has been performed, cleans up func_* groups
	void finishScene();
};

} // namespace
