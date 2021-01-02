#include "ThreadedResourceTreePopulator.h"

namespace wxutil
{

// Custom exception type thrown when a thread is cancelled
struct ThreadAbortedException : public std::runtime_error
{
    ThreadAbortedException() :
        runtime_error("Thread aborted")
    {}
};

// Construct and initialise variables
ThreadedResourceTreePopulator::ThreadedResourceTreePopulator(const TreeModel::ColumnRecord& columns) :
    wxThread(wxTHREAD_JOINABLE),
    _finishedHandler(nullptr),
    _columns(columns),
    _started(false)
{}

ThreadedResourceTreePopulator::~ThreadedResourceTreePopulator()
{
    // When running into crashes with a calling thread waiting for this
    // method, this might be due to the deriving class methods referencing
    // members that have already been destructed at this point.
    // Be sure to call EnsureStopped() in the subclass destructor.
    EnsureStopped();
}

void ThreadedResourceTreePopulator::ThrowIfCancellationRequested()
{
    if (TestDestroy())
    {
        throw ThreadAbortedException();
    }
}

wxThread::ExitCode ThreadedResourceTreePopulator::Entry()
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

void ThreadedResourceTreePopulator::SetFinishedHandler(wxEvtHandler* finishedHandler)
{
    _finishedHandler = finishedHandler;
}

void ThreadedResourceTreePopulator::EnsurePopulated()
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

void ThreadedResourceTreePopulator::Populate()
{
    if (_finishedHandler == nullptr)
    {
        throw std::runtime_error("Cannot start population without a finished handler");
    }

    if (IsRunning())
    {
        return;
    }

    // Set the latch
    _started = true;
    wxThread::Run();
}

void ThreadedResourceTreePopulator::EnsureStopped()
{
    if (IsAlive())
    {
        Delete(); // cancel the running thread
    }
}

}
