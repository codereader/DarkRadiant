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

    // The change count we're currently at. This represents a position in the map's undo history.
    virtual std::size_t getCurrentChangeCount() const = 0;

    // Marks the current change count as the one that got saved
    virtual void setSavedChangeCount() = 0;

    // Returns true if the current undo history position corresponds to the most recently saved state
    virtual bool isAtSavedPosition() const = 0;

    // Emitted as soon as the current change count is modified
    virtual sigc::signal<void()>& signal_changed() = 0;
};
