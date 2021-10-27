#pragma once

#include "iundo.h"
#include "imapfilechangetracker.h"
#include <limits>
#include <sigc++/signal.h>

class UndoFileChangeTracker :
    public IUndoSystem::Tracker,
    public IMapFileChangeTracker
{
private:
    constexpr static std::size_t MAPFILE_MAX_CHANGES = std::numeric_limits<std::size_t>::max();

	std::size_t _currentChangeCount;
	std::size_t _savedChangeCount;
    sigc::signal<void()> _changed;

public:
	UndoFileChangeTracker() :
        _currentChangeCount(0),
        _savedChangeCount(MAPFILE_MAX_CHANGES)
	{}

    void setSavedChangeCount() override
    {
        _savedChangeCount = _currentChangeCount;
        _changed.emit();
    }

    // Returns true if the current undo history position corresponds to the most recently saved state
    bool isAtSavedPosition() const override
    {
        return _savedChangeCount == _currentChangeCount;
    }

    sigc::signal<void()>& signal_changed() override
    {
        return _changed;
    }

    std::size_t getCurrentChangeCount() const override
    {
        return _currentChangeCount;
    }

    void onOperationRecorded(const std::string& operationName) override
    {
        if (_currentChangeCount < _savedChangeCount)
        {
            // redo queue has been flushed.. it is now impossible to get back to the saved state via undo/redo
            _savedChangeCount = MAPFILE_MAX_CHANGES;
        }
        
        ++_currentChangeCount;
        _changed.emit();
    }

    void onOperationUndone(const std::string& operationName) override
    {
        --_currentChangeCount;
        _changed.emit();
    }
    
    void onOperationRedone(const std::string& operationName) override
    {
        ++_currentChangeCount;
        _changed.emit();
    }

    void onAllOperationsCleared() override
    {
        _currentChangeCount = 0;
        _changed.emit();
    }
};
