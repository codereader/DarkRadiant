#pragma once

#include "wxutil/VFSTreePopulator.h"
#include "wxutil/ModalProgressDialog.h"
#include "imainframe.h"
#include "iregistry.h"
#include "igame.h"
#include "EventRateLimiter.h"

#include "ifilesystem.h"
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
class ModelPopulator :
    public wxThread
{
    const ModelSelector::TreeColumns& _columns;

    // The working copy to populate
    wxutil::TreeModel::Ptr _treeStore;

	// VFSTreePopulator to populate
	wxutil::VFSTreePopulator _populator;

	// Progress dialog and model count
	std::size_t _count;

    // Event rate limiter for progress dialog
    EventRateLimiter _evLimiter;

	std::set<std::string> _allowedExtensions;

    // The event handler to notify on completion
    wxEvtHandler* _finishedHandler;

    class ThreadAbortedException : public std::exception
    {};

public:

	// Constructor sets the populator
    ModelPopulator(const ModelSelector::TreeColumns& columns, 
                   wxEvtHandler* finishedHandler) :
        wxThread(wxTHREAD_JOINABLE),
        _columns(columns),
        _treeStore(new wxutil::TreeModel(_columns)),
		_populator(_treeStore),
		//_progress(_("Loading models")),
		_count(0),
		_evLimiter(50),
        _finishedHandler(finishedHandler)
	{
		//_progress.setText(_("Searching"));

		// Load the allowed extensions
		std::string extensions = GlobalGameManager().currentGame()->getKeyValue("modeltypes");
		string::split(_allowedExtensions, extensions, " ");
	}

    ~ModelPopulator()
    {
        // We might have a running thread, wait for it
        if (IsRunning())
        {
            Delete();
        }
    }

    // Thread entry point
    ExitCode Entry()
    {
        try
        {
            // Search for model files
            GlobalFileSystem().forEachFile(
                MODELS_FOLDER, "*",
                [&](const vfs::FileInfo& fileInfo)
                {
                    // Only add visible models
                    if (fileInfo.visibility == vfs::Visibility::NORMAL)
                        visitModelFile(fileInfo.name);
                },
                0
            );

            if (TestDestroy()) return static_cast<wxThread::ExitCode>(0);

            reportProgress(_("Building tree..."));

            // Fill in the column data (TRUE = including skins)
            ModelDataInserter inserterSkins(_columns, true);
            _populator.forEachNode(inserterSkins);

            if (TestDestroy()) return static_cast<wxThread::ExitCode>(0);

            // Sort the model before returning it
            _treeStore->SortModelFoldersFirst(_columns.filename, _columns.isFolder);

            if (!TestDestroy())
            {
                // Send the event to our listener, only if we are not forced to finish
                wxQueueEvent(_finishedHandler, new wxutil::TreeModel::PopulationFinishedEvent(_treeStore));
            }

            return static_cast<wxThread::ExitCode>(0);
        }
        catch (ThreadAbortedException)
        {
            return static_cast<wxThread::ExitCode>(0);
        }
    }

    void visitModelFile(const std::string& file)
	{
        if (TestDestroy())
        {
            throw ThreadAbortedException();
        }

		std::string ext = os::getExtension(file);
		string::to_lower(ext);

		// Test the extension. If it is not matching any of the known extensions,
		// not interested
		if (_allowedExtensions.find(ext) != _allowedExtensions.end())
		{
			_count++;

			_populator.addPath(file);

			if (_evLimiter.readyForEvent())
            {
                reportProgress(fmt::format(_("{0:d} models loaded"), _count));
			}
		}
	}

    void reportProgress(const std::string& message)
    {
        wxQueueEvent(_finishedHandler, new wxutil::TreeModel::PopulationProgressEvent(
            fmt::format(_("{0:d} models loaded"), _count)));
    }
};

}
