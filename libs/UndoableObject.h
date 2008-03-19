#ifndef UNDOABLE_OBJECT_H_
#define UNDOABLE_OBJECT_H_

#include "iundo.h"

class MapFile;
class UndoObserver;
class UndoMemento;

template<typename Copyable>
class UndoableObject : public Undoable
{
  Copyable& m_object;
  UndoObserver* m_undoQueue;
  MapFile* m_map;

public:
	UndoableObject(Copyable& object);

  void instanceAttach(MapFile* map);
  void instanceDetach(MapFile* map);

  void save();

  UndoMemento* exportState() const;
  void importState(const UndoMemento* state);
};

#endif /* UNDOABLE_OBJECT_H_ */
