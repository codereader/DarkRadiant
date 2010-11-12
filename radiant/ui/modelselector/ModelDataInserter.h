#ifndef MODELDATAINSERTER_H_
#define MODELDATAINSERTER_H_

#include "iuimanager.h"

namespace ui
{

/* CONSTANTS */
namespace
{
	const char* MODEL_ICON = "model16green.png";
	const char* SKIN_ICON = "skin16.png";
	const char* FOLDER_ICON = "folder16.png";

	const char* MODELS_FOLDER = "models/";
}

/**
 * VFSPopulatorVisitor subclass to fill in column data for the model tree nodes
 * created by the VFS tree populator.
 */
class ModelDataInserter :
	public gtkutil::VFSTreePopulator::Visitor
{
private:
	const ModelSelector::TreeColumns& _columns;

	bool _includeSkins;

public:
	/**
	 * greebo: Pass TRUE to the constructor to add skins to the store.
	 */
	ModelDataInserter(const ModelSelector::TreeColumns& columns, bool includeSkins) :
		_columns(columns),
		_includeSkins(includeSkins)
	{}

	virtual ~ModelDataInserter() {}

	// Required visit function
	void visit(const Glib::RefPtr<Gtk::TreeStore>& store,
			   const Gtk::TreeModel::iterator& iter,
			   const std::string& path,
			   bool isExplicit)
	{
		// Get the display name by stripping off everything before the last
		// slash
		std::string displayName = path.substr(path.rfind("/") + 1);

		// Pathname is the model VFS name for a model, and blank for a folder
		std::string fullPath = isExplicit ? (MODELS_FOLDER + path) : "";

		// Pixbuf depends on model type
		Gtk::TreeModel::Row row = *iter;

		row[_columns.filename] = displayName;
		row[_columns.vfspath] = fullPath;
		row[_columns.icon] = GlobalUIManager().getLocalPixbuf(isExplicit ? MODEL_ICON : FOLDER_ICON);
		row[_columns.skin] = std::string();
		row[_columns.isFolder] = !isExplicit;

		if (!_includeSkins) return; // done

		// Now check if there are any skins for this model, and add them as
		// children if so
		const StringList& skinList = GlobalModelSkinCache().getSkinsForModel(fullPath);

		for (StringList::const_iterator i = skinList.begin();
			 i != skinList.end();
			 ++i)
		{
			Gtk::TreeModel::Row skinRow = *store->append(iter->children());

			skinRow[_columns.filename] = *i;
			skinRow[_columns.vfspath] = fullPath;
			skinRow[_columns.icon] = GlobalUIManager().getLocalPixbuf(SKIN_ICON);
			skinRow[_columns.skin] = *i;
			skinRow[_columns.isFolder] = !isExplicit;
		}
	}
};

}

#endif /*MODELDATAINSERTER_H_*/
