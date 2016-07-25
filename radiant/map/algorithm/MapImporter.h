#pragma once

#include "inode.h"
#include "imapformat.h"
#include "imapinfofile.h"
#include <map>

#include "wxutil/ModalProgressDialog.h"
#include "EventRateLimiter.h"

namespace map
{

/**
 * A default map import filter/handler. An instance of this class
 * is required to get a valid IMapReader from the MapFormat module.
 *
 * During map parsing the methods addEntity/addPrimitiveToEntity are called
 * by the parser module to insert the newly created nodes into the scene.
 *
 * This IMapImportFilter implementation is handling the modal progress dialog too.
 */
class MapImporter :
	public IMapImportFilter
{
private:
	scene::INodePtr _root;

	// The progress dialog
	wxutil::ModalProgressDialogPtr _dialog;

	// The progress dialog text for the current entity
	std::string _dlgEntityText;

    // Event rate limiter for the progress dialog
    EventRateLimiter _dialogEventLimiter;

	// Current entity/primitive number for progress display
	std::size_t _entityCount;
	std::size_t _primitiveCount;

	// The size of the input file, can be 0 for dummy progress bar
	std::istream& _inputStream;
	std::size_t _fileSize;

	// Keep track of all the entities and primitives for later retrieval
	NodeIndexMap _nodes;

public:
	MapImporter(const scene::INodePtr& root, std::istream& inputStream);

	bool addEntity(const scene::INodePtr& entityNode);
	bool addPrimitiveToEntity(const scene::INodePtr& primitive, const scene::INodePtr& entity);

	const NodeIndexMap& getNodeMap() const;

private:
	double getProgressFraction();
};

} // namespace
