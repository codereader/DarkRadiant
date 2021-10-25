#pragma once

#include "inode.h"
#include <sigc++/signal.h>

/**
 * The file change tracker monitors the changes made to a single map with the help
 * of the root node's undo system. It fires the given callback function whenever 
 * the change count is increased or decreased (this is happening on change, undo and redo).
 * It also provides methods for the client code to check whether the current point 
 * in the map's undo history corresponds to a saved state or not - this allows to keep 
 * the map's modified flag up to date.
 */
class IMapFileChangeTracker
{
public:
    virtual ~IMapFileChangeTracker() {}

    virtual void save() = 0;
    virtual bool saved() const = 0;
    virtual sigc::signal<void()>& signal_changed() = 0;
    virtual std::size_t changes() const = 0;
};
