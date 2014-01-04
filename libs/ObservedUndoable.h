#pragma once

#include "iundo.h"
#include "mapfile.h"
#include "warnings.h"
#include <boost/function.hpp>
#include "BasicUndoMemento.h"

namespace undo
{

template<typename Copyable>
class ObservedUndoable : 
	public Undoable
{
	typedef boost::function<void (const Copyable&)> ImportCallback;

	Copyable& _object;
	ImportCallback _importCallback;
	UndoObserver* _undoQueue;
	MapFile* _map;
public:
	ObservedUndoable<Copyable>(Copyable& object, const ImportCallback& importCallback) :
		_object(object), 
		_importCallback(importCallback), 
		_undoQueue(NULL), 
		_map(NULL)
	{}

	MapFile* map()
	{
		return _map;
	}

	void instanceAttach(MapFile* map)
	{
		_map = map;
		_undoQueue = GlobalUndoSystem().observer(this);
	}

	void instanceDetach(MapFile* map)
	{
		_map = NULL;
		_undoQueue = NULL;
		GlobalUndoSystem().release(this);
	}

	void save()
	{
		if (_map != NULL)
		{
			_map->changed();
		}

		if (_undoQueue != NULL)
		{
			_undoQueue->save(this);
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
