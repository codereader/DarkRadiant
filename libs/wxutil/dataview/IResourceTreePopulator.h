#pragma once

#include "ResourceTreeView.h"

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

    // Will start the population and block until population is done.
    virtual void EnsurePopulated() = 0;

    // Start the population process, unless it's already running
    // This might spawn a worker thread which performs the
    // population in the background - use EnsurePopulated to synchronise.
    virtual void Populate() = 0;
};

}
