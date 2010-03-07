#include "iuimanager.h"
#include "gtkutil/VFSTreePopulator.h"

namespace ui
{

namespace
{
	const char* GUI_ICON = "sr_icon_readable.png";
	const char* FOLDER_ICON = "folder16.png";

	// Treestore enum
	enum {
		NAME_COLUMN,		// e.g. "bla"
		FULLNAME_COLUMN,	// e.g. "guis/readables/bla"
		IMAGE_COLUMN,		// icon to display
		IS_FOLDER_COLUMN,	// whether this is a folder
		N_COLUMNS
	};
}

///////////////////////////// GuiInserter:
// Visitor for the VFSTreePopulator to build the XData-filestructure.
class GuiInserter : 
	public gtkutil::VFSTreePopulator::Visitor
{
public:
	GuiInserter() {}

	virtual ~GuiInserter() {}


	// Required visit function
	void visit(GtkTreeStore* store, 
		GtkTreeIter* iter, 
		const std::string& path,
		bool isExplicit)
	{
		// Get the display name by stripping off everything before the last
		// slash
		std::string displayName = path.substr(path.rfind("/") + 1);
		displayName = displayName.substr(0,displayName.rfind("."));

		// Pixbuf depends on model type
		GdkPixbuf* pixBuf = isExplicit 
			? GlobalUIManager().getLocalPixbuf(GUI_ICON)
			: GlobalUIManager().getLocalPixbuf(FOLDER_ICON);

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
