#include "MapImporter.h"

#include "i18n.h"
#include "ientity.h"
#include "imap.h"
#include "imainframe.h"

#include <boost/format.hpp>
#include "registry/registry.h"
#include "string/string.h"
#include "gtkutil/dialog/MessageBox.h"

namespace map
{

namespace
{
	const char* const RKEY_MAP_LOAD_STATUS_INTERLEAVE = "user/ui/map/loadStatusInterleave";
}

MapImporter::MapImporter(const scene::INodePtr& root, std::istream& inputStream) :
	_root(root),
	_dialogEventLimiter(registry::getValue<int>(RKEY_MAP_LOAD_STATUS_INTERLEAVE)),
	_inputStream(inputStream),
	_fileSize(0)
{
	// Set the default capacity of the vector containers
	_entities.reserve(256);
	_primitives.reserve(512);

	// Get the file size, for handling the progress dialog
	_inputStream.seekg(0, std::ios::end);
	_fileSize = static_cast<std::size_t>(_inputStream.tellg());

	// Move the pointer back to the beginning of the file
	_inputStream.seekg(0, std::ios::beg);

	bool showProgressDialog = !registry::getValue<bool>(RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG);

	if (showProgressDialog)
	{
		_dialog = gtkutil::ModalProgressDialogPtr(
			new gtkutil::ModalProgressDialog(
				GlobalMainFrame().getTopLevelWindow(), _("Loading map")
			)
		);

		// Initialise the text
		_dlgEntityText = (boost::format(_("Loading entity %d")) % _entities.size()).str();
	}
}

bool MapImporter::addEntity(const scene::INodePtr& entityNode)
{
	// Double the vector capacity if we're reaching the current cap
	if (_entities.capacity() == _entities.size())
	{
		_entities.reserve(_entities.capacity()*2);
	}

	_entities.push_back(entityNode);

	if (_dialog)
	{
		// Update the dialog text
		_dlgEntityText = (boost::format(_("Loading entity %d")) % _entities.size()).str();

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
	// Double the vector capacity if we're reaching the current cap
	if (_primitives.capacity() == _primitives.size())
	{
		_primitives.reserve(_primitives.capacity()*2);
	}

	_primitives.push_back(primitive);

	if (_dialog && _dialogEventLimiter.readyForEvent())
    {
		_dialog->setTextAndFraction(
            _dlgEntityText + "\nPrimitive " + string::to_string(_primitives.size()),
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

scene::INodePtr MapImporter::getEntity(std::size_t entityNum)
{
	return (entityNum >= 0 && entityNum < _entities.size()) ? _entities[entityNum] : scene::INodePtr();
}

scene::INodePtr MapImporter::getPrimitive(std::size_t primitiveNum)
{
	return (primitiveNum >= 0 && primitiveNum < _primitives.size()) ? _primitives[primitiveNum] : scene::INodePtr();
}

double MapImporter::getProgressFraction()
{
	long readBytes = static_cast<long>(_inputStream.tellg());
	return static_cast<double>(readBytes) / _fileSize;
}

} // namespace
