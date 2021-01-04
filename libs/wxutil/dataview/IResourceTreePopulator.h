#pragma once

#include <memory>
#include <wx/event.h>

namespace wxutil
{

/**
 * TreeView populator interface, used to load data
 * into a TreeModel instance used by ResourceTreeView.
 * 
 * It provides an interface to deal with threaded implementations:
 * - Populate(): will at least start the population, might return
 *               before or after the work is done.
 * - EnsurePopulated(): will ensure that the population has been 
 *                      started and will block until it's done.
 */
class IResourceTreePopulator
{
public:
    using Ptr = std::shared_ptr<IResourceTreePopulator>;

    virtual ~IResourceTreePopulator() {}

    // Define the event handler that is notified once population is done
    virtual void SetFinishedHandler(wxEvtHandler* finishedHandler) = 0;

    // Will start the population and block until population is done.
    virtual void EnsurePopulated() = 0;

    // Start the population process, unless it's already running
    // This might spawn a worker thread which performs the
    // population in the background - use EnsurePopulated to synchronise.
    virtual void Populate() = 0;

    // In threaded implementations this method should cancel the
    // async method and block until it's finished
    virtual void EnsureStopped() = 0;
};

}
