#ifndef MODELDATAINSERTER_H_
#define MODELDATAINSERTER_H_

namespace ui
{

/* CONSTANTS */
namespace {
	const char* MODEL_ICON = "model16green.png";
	const char* SKIN_ICON = "skin16.png";
	const char* FOLDER_ICON = "folder16.png";
}

/**
 * VFSPopulatorVisitor subclass to fill in column data for the model tree nodes
 * created by the VFS tree populator.
 */
class ModelDataInserter : 
	public gtkutil::VFSTreePopulator::Visitor
{
	bool _includeSkins;

public:
	/**
	 * greebo: Pass TRUE to the constructor to add skins to the store.
	 */
	ModelDataInserter(const bool includeSkins) :
		_includeSkins(includeSkins)
	{}

	virtual ~ModelDataInserter() {}

	// Required visit function
	void visit(GtkTreeStore* store, 
			   GtkTreeIter* iter, 
			   const std::string& path,
			   bool isExplicit)
	{
		// Get the display name by stripping off everything before the last
		// slash
		std::string displayName = path.substr(path.rfind("/") + 1);

		// Pathname is the model VFS name for a model, and blank for a folder
		std::string fullPath = isExplicit ? (MODELS_FOLDER + path) : "";
					   
		// Pixbuf depends on model type
		GdkPixbuf* pixBuf = isExplicit 
							? GlobalRadiant().getLocalPixbuf(MODEL_ICON)
							: GlobalRadiant().getLocalPixbuf(FOLDER_ICON);

		// Fill in the column values
		gtk_tree_store_set(store, iter, 
						   NAME_COLUMN, displayName.c_str(),
						   FULLNAME_COLUMN, fullPath.c_str(),
						   SKIN_COLUMN, "",
						   IMAGE_COLUMN, pixBuf,
						   IS_FOLDER_COLUMN, isExplicit ? FALSE : TRUE,
						   -1);
		
		if (!_includeSkins) {
			return; // done
		}

		// Now check if there are any skins for this model, and add them as
		// children if so
		const StringList& skinList = 
			GlobalModelSkinCache().getSkinsForModel(fullPath);
			
		for (StringList::const_iterator i = skinList.begin();
			 i != skinList.end();
			 ++i)
		{
			GtkTreeIter tmpIter;
			gtk_tree_store_append(store, &tmpIter, iter);
			gtk_tree_store_set(store, &tmpIter,
							   NAME_COLUMN, i->c_str(),
							   FULLNAME_COLUMN, fullPath.c_str(),
							   SKIN_COLUMN, i->c_str(),
							   IMAGE_COLUMN, GlobalRadiant().getLocalPixbuf(SKIN_ICON),
							   IS_FOLDER_COLUMN, isExplicit ? FALSE : TRUE,
							   -1);
		}
	} 	
};

}

#endif /*MODELDATAINSERTER_H_*/
