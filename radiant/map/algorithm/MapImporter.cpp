#include "MapImporter.h"

#include "i18n.h"
#include "ientity.h"
#include "imap.h"
#include "imainframe.h"

#include <fmt/format.h>
#include "registry/registry.h"
#include "string/string.h"
#include "wxutil/dialog/MessageBox.h"

namespace map
{

namespace
{
	const char* const RKEY_MAP_LOAD_STATUS_INTERLEAVE = "user/ui/map/loadStatusInterleave";
	std::size_t EMPTY_PRIMITVE_NUM = std::numeric_limits<std::size_t>::max();
}

MapImporter::MapImporter(const scene::INodePtr& root, std::istream& inputStream) :
	_root(root),
	_dialogEventLimiter(registry::getValue<int>(RKEY_MAP_LOAD_STATUS_INTERLEAVE)),
	_entityCount(0),
	_primitiveCount(0),
	_inputStream(inputStream),
	_fileSize(0)
{
	// Get the file size, for handling the progress dialog
	_inputStream.seekg(0, std::ios::end);
	_fileSize = static_cast<std::size_t>(_inputStream.tellg());

	// Move the pointer back to the beginning of the file
	_inputStream.seekg(0, std::ios::beg);

	bool showProgressDialog = !registry::getValue<bool>(RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG);

	if (showProgressDialog)
	{
		_dialog.reset(new wxutil::ModalProgressDialog(_("Loading map")));

		// Initialise the text
		_dlgEntityText = fmt::format(_("Loading entity {0:d}\n"), _entityCount);
	}
}

bool MapImporter::addEntity(const scene::INodePtr& entityNode)
{
	// Keep track of this entity
	_nodes.insert(NodeIndexMap::value_type(
		NodeIndexPair(_entityCount, EMPTY_PRIMITVE_NUM), entityNode));

	_entityCount++;

	if (_dialog)
	{
		// Update the dialog text
		_dlgEntityText = fmt::format(_("Loading entity {0:d}\n"), _entityCount);

		// Update the dialog text. This will throw an exception if the cancel
		// button is clicked, which we must catch and handle.
		if (_dialogEventLimiter.readyForEvent())
		{
			// Let the OperationAbortedException fall through, it will be caught in the MapResource class
			_dialog->setTextAndFraction(_dlgEntityText, getProgressFraction());
		}
	}

	_root->addChildNode(entityNode);

	return true;
}

bool MapImporter::addPrimitiveToEntity(const scene::INodePtr& primitive, const scene::INodePtr& entity)
{
	_nodes.insert(NodeIndexMap::value_type(
		NodeIndexPair(_entityCount, _primitiveCount), primitive));

	_primitiveCount++;

	if (_dialog && _dialogEventLimiter.readyForEvent())
    {
		_dialog->setTextAndFraction(
            _dlgEntityText + "Primitive " + string::to_string(_primitiveCount),
			getProgressFraction()
        );
    }

	if (Node_getEntity(entity)->isContainer())
	{
		entity->addChildNode(primitive);
		return true;
	}
	else
	{
		return false;
	}
}

const NodeIndexMap& MapImporter::getNodeMap() const
{
	return _nodes;
}

double MapImporter::getProgressFraction()
{
	long readBytes = static_cast<long>(_inputStream.tellg());
	return static_cast<double>(readBytes) / _fileSize;
}

} // namespace
