#ifndef ENTITYCLASSTREEPOPULATOR_H_
#define ENTITYCLASSTREEPOPULATOR_H_

#include "ieclass.h"

#include <map>
#include <string>
#include "gtkutil/VFSTreePopulator.h"
#include "EntityClassChooser.h"

namespace ui
{

/**
 * EntityClassVisitor which populates a Gtk::TreeStore with entity classnames
 * taking account of display folders and mod names.
 */
class EntityClassTreePopulator :
	public gtkutil::VFSTreePopulator,
	public gtkutil::VFSTreePopulator::Visitor,
	public EntityClassVisitor
{
private:
    // TreeStore to populate
    Glib::RefPtr<Gtk::TreeStore> _store;

	// Column definition
	const EntityClassChooser::TreeColumns& _columns;

    // Key that specifies the display folder
    std::string _folderKey;

	Glib::RefPtr<Gdk::Pixbuf> _folderIcon;
	Glib::RefPtr<Gdk::Pixbuf> _entityIcon;

public:
    // Constructor
	EntityClassTreePopulator(const Glib::RefPtr<Gtk::TreeStore>& store,
							 const EntityClassChooser::TreeColumns& columns);

    // Required visit function
	void visit(const IEntityClassPtr& eclass);

	// VFSTreePopulator::Visitor implementation
	void visit(const Glib::RefPtr<Gtk::TreeStore>& store,
			   const Gtk::TreeModel::iterator& iter,
			   const std::string& path,
			   bool isExplicit);
};

}

#endif /*ENTITYCLASSTREEPOPULATOR_H_*/
