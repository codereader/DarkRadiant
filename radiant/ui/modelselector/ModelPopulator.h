#pragma once

#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/dataview/ThreadedResourceTreePopulator.h"
#include "iregistry.h"
#include "igame.h"
#include "EventRateLimiter.h"

#include "ifilesystem.h"
#include "ieclass.h"
#include "i18n.h"
#include "string/string.h"
#include "os/path.h"

#include "string/split.h"
#include "string/case_conv.h"
#include <fmt/format.h>

#include "ModelSelector.h"
#include "ModelDataInserter.h"

namespace ui
{

/**
 * Threaded functor object to visit the global VFS and add model paths 
 * to a new TreeModel object. Fires a PopulationFinished event once
 * its work is done.
 */
class ModelPopulator final :
    public wxutil::ThreadedResourceTreePopulator
{
private:
    const ModelTreeView::TreeColumns& _columns;

	// Progress dialog and model count
	std::size_t _count;

    // Event rate limiter for progress dialog
    EventRateLimiter _evLimiter;

	std::set<std::string> _allowedExtensions;

public:

	// Constructor sets the populator
    ModelPopulator(const ModelTreeView::TreeColumns& columns) :
        ThreadedResourceTreePopulator(columns),
        _columns(columns),
		_count(0),
        _evLimiter(50)
	{
		// Load the allowed extensions
		std::string extensions = GlobalGameManager().currentGame()->getKeyValue("modeltypes");
		string::split(_allowedExtensions, extensions, " ");
	}

    ~ModelPopulator()
    {
        EnsureStopped();
    }

protected:
    void PopulateModel(const wxutil::TreeModel::Ptr& model) override
    {
        wxutil::VFSTreePopulator populator(model);
        constexpr const char* MODELS_FOLDER = "models/";

        // Search for model files
        GlobalFileSystem().forEachFile(
            MODELS_FOLDER, "*",
            [&](const vfs::FileInfo& fileInfo)
            {
                // Only add visible models
                if (fileInfo.visibility == vfs::Visibility::NORMAL)
                {
                    visitModelFile(MODELS_FOLDER + fileInfo.name, populator);
                }
            },
            0
        );

        ThrowIfCancellationRequested();

        // Model Defs Folder
        wxutil::Icon folderIcon(wxutil::GetLocalBitmap(FOLDER_ICON));

        reportProgress(_("Building tree..."));

        // Fill in the column data (TRUE = including skins)
        ModelDataInserter inserterSkins(_columns, true);
        populator.forEachNode(inserterSkins);

        reportProgress(_("Adding Model Definitions..."));

        // Model Defs
        auto modelDefName = _("modelDefs");
        wxutil::TreeModel::Row modelDefs(model->AddItem());
        modelDefs[_columns.iconAndName] = wxVariant(wxDataViewIconText(modelDefName, folderIcon));
        modelDefs[_columns.fullName] = modelDefName;
        modelDefs[_columns.modelPath] = "";
        modelDefs[_columns.leafName] = modelDefName;
        modelDefs[_columns.skin] = std::string();
        modelDefs[_columns.isSkin] = false;
        modelDefs[_columns.isFolder] = true;
        modelDefs[_columns.isFavourite] = false;
        modelDefs[_columns.isModelDefFolder] = true;
        modelDefs.SendItemAdded();

        wxutil::VFSTreePopulator modelDefPopulator(model, modelDefs.getItem());

        GlobalEntityClassManager().forEachModelDef([&](const IModelDef::Ptr& def)
        {
            ThrowIfCancellationRequested();
            modelDefPopulator.addPath(def->getDeclName());
        });

        modelDefPopulator.forEachNode(inserterSkins);
    }

    void SortModel(const wxutil::TreeModel::Ptr& model) override
    {
        // Sort the model before returning it
        model->SortModelFoldersFirst(_columns.iconAndName, _columns.isFolder,
            [&](const wxDataViewItem& a, const wxDataViewItem& b)
        {
            // Special folder comparison function
            wxVariant aIsModelDef, bIsModelDef;

            model->GetValue(aIsModelDef, a, _columns.isModelDefFolder.getColumnIndex());
            model->GetValue(bIsModelDef, b, _columns.isModelDefFolder.getColumnIndex());

            // Special treatment for "Other Materials" folder, which always comes last
            if (aIsModelDef)
            {
                return +1;
            }

            if (bIsModelDef)
            {
                return -1;
            }

            return 0; // no special folders, return equal to continue the regular sort algorithm
        });
    }

    void visitModelFile(const std::string& file, wxutil::VFSTreePopulator& populator)
	{
        ThrowIfCancellationRequested();

		std::string ext = os::getExtension(file);
		string::to_lower(ext);

		// Test the extension. If it is not matching any of the known extensions,
		// not interested
		if (_allowedExtensions.find(ext) != _allowedExtensions.end())
		{
			_count++;

			populator.addPath(file);

			if (_evLimiter.readyForEvent())
            {
                reportProgress(fmt::format(_("{0:d} models loaded"), _count));
			}
		}
	}

    void reportProgress(const std::string& message)
    {
        PostEvent(new wxutil::TreeModel::PopulationProgressEvent(fmt::format(_("{0:d} models loaded"), _count)));
    }
};

}
