#pragma once

#include "iundo.h"
#include "mapfile.h"
#include <limits>
#include <sigc++/signal.h>

class UndoFileChangeTracker :
    public IUndoSystem::Tracker,
    public IMapFileChangeTracker
{
private:
    constexpr static std::size_t MAPFILE_MAX_CHANGES = std::numeric_limits<std::size_t>::max();

	std::size_t _size;
	std::size_t _saved;
    sigc::signal<void()> _changed;

public:
	UndoFileChangeTracker() :
		_size(0),
		_saved(MAPFILE_MAX_CHANGES)
	{}

	void push() 
    {
        ++_size;
        _changed.emit();
	}

	void pop()
    {
        --_size;
        _changed.emit();
	}

	void pushOperation()
    {
		if (_size < _saved)
        {
			// redo queue has been flushed.. it is now impossible to get back to the saved state via undo/redo
			_saved = MAPFILE_MAX_CHANGES;
		}
		push();
	}

	void clear() override
    {
		_size = 0;
        _changed.emit();
	}

    void save() override 
    {
		_saved = _size;
        _changed.emit();
	}

    // Returns true if the current undo history position corresponds to the most recently saved state
    bool saved() const override
    {
		return _saved == _size;
	}

    sigc::signal<void()>& signal_changed() override
    {
        return _changed;
	}

    std::size_t changes() const override
    {
		return _size;
	}

    void onOperationRecorded() override
    {
        pushOperation();
    }

    void onOperationUndone() override
    {
        pop();
    }
    
    void onOperationRedone() override
    {
        push();
    }
};
