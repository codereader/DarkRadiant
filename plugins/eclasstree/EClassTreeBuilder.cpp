#include "EClassTreeBuilder.h"

#include "itextstream.h"
#include "iuimanager.h"
#include "EClassTree.h"

#include <gtkmm/treemodel.h>

namespace ui {

	namespace
	{
		const char* ENTITY_ICON = "cmenu_add_entity.png";
		const std::string INHERIT_KEY("inherit");
	}

EClassTreeBuilder::EClassTreeBuilder(const Glib::RefPtr<Gtk::TreeStore>& targetStore,
									 const EClassTreeColumns& columns) :
	_treePopulator(targetStore),
	_columns(columns),
	_entityIcon(GlobalUIManager().getLocalPixbuf(ENTITY_ICON))
{
	// Travese the entity classes, this will call visit() for each eclass
	GlobalEntityClassManager().forEachEntityClass(*this);

	// Visit the tree populator in order to fill in the column data
	_treePopulator.forEachNode(*this);
}

void EClassTreeBuilder::visit(const IEntityClassPtr& eclass)
{
	std::string fullPath;

	// Prefix mod name
	fullPath = eclass->getModName() + "/";

	// Prefix inheritance path (recursively)
	fullPath += getInheritancePathRecursive(eclass);

	// The entityDef name itself
	fullPath += eclass->getName();

	// Let the VFSTreePopulator do the insertion
	_treePopulator.addPath(fullPath);
}

void EClassTreeBuilder::visit(const Glib::RefPtr<Gtk::TreeStore>& store,
							  const Gtk::TreeModel::iterator& iter,
							  const std::string& path,
							  bool isExplicit)
{
	// Get the display path, everything after rightmost slash
	Gtk::TreeModel::Row row = *iter;
	row[_columns.name] = path.substr(path.rfind("/") + 1);
	row[_columns.icon] = _entityIcon;
}

std::string EClassTreeBuilder::getInheritancePathRecursive(const IEntityClassPtr& eclass) {
	std::string returnValue;

	try {
		EntityClassAttribute attribute = eclass->getAttribute(INHERIT_KEY);

		// Don't use empty or derived "inherit" keys
		if (!attribute.getValue().empty() && !attribute.inherited) {

			// Get the inherited eclass first and resolve the path
			IEntityClassPtr parent = GlobalEntityClassManager().findClass(
				attribute.getValue()
			);

			if (parent != NULL) {
				returnValue += getInheritancePathRecursive(parent);
			}
			else {
				rError() << "EClassTreeBuilder: Cannot resolve inheritance path for "
					<< eclass->getName() << std::endl;
			}

			returnValue += attribute.getValue() + "/";
		}
	}
	catch (std::runtime_error&) {
		// no inherit key
	}

	return returnValue;
}

} // namespace ui

