#pragma once

#include "iundo.h"
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

	std::string _debugName;

public:
	ObservedUndoable<Copyable>(Copyable& object, const ImportCallback& importCallback) :
		_object(object), 
		_importCallback(importCallback), 
        _undoStateSaver(nullptr)
	{}

	ObservedUndoable<Copyable>(Copyable& object, const ImportCallback& importCallback, const std::string& debugName) :
		_object(object),
		_importCallback(importCallback),
		_undoStateSaver(nullptr),
		_debugName(debugName)
	{}

	void connectUndoSystem(IUndoSystem& undoSystem)
	{
		_undoStateSaver = undoSystem.getStateSaver(*this);
	}

    void disconnectUndoSystem(IUndoSystem& undoSystem)
	{
        _undoStateSaver = nullptr;
		undoSystem.releaseStateSaver(*this);
	}

    // Returns true if this Undoable is connected to an UndoSystem
    bool isConnected() const
    {
        return _undoStateSaver != nullptr;
    }

    IUndoSystem& getUndoSystem()
    {
        if (!_undoStateSaver) throw std::logic_error("ObservedUndoable node connected to any UndoSystem");

        return _undoStateSaver->getUndoSystem();
    }

	void save()
	{
		if (_undoStateSaver != nullptr)
		{
			_undoStateSaver->saveState();
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
