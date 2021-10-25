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

	void connectUndoSystem()
	{
		_undoStateSaver = GlobalUndoSystem().getStateSaver(*this);
	}

    void disconnectUndoSystem()
	{
        _undoStateSaver = nullptr;
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
