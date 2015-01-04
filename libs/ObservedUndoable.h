#pragma once

#include "iundo.h"
#include "mapfile.h"
#include <functional>
#include "BasicUndoMemento.h"

namespace undo
{

template<typename Copyable>
class ObservedUndoable : 
	public IUndoable
{
	typedef std::function<void (const Copyable&)> ImportCallback;

	Copyable& _object;
	ImportCallback _importCallback;
	IUndoStateSaver* _undoStateSaver;
    IMapFileChangeTracker* _changeTracker;

public:
	ObservedUndoable<Copyable>(Copyable& object, const ImportCallback& importCallback) :
		_object(object), 
		_importCallback(importCallback), 
        _undoStateSaver(nullptr),
        _changeTracker(nullptr)
	{}

    IMapFileChangeTracker& getUndoChangeTracker()
    {
        return *_changeTracker;
    }

	void connectUndoSystem(IMapFileChangeTracker& changeTracker)
	{
        _changeTracker = &changeTracker;
		_undoStateSaver = GlobalUndoSystem().getStateSaver(*this, changeTracker);
	}

    void disconnectUndoSystem(IMapFileChangeTracker& map)
	{
        _undoStateSaver = nullptr;
        _changeTracker = nullptr;
		GlobalUndoSystem().releaseStateSaver(*this);
	}

	void save()
	{
		if (_undoStateSaver != nullptr)
		{
			_undoStateSaver->save(*this);
		}
	}

	IUndoMementoPtr exportState() const
	{
		return IUndoMementoPtr(new BasicUndoMemento<Copyable>(_object));
	}

	void importState(const IUndoMementoPtr& state)
	{
		save();

		_importCallback(std::static_pointer_cast<BasicUndoMemento<Copyable> >(state)->data());
	}
};

} // namespace
