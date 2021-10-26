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
	typedef std::function<void()> RestoreFinishedCallback;

	Copyable& _object;
	ImportCallback _importCallback;
    RestoreFinishedCallback _finishedCallback;
	IUndoStateSaver* _undoStateSaver;

	std::string _debugName;

public:
    ObservedUndoable<Copyable>(Copyable& object, const ImportCallback& importCallback) :
        ObservedUndoable(object, importCallback, RestoreFinishedCallback(), "")
	{}

    ObservedUndoable<Copyable>(Copyable& object, const ImportCallback& importCallback, 
                               const RestoreFinishedCallback& finishedCallback, const std::string& debugName) :
		_object(object),
		_importCallback(importCallback),
        _finishedCallback(finishedCallback),
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

	IUndoMementoPtr exportState() const override
	{
		return IUndoMementoPtr(new BasicUndoMemento<Copyable>(_object));
	}

	void importState(const IUndoMementoPtr& state) override
	{
		save();

		_importCallback(std::static_pointer_cast<BasicUndoMemento<Copyable> >(state)->data());
	}

    void onOperationRestored() override
    {
        if (_finishedCallback)
        {
            _finishedCallback();
        }
    }
};

} // namespace
