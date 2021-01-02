#pragma once

#include <wx/thread.h>
#include <wx/event.h>
#include "TreeModel.h"
#include "IResourceTreePopulator.h"

namespace wxutil
{

/**
 * Threaded resource tree populator implementation class.
 * Subclasses need to implement the abstract members to add
 * the needed insertion or sorting logic.
 * 
 * At the end of the thread execution this populator will send
 *  a wxutil::TreeModel::PopulationFinishedEvent to the handler.
 */
class ThreadedResourceTreePopulator :
    public IResourceTreePopulator,
    protected wxThread
{
private:
    // The event handler to notify on completion
    wxEvtHandler* _finishedHandler;

    // Column specification struct, used to construct the new tree model
    const TreeModel::ColumnRecord& _columns;

    // The tree store to populate. We must operate on our own tree store, since
    // updating the target tree store from a different thread isn't safe
    TreeModel::Ptr _treeStore;

    // Whether this thread has been started at all
    bool _started;

protected:

    // Needed method to load data into the allocated tree model
    // This is called from within the worker thread
    virtual void PopulateModel(const TreeModel::Ptr& model) = 0;

    // Optional sorting method which is invoked after PopulateModel() is done
    virtual void SortModel(const TreeModel::Ptr& model)
    {}

    // The worker function that will run in a separate thread
    virtual wxThread::ExitCode Entry() override
    {
        // Create new treestore
        _treeStore = new TreeModel(_columns);
        _treeStore->SetHasDefaultCompare(false);

        PopulateModel(_treeStore);

        if (TestDestroy()) return static_cast<ExitCode>(0);

        // Sort the model while we're still in the worker thread
        SortModel(_treeStore);

        if (!TestDestroy())
        {
            wxQueueEvent(_finishedHandler, new TreeModel::PopulationFinishedEvent(_treeStore));
        }

        return static_cast<ExitCode>(0);
    }

public:
    // Construct and initialise variables
    ThreadedResourceTreePopulator(const TreeModel::ColumnRecord& columns, wxEvtHandler* finishedHandler) :
        wxThread(wxTHREAD_JOINABLE),
        _finishedHandler(finishedHandler),
        _columns(columns),
        _started(false)
    {}

    virtual ~ThreadedResourceTreePopulator()
    {
        if (IsAlive())
        {
            Delete(); // cancel the running thread
        }
    }

    // Blocks until the worker thread is done. 
    void EnsurePopulated() override
    {
        // Start the thread now if we have to
        if (!_started)
        {
            Populate();
        }

        // Wait for any running thread
        if (IsRunning())
        {
            Wait();
        }
    }

    // Start the thread, if it isn't already running
    void Populate() override
    {
        if (IsRunning())
        {
            return;
        }

        // Set the latch
        _started = true;
        wxThread::Run();
    }
};

}
