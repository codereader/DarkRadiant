#pragma once

#include <wx/thread.h>
#include <wx/event.h>
#include "TreeModel.h"

namespace wxutil
{

/**
 * Threaded helper class to load data into a ResourceTreeView.
 * Subclasses need to implement the abstract members to add
 * the needed insertion or sorting logic.
 */
class ResourceTreePopulator :
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
    ResourceTreePopulator(const TreeModel::ColumnRecord& columns, wxEvtHandler* finishedHandler) :
        wxThread(wxTHREAD_JOINABLE),
        _finishedHandler(finishedHandler),
        _columns(columns)
    {}

    ~ResourceTreePopulator()
    {
        if (IsRunning())
        {
            Delete(); // cancel the running thread
        }
    }

    // Blocks until the worker thread is done. 
    // Returns immediately if the thread is not running
    void WaitUntilFinished()
    {
        if (IsRunning())
        {
            Wait();
        }
    }

    // Start the thread, if it isn't already running
    void Run()
    {
        if (IsRunning())
        {
            return;
        }

        wxThread::Run();
    }
};

}
