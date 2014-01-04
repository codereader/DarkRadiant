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
	public IUndoable
{
	typedef boost::function<void (const Copyable&)> ImportCallback;

	Copyable& _object;
	ImportCallback _importCallback;
	IUndoStateSaver* _undoStateSaver;
	MapFile* _map;
public:
	ObservedUndoable<Copyable>(Copyable& object, const ImportCallback& importCallback) :
		_object(object), 
		_importCallback(importCallback), 
		_undoStateSaver(NULL), 
		_map(NULL)
	{}

	MapFile* map()
	{
		return _map;
	}

	void instanceAttach(MapFile* map)
	{
		_map = map;
		_undoStateSaver = GlobalUndoSystem().getStateSaver(*this);
	}

	void instanceDetach(MapFile* map)
	{
		_map = NULL;
		_undoStateSaver = NULL;
		GlobalUndoSystem().releaseStateSaver(*this);
	}

	void save()
	{
		if (_map != NULL)
		{
			_map->changed();
		}

		if (_undoStateSaver != NULL)
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
