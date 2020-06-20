#include "MapImporter.h"

#include "i18n.h"
#include "ientity.h"
#include "imap.h"
#include "iradiant.h"
#include "imainframe.h"

#include <fmt/format.h>
#include "registry/registry.h"
#include "string/string.h"
#include "messages/MapFileOperation.h"

namespace map
{

namespace
{
	const char* const RKEY_MAP_LOAD_STATUS_INTERLEAVE = "user/ui/map/loadStatusInterleave";
	std::size_t EMPTY_PRIMITVE_NUM = std::numeric_limits<std::size_t>::max();
}

MapImporter::MapImporter(const scene::IMapRootNodePtr& root, std::istream& inputStream) :
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

	FileOperation startedMsg(FileOperation::Type::Import, FileOperation::Started, _fileSize > 0);
	GlobalRadiantCore().getMessageBus().sendMessage(startedMsg);

	// Initialise the text
	_dlgEntityText = fmt::format(_("Loading entity {0:d}\n"), _entityCount);
}

MapImporter::~MapImporter()
{
	// Send the final finished message to give the UI a chance to close progress dialogs
	FileOperation startedMsg(FileOperation::Type::Import, FileOperation::Finished, _fileSize > 0);
	GlobalRadiantCore().getMessageBus().sendMessage(startedMsg);
}

const scene::IMapRootNodePtr& MapImporter::getRootNode() const
{
	return _root;
}

bool MapImporter::addEntity(const scene::INodePtr& entityNode)
{
	// Keep track of this entity
	_nodes.insert(NodeIndexMap::value_type(
		NodeIndexPair(_entityCount, EMPTY_PRIMITVE_NUM), entityNode));

	_entityCount++;

	// Update the dialog text
	_dlgEntityText = fmt::format(_("Loading entity {0:d}\n"), _entityCount);

	if (_dialogEventLimiter.readyForEvent())
	{
		FileOperation msg(FileOperation::Type::Import, FileOperation::Progress, _fileSize > 0, getProgressFraction());
		msg.setText(_dlgEntityText);
		GlobalRadiantCore().getMessageBus().sendMessage(msg);
	}

	_root->addChildNode(entityNode);

	return true;
}

bool MapImporter::addPrimitiveToEntity(const scene::INodePtr& primitive, const scene::INodePtr& entity)
{
	_nodes.insert(NodeIndexMap::value_type(
		NodeIndexPair(_entityCount, _primitiveCount), primitive));

	_primitiveCount++;

	if (_dialogEventLimiter.readyForEvent())
	{
		// Update the dialog text
		FileOperation msg(FileOperation::Type::Import, FileOperation::Progress, _fileSize > 0, getProgressFraction());
		msg.setText(_dlgEntityText + fmt::format(_("Primitive {0:d}"), _primitiveCount));
		GlobalRadiantCore().getMessageBus().sendMessage(msg);
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

float MapImporter::getProgressFraction()
{
	long readBytes = static_cast<long>(_inputStream.tellg());
	return static_cast<float>(readBytes) / _fileSize;
}

} // namespace
