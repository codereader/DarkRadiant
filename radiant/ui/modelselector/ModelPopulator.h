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

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/format.hpp>

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
		boost::algorithm::split(_allowedExtensions, extensions, boost::algorithm::is_any_of(" "));
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
        // Search for model files
        GlobalFileSystem().forEachFile(MODELS_FOLDER,
            "*", 
            [&](const std::string& filename) { visitModelFile(filename); },
            0);

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

    void visitModelFile(const std::string& file)
	{
        if (TestDestroy())
        {
            return;
        }

		std::string ext = os::getExtension(file);
		boost::algorithm::to_lower(ext);

		// Test the extension. If it is not matching any of the known extensions,
		// not interested
		if (_allowedExtensions.find(ext) != _allowedExtensions.end())
		{
			_count++;

			_populator.addPath(file);

			if (_evLimiter.readyForEvent())
            {
                reportProgress((boost::format(_("%d models loaded")) % _count).str());
			}
		}
	}

    void reportProgress(const std::string& message)
    {
        wxQueueEvent(_finishedHandler, new wxutil::TreeModel::PopulationProgressEvent(
            (boost::format(_("%d models loaded")) % _count).str()));
    }
};

}
