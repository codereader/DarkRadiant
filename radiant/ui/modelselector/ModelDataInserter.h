#pragma once

#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/dataview/TreeViewItemStyle.h"
#include "ifavourites.h"
#include "ModelSelector.h"
#include "wxutil/Bitmap.h"
#include "wxutil/Icon.h"

#include "ModelTreeView.h"

namespace ui
{

/* CONSTANTS */
namespace
{
	constexpr const char* MODEL_ICON = "model16green.png";
	constexpr const char* SKIN_ICON = "skin16.png";
	constexpr const char* FOLDER_ICON = "folder16.png";
}

/**
 * VFSPopulatorVisitor subclass to fill in column data for the model tree nodes
 * created by the VFS tree populator.
 */
class ModelDataInserter :
	public wxutil::VFSTreePopulator::Visitor
{
private:
	const ModelTreeView::TreeColumns& _columns;

	bool _includeSkins;

    wxutil::Icon _modelIcon;
    wxutil::Icon _folderIcon;
    wxutil::Icon _skinIcon;

    std::set<std::string> _favourites;

public:
	/**
	 * greebo: Pass TRUE to the constructor to add skins to the store.
	 */
	ModelDataInserter(const ModelTreeView::TreeColumns& columns, bool includeSkins) :
		_columns(columns),
		_includeSkins(includeSkins),
        _modelIcon(wxutil::GetLocalBitmap(MODEL_ICON)),
        _folderIcon(wxutil::GetLocalBitmap(FOLDER_ICON)),
        _skinIcon(wxutil::GetLocalBitmap(SKIN_ICON))
	{
        // Get the list of favourites
        _favourites = GlobalFavouritesManager().getFavourites("model");
	}

	virtual ~ModelDataInserter() {}

	// Required visit function
	void visit(wxutil::TreeModel& store,
				wxutil::TreeModel::Row& row,
				const std::string& path,
				bool isExplicit)
	{
		// Get the display name by stripping off everything before the last
		// slash
		std::string displayName = path.substr(path.rfind("/") + 1);

		// Pathname is the model VFS name for a model, and blank for a folder
		std::string fullPath = isExplicit ? path : "";

        bool isFavourite = isExplicit && _favourites.count(fullPath) > 0;

		// Pixbuf depends on model type
		row[_columns.iconAndName] = wxVariant(wxDataViewIconText(displayName, isExplicit ? _modelIcon : _folderIcon));
        row[_columns.iconAndName].setAttr(wxutil::TreeViewItemStyle::Declaration(isFavourite)); // assign attributes
		row[_columns.fullName] = fullPath;
		row[_columns.modelPath] = fullPath;
		row[_columns.leafName] = displayName;
		row[_columns.skin] = std::string();
        row[_columns.isSkin] = false;
		row[_columns.isFolder] = !isExplicit;
		row[_columns.isFavourite] = isFavourite;
        row[_columns.isModelDefFolder] = false;

        // Don't search skins for folders (or if switched off by preference)
		if (!_includeSkins || !isExplicit) return; // done

		// Now check if there are any skins for this model, and add them as
		// children if so
		const StringList& skinList = GlobalModelSkinCache().getSkinsForModel(fullPath);

		for (const auto& skinName : skinList)
		{
			wxutil::TreeModel::Row skinRow = store.AddItem(row.getItem());

            auto fullSkinPath = fullPath + "/" + skinName;
            isFavourite = isExplicit && _favourites.count(fullSkinPath) > 0;

			skinRow[_columns.iconAndName] = wxVariant(wxDataViewIconText(skinName, _skinIcon));
            skinRow[_columns.iconAndName].setAttr(wxutil::TreeViewItemStyle::Declaration(isFavourite)); // assign attributes
			skinRow[_columns.fullName] = fullSkinPath; // model path + skin
			skinRow[_columns.modelPath] = fullPath; // this is the model path
			skinRow[_columns.leafName] = skinName;
			skinRow[_columns.skin] = skinName;
            skinRow[_columns.isSkin] = true;
			skinRow[_columns.isFolder] = false;
            skinRow[_columns.isFavourite] = isFavourite;
            skinRow[_columns.isModelDefFolder] = false;
		}
	}
};

}
