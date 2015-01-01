#pragma once

#include "iundo.h"
#include "mapfile.h"
#include "itextstream.h"
#include <functional>

class UndoFileChangeTracker :
    public UndoSystem::Tracker,
    public IMapFileChangeTracker
{
private:
	std::size_t m_size;
	std::size_t m_saved;
	typedef void (UndoFileChangeTracker::*Pending)();
	Pending m_pending;
	std::function<void()> m_changed;

public:
	UndoFileChangeTracker() :
		m_size(0),
		m_saved(MAPFILE_MAX_CHANGES),
		m_pending(0)
	{}

	void print() {
		rMessage() << "saved: " << m_saved << " size: " << m_size << "\n";
	}

	void push() {
		++m_size;
		m_changed();
		//print();
	}

	void pop() {
		--m_size;
		m_changed();
		//print();
	}

	void pushOperation() {
		if (m_size < m_saved) {
			// redo queue has been flushed.. it is now impossible to get back to the saved state via undo/redo
			m_saved = MAPFILE_MAX_CHANGES;
		}
		push();
	}

	void clear() override {
		m_size = 0;
		m_changed();
		//print();
	}

	void begin() override 
    {
		m_pending = Pending(&UndoFileChangeTracker::pushOperation);
	}

	void undo() override 
    {
		m_pending = Pending(&UndoFileChangeTracker::pop);
	}

	void redo() override 
    {
		m_pending = Pending(&UndoFileChangeTracker::push);
	}

	void changed() {
		if (m_pending != 0) {
			((*this).*m_pending)();
			m_pending = 0;
		}
	}

	void save() {
		m_saved = m_size;
		m_changed();
	}

	bool saved() const {
		return m_saved == m_size;
	}

	void setChangedCallback(const std::function<void()>& changed) {
		m_changed = changed;
		m_changed();
	}

	std::size_t changes() const {
		return m_size;
	}
};
