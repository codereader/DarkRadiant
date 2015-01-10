#pragma once

#include "iundo.h"
#include "mapfile.h"
#include "itextstream.h"
#include <limits>
#include <functional>

class UndoFileChangeTracker :
    public UndoSystem::Tracker,
    public IMapFileChangeTracker
{
private:
    const std::size_t MAPFILE_MAX_CHANGES;

	std::size_t _size;
	std::size_t _saved;
	typedef void (UndoFileChangeTracker::*Pending)();
	Pending _pending;
	std::function<void()> _changed;

public:
	UndoFileChangeTracker() :
        MAPFILE_MAX_CHANGES(std::numeric_limits<std::size_t>::max()),
		_size(0),
		_saved(MAPFILE_MAX_CHANGES),
		_pending(0)
	{}

	void print() {
		rMessage() << "saved: " << _saved << " size: " << _size << std::endl;
	}

	void push() {
		++_size;
		_changed();
		//print();
	}

	void pop() {
		--_size;
		_changed();
		//print();
	}

	void pushOperation() {
		if (_size < _saved) {
			// redo queue has been flushed.. it is now impossible to get back to the saved state via undo/redo
			_saved = MAPFILE_MAX_CHANGES;
		}
		push();
	}

	void clear() override {
		_size = 0;
		_changed();
		//print();
	}

	void begin() override 
    {
		_pending = Pending(&UndoFileChangeTracker::pushOperation);
	}

	void undo() override 
    {
		_pending = Pending(&UndoFileChangeTracker::pop);
	}

	void redo() override 
    {
		_pending = Pending(&UndoFileChangeTracker::push);
	}

    void changed() override {
		if (_pending != 0) {
			((*this).*_pending)();
			_pending = 0;
		}
	}

    void save() override {
		_saved = _size;
		_changed();
	}

    bool saved() const override {
		return _saved == _size;
	}

    void setChangedCallback(const std::function<void()>& changed) override {
		_changed = changed;
		_changed();
	}

    std::size_t changes() const override {
		return _size;
	}
};
