#pragma once

#include "iuimanager.h"
#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/dataview/TreeViewItemStyle.h"
#include "ifavourites.h"
#include "ModelSelector.h"
#include <wx/artprov.h>

#include "ModelTreeView.h"

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
	public wxutil::VFSTreePopulator::Visitor
{
private:
	const ModelTreeView::TreeColumns& _columns;

	bool _includeSkins;

	wxIcon _modelIcon;
	wxIcon _folderIcon;
	wxIcon _skinIcon;

    std::set<std::string> _favourites;

public:
	/**
	 * greebo: Pass TRUE to the constructor to add skins to the store.
	 */
	ModelDataInserter(const ModelTreeView::TreeColumns& columns, bool includeSkins) :
		_columns(columns),
		_includeSkins(includeSkins)
	{
		_modelIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + MODEL_ICON));
		_folderIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON));
		_skinIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + SKIN_ICON));

        // Get the list of favourites
        _favourites = GlobalFavouritesManager().getFavourites(decl::Type::Model);
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
		std::string fullPath = isExplicit ? (MODELS_FOLDER + path) : "";

        bool isFavourite = isExplicit && _favourites.count(fullPath) > 0;

		// Pixbuf depends on model type
		row[_columns.iconAndName] = wxVariant(wxDataViewIconText(displayName, isExplicit ? _modelIcon : _folderIcon));
        row[_columns.iconAndName] = wxutil::TreeViewItemStyle::Declaration(isFavourite); // assign attributes
		row[_columns.fullName] = fullPath;
		row[_columns.modelPath] = fullPath;
		row[_columns.leafName] = displayName;
		row[_columns.skin] = std::string();
        row[_columns.isSkin] = false;
		row[_columns.isFolder] = !isExplicit;
		row[_columns.isFavourite] = isFavourite;

		if (!_includeSkins) return; // done

		// Now check if there are any skins for this model, and add them as
		// children if so
		const StringList& skinList = GlobalModelSkinCache().getSkinsForModel(fullPath);

		for (const auto& skinName : skinList)
		{
			wxutil::TreeModel::Row skinRow = store.AddItem(row.getItem());

            auto fullSkinPath = fullPath + "/" + skinName;
            isFavourite = isExplicit && _favourites.count(fullSkinPath) > 0;

			skinRow[_columns.iconAndName] = wxVariant(wxDataViewIconText(skinName, _skinIcon));
            skinRow[_columns.iconAndName] = wxutil::TreeViewItemStyle::Declaration(isFavourite); // assign attributes
			skinRow[_columns.fullName] = fullSkinPath; // model path + skin
			skinRow[_columns.modelPath] = fullPath; // this is the model path
			skinRow[_columns.leafName] = skinName;
			skinRow[_columns.skin] = skinName;
            skinRow[_columns.isSkin] = true;
			skinRow[_columns.isFolder] = false;
            skinRow[_columns.isFavourite] = isFavourite;
		}
	}
};

}
