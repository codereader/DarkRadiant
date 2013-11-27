#include "EntityClassTreePopulator.h"
#include "EntityClassChooser.h"

#include "iregistry.h"
#include "iradiant.h"
#include "iuimanager.h"
#include "gamelib.h"

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
EntityClassTreePopulator::EntityClassTreePopulator(const Glib::RefPtr<Gtk::TreeStore>& store,
												   const EntityClassChooser::TreeColumns& columns)
: gtkutil::VFSTreePopulator(store),
  _store(store),
  _columns(columns),
  _folderKey(game::current::getValue<std::string>(FOLDER_KEY_PATH)),
  _folderIcon(GlobalUIManager().getLocalPixbuf(FOLDER_ICON)),
  _entityIcon(GlobalUIManager().getLocalPixbuf(ENTITY_ICON))
{}

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

void EntityClassTreePopulator::visit(const Glib::RefPtr<Gtk::TreeStore>& store,
									 const Gtk::TreeModel::iterator& iter,
									 const std::string& path,
									 bool isExplicit)
{
	Gtk::TreeModel::Row row = *iter;

	// Get the display name by stripping off everything before the last slash
	row[_columns.name] = path.substr(path.rfind("/") + 1);
	row[_columns.isFolder] = !isExplicit;
	row[_columns.icon] = isExplicit ? _entityIcon : _folderIcon;
}

} // namespace ui
