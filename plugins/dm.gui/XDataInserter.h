#ifndef _XDATA_INSERTER_H_
#define _XDATA_INSERTER_H_

#include "iuimanager.h"

namespace ui
{
	
namespace
{
	const char* XDATA_ICON = "sr_icon_readable.png";
	const char* FOLDER_ICON = "folder16.png";

	// Treestore enum
	enum {
		NAME_COLUMN,		// e.g. "bla"
		FULLNAME_COLUMN,	// e.g. "xdata/readables/bla"
		IMAGE_COLUMN,		// icon to display
		IS_FOLDER_COLUMN,	// whether this is a folder
		N_COLUMNS
	};
}

///////////////////////////// XDataInserter:
// Visitor for the VFSTreePopulator to build the XData-filestructure.
class XDataInserter : 
	public gtkutil::VFSTreePopulator::Visitor
{
public:
	XDataInserter() {}

	virtual ~XDataInserter() {}


	// Required visit function
	void visit(GtkTreeStore* store, 
		GtkTreeIter* iter, 
		const std::string& path,
		bool isExplicit)
	{
		// Get the display name by stripping off everything before the last
		// slash
		std::string displayName = path.substr(path.rfind("/") + 1);

		// Pixbuf depends on model type
		GdkPixbuf* pixBuf = isExplicit 
			? GlobalUIManager().getLocalPixbuf(XDATA_ICON)->gobj()
			: GlobalUIManager().getLocalPixbuf(FOLDER_ICON)->gobj();

		// Fill in the column values
		gtk_tree_store_set(store, iter, 
			NAME_COLUMN, displayName.c_str(),
			FULLNAME_COLUMN, path.c_str(),
			IMAGE_COLUMN, pixBuf,
			IS_FOLDER_COLUMN, isExplicit ? FALSE : TRUE,
			-1);
	} 	
};

} // namespace ui

#endif
