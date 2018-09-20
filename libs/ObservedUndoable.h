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

	std::string _debugName;

public:
	ObservedUndoable<Copyable>(Copyable& object, const ImportCallback& importCallback) :
		_object(object), 
		_importCallback(importCallback), 
        _undoStateSaver(nullptr),
        _changeTracker(nullptr)
	{}

	ObservedUndoable<Copyable>(Copyable& object, const ImportCallback& importCallback, const std::string& debugName) :
		_object(object),
		_importCallback(importCallback),
		_undoStateSaver(nullptr),
		_changeTracker(nullptr),
		_debugName(debugName)
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
			//rMessage() << "Saving undoable's state of observed object '" << _debugName << "'\n";

			_undoStateSaver->save(*this);
		}
	}

	IUndoMementoPtr exportState() const
	{
		//rMessage() << "Exporting state of observed object '" << _debugName << "'\n";

		return IUndoMementoPtr(new BasicUndoMemento<Copyable>(_object));
	}

	void importState(const IUndoMementoPtr& state)
	{
		save();

		//rMessage() << "Importing state of observed object '" << _debugName << "'\n";

		_importCallback(std::static_pointer_cast<BasicUndoMemento<Copyable> >(state)->data());
	}
};

} // namespace
