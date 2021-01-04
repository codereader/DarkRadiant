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
    void ThrowIfCancellationRequested();

    // Needed method to load data into the allocated tree model
    // This is called from within the worker thread
    virtual void PopulateModel(const TreeModel::Ptr& model) = 0;

    // Optional sorting method which is invoked after PopulateModel() is done
    virtual void SortModel(const TreeModel::Ptr& model)
    {}

    // The required entry point for wxWidgets that contains the calls
    // to PopulateModel/SortModel as well as the exception handling
    wxThread::ExitCode Entry() override final;

    // Queues an event to the attached finished handler
    void PostEvent(wxEvent* ev);

public:
    // Construct and initialise variables
    ThreadedResourceTreePopulator(const TreeModel::ColumnRecord& columns);

    virtual ~ThreadedResourceTreePopulator();

    // IResourceTreePopulator implementation

    virtual void SetFinishedHandler(wxEvtHandler* finishedHandler) override;

    // Blocks until the worker thread is done.
    virtual void EnsurePopulated() override;

    // Start the thread, if it isn't already running
    virtual void Populate() override;

    // Blocks and waits until the worker thread is done
    virtual void EnsureStopped() override;
};

}
