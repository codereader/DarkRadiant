#include "UndoableObject.h"

#include "undolib.h"

template<>
UndoableObject<TraversableNodeSet>::UndoableObject(TraversableNodeSet& object) : 
	m_object(object), 
	m_undoQueue(0), 
	m_map(0)
{}

template<>
void UndoableObject<TraversableNodeSet>::instanceAttach(MapFile* map) {
    m_map = map;
    m_undoQueue = GlobalUndoSystem().observer(this);
}

template<>
void UndoableObject<TraversableNodeSet>::instanceDetach(MapFile* map) {
    m_map = 0;
    m_undoQueue = 0;
    GlobalUndoSystem().release(this);
}

template<>
void UndoableObject<TraversableNodeSet>::save() {
    if(m_map != 0)
    {
      m_map->changed();
    }
    if(m_undoQueue != 0)
    {
      m_undoQueue->save(this);
    }
}

template<>
UndoMemento* UndoableObject<TraversableNodeSet>::exportState() const {
    return new BasicUndoMemento<TraversableNodeSet>(m_object);
}

template<>
void UndoableObject<TraversableNodeSet>::importState(const UndoMemento* state) {
    save();
    m_object = (static_cast<const BasicUndoMemento<TraversableNodeSet>*>(state))->get();
}
