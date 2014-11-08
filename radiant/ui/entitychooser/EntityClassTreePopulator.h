#pragma once

#include "ieclass.h"

#include <map>
#include <string>
#include "wxutil/VFSTreePopulator.h"
#include "EntityClassChooser.h"

namespace ui
{

/**
 * EntityClassVisitor which populates a Gtk::TreeStore with entity classnames
 * taking account of display folders and mod names.
 */
class EntityClassTreePopulator :
	public wxutil::VFSTreePopulator,
	public wxutil::VFSTreePopulator::Visitor,
	public EntityClassVisitor
{
private:
    // TreeStore to populate
    wxutil::TreeModel::Ptr _store;

	// Column definition
	const EntityClassChooser::TreeColumns& _columns;

    // Key that specifies the display folder
    std::string _folderKey;

	wxIcon _folderIcon;
	wxIcon _entityIcon;

public:
    // Constructor
	EntityClassTreePopulator(wxutil::TreeModel::Ptr store,
							 const EntityClassChooser::TreeColumns& columns);

    // Required visit function
	void visit(const IEntityClassPtr& eclass);

	// VFSTreePopulator::Visitor implementation
	void visit(wxutil::TreeModel& store,
				wxutil::TreeModel::Row& row,
				const std::string& path,
				bool isExplicit);
};

}
