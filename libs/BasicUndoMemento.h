#pragma once

#include "iundo.h"

namespace undo
{

/**
 * An UndoMemento implementation capable of holding a single
 * copyable object, which is stored by value.
 */
template<typename Copyable>
class BasicUndoMemento : 
	public IUndoMemento
{
	Copyable _data;
public:
	BasicUndoMemento(const Copyable& data) : 
		_data(data)
	{}

	const Copyable& data() const
	{
		return _data;
	}
};

} // namespace
