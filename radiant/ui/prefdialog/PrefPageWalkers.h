#ifndef PREFPAGEWALKERS_H_
#define PREFPAGEWALKERS_H_

#include "PrefDialog.h"
#include "gtkutil/VFSTreePopulator.h"

namespace ui {

	namespace {
		// The treestore enumeration for the preference tree
		enum {
			NAME_COL,		// The column with the caption (for lookups)
			PREFPAGE_COL,	// The pointer to the preference page 
		};
	}

/** greebo: A hybrid walker that is used twice:
 * 			First time to add each PrefPage to the TreeStore using a VFSTreePopulator
 * 			Second time as VFSTreeVisitor to store the data into the treestore	
 */
class PrefTreePopulator :
	public PrefPage::Visitor,
	public gtkutil::VFSTreePopulator::Visitor 
{
	// The helper class creating the GtkTreeIter
	gtkutil::VFSTreePopulator& _vfsPopulator;
	
	PrefDialog& _dialog;
	
public:
	PrefTreePopulator(gtkutil::VFSTreePopulator& vfsPopulator, PrefDialog& dialog) :
		_vfsPopulator(vfsPopulator),
		_dialog(dialog)
	{}

	void visit(PrefPagePtr prefPage) {
		// Check for an empty path (this would be the root item)
		if (!prefPage->getPath().empty()) {
			// Tell the VFSTreePopulator to add the item with this path
			_vfsPopulator.addPath(prefPage->getPath());
		}
	}
	
	void visit(GtkTreeStore* store, GtkTreeIter* iter, 
			   const std::string& path, bool isExplicit)
	{
		// Do not process add the root item
		if (!path.empty()) {
			// Get the leaf name (truncate the path)
			std::string leafName = path.substr(path.rfind("/")+1);
			
			// Get a reference to the page defined by this path
			PrefPagePtr page = _dialog.createOrFindPage(path);
			
			if (page != NULL) {
				// Add the caption to the liststore
				gtk_tree_store_set(store, iter, 
								   NAME_COL, leafName.c_str(),
								   PREFPAGE_COL, page->getWidget(), 
								   -1);
			}
		}
	}
};

/** greebo: A walker searching for a page matching the given path.
 * 			The result is stored in the passed PrefPagePtr&
 */
class PrefPageFinder :
	public PrefPage::Visitor 
{
	// The helper class creating the GtkTreeIter
	PrefPagePtr& _page;
	
	// The path to look up
	std::string _path;
public:
	PrefPageFinder(const std::string& path, PrefPagePtr& page) :
		_page(page),
		_path(path)
	{
		// Initialise the search result to "empty"
		_page = PrefPagePtr();
	}

	void visit(PrefPagePtr prefPage) {
		// Check for a match
		if (prefPage->getPath() == _path) {
			_page = prefPage;
		}
	}
};

} // namespace ui

#endif /*PREFPAGEWALKERS_H_*/
