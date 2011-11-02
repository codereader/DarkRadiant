#pragma once

#include "inode.h"
#include "imapformat.h"
#include "igame.h"

#include "gtkutil/ModalProgressDialog.h"
#include "EventRateLimiter.h"

namespace map
{

/**
 * Walker class which passes the visited scene nodes to the
 * attached MapExporter class, for writing it to the given
 * string output stream. The exporter is using a IMapWriter class
 * to dispatch various calls like beginWriteEntity(), 
 * beginMap(), endWriteBrush() during scene traversal etc.
 *
 * If the progress dialog is enabled (i.e. nodeCount > 0 in constructor)
 * a gtkutil::OperationAbortedException& might be thrown during traversal, 
 * the calling code needs to be able to handle that.
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

	// The progress dialog
	gtkutil::ModalProgressDialogPtr _dialog;

    // Event rate limiter for the progress dialog
    EventRateLimiter _dialogEventLimiter;

	// The total number, used for progress measurement
	std::size_t _totalNodeCount;
	std::size_t	_curNodeCount;

public:
	// The constructor prepares the scene and the output stream
	MapExporter(IMapWriter& writer, const scene::INodePtr& root, std::ostream& mapStream, std::size_t nodeCount = 0);

	// Cleans up the scene on destruction
	~MapExporter();

	// Entry point for traversing the given root node using the given traversal function
	virtual void exportMap(const scene::INodePtr& root, const GraphTraversalFunc& traverse);

	void enableProgressDialog();
	void disableProgressDialog();

	// NodeVisitor implementation, is called by the traversal func passed to MapResource
	bool pre(const scene::INodePtr& node);
	void post(const scene::INodePtr& node);

private:
	void onNodeProgress();

	// Is called before exporting the scene to prepare func_* groups.
	void prepareScene();

	// Called after all the writing has been performed, cleans up func_* groups
	void finishScene();
};

} // namespace
