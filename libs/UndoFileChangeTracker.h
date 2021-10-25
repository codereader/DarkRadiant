#pragma once

#include "iundo.h"
#include "mapfile.h"
#include <limits>
#include <functional>

class UndoFileChangeTracker :
    public IUndoSystem::Tracker,
    public IMapFileChangeTracker
{
private:
    constexpr static std::size_t MAPFILE_MAX_CHANGES = std::numeric_limits<std::size_t>::max();

	std::size_t _size;
	std::size_t _saved;
	std::function<void()> _changed;

public:
	UndoFileChangeTracker() :
		_size(0),
		_saved(MAPFILE_MAX_CHANGES)
	{}

	void push() 
    {
        ++_size;
        if (_changed)
        {
            _changed();
        }
	}

	void pop()
    {
        --_size;
        if (_changed)
        {
            _changed();
        }
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

        if (_changed)
        {
            _changed();
        }
	}

    void save() override 
    {
		_saved = _size;
        if (_changed)
        {
            _changed();
        }
	}

    // Returns true if the current undo history position corresponds to the most recently saved state
    bool saved() const override
    {
		return _saved == _size;
	}

    void setChangedCallback(const std::function<void()>& changed) override
    {
		_changed = changed;

        if (_changed)
        {
            _changed();
        }
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
