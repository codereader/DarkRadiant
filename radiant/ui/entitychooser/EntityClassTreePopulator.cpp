#include "EntityClassTreePopulator.h"
#include "EntityClassChooser.h"

#include "iregistry.h"
#include "iradiant.h"
#include "iuimanager.h"
#include "gamelib.h"

#include <wx/artprov.h>

namespace ui
{

namespace
{
	const char* const FOLDER_ICON = "folder16.png";
	const char* const ENTITY_ICON = "cmenu_add_entity.png";

	// Registry XPath to lookup key that specifies the display folder
    const char* const FOLDER_KEY_PATH = "/entityChooser/displayFolderKey";
}

// Constructor
EntityClassTreePopulator::EntityClassTreePopulator(wxutil::TreeModel::Ptr store,
												   const EntityClassChooser::TreeColumns& columns)
: wxutil::VFSTreePopulator(store),
  _store(store),
  _columns(columns),
  _folderKey(game::current::getValue<std::string>(FOLDER_KEY_PATH))
{
	_folderIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON));
	_entityIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + ENTITY_ICON));
}

// Required visit function
void EntityClassTreePopulator::visit(const IEntityClassPtr& eclass)
{
	std::string folderPath = eclass->getAttribute(_folderKey).getValue();

    if (!folderPath.empty())
	{
        folderPath = "/" + folderPath;
	}

	// Create the folder to put this EntityClass in, depending on the value
	// of the DISPLAY_FOLDER_KEY.
    addPath(eclass->getModName() + folderPath + "/" + eclass->getName());
}

void EntityClassTreePopulator::visit(wxutil::TreeModel& /* store */,
				wxutil::TreeModel::Row& row, const std::string& path, bool isExplicit)
{
	// Get the display name by stripping off everything before the last slash
	row[_columns.name] = wxVariant(wxDataViewIconText(
		path.substr(path.rfind("/") + 1), isExplicit ? _entityIcon : _folderIcon));
	row[_columns.isFolder] = !isExplicit;

	row.SendItemAdded();
}

} // namespace ui
