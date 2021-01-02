#pragma once

#include <stdexcept>
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
 * 
 * Note: if a subclass is introducing additional class members
 * that are used in the PopulateModel/SortModel methods, it's mandatory
 * for the subclass destructor to call EnsureStopped().
 */
class ThreadedResourceTreePopulator :
    public IResourceTreePopulator,
    protected wxThread
{
private:
    // Custom exception type thrown when a thread is cancelled
    struct ThreadAbortedException : public std::runtime_error
    {
        ThreadAbortedException() : 
            runtime_error("Thread aborted")
        {}
    };

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
    // Wrapper around TestDestroy that escalated by throwing a 
    // ThreadAbortedException when cancellation has been requested
    void ThrowIfCancellationRequested()
    {
        if (TestDestroy())
        {
            throw ThreadAbortedException();
        }
    }

    // Needed method to load data into the allocated tree model
    // This is called from within the worker thread
    virtual void PopulateModel(const TreeModel::Ptr& model) = 0;

    // Optional sorting method which is invoked after PopulateModel() is done
    virtual void SortModel(const TreeModel::Ptr& model)
    {}

    // The worker function that will run in a separate thread
    virtual wxThread::ExitCode Entry() override
    {
        try
        {
            // Create new treestore
            _treeStore = new TreeModel(_columns);
            _treeStore->SetHasDefaultCompare(false);

            PopulateModel(_treeStore);

            ThrowIfCancellationRequested();

            // Sort the model while we're still in the worker thread
            SortModel(_treeStore);

            ThrowIfCancellationRequested();

            wxQueueEvent(_finishedHandler, new TreeModel::PopulationFinishedEvent(_treeStore));
        }
        catch (const ThreadAbortedException&)
        {
            // Thread aborted due to Cancel request, exit now
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
        // When running into crashes with a calling thread waiting for this
        // method, this might be due to the deriving class methods referencing
        // members that have already been destructed at this point.
        // Be sure to call EnsureStopped() in the subclass destructor.
        EnsureStopped();
    }

    // Blocks until the worker thread is done. 
    virtual void EnsurePopulated() override
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
    virtual void Populate() override
    {
        if (IsRunning())
        {
            return;
        }

        // Set the latch
        _started = true;
        wxThread::Run();
    }

    virtual void EnsureStopped() override
    {
        if (IsAlive())
        {
            Delete(); // cancel the running thread
        }
    }
};

}
