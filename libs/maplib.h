/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined (INCLUDED_MAPLIB_H)
#define INCLUDED_MAPLIB_H

#include "mapfile.h"

class UndoFileChangeTracker : 
	public UndoTracker, 
	public MapFile
{
  std::size_t m_size;
  std::size_t m_saved;
  typedef void (UndoFileChangeTracker::*Pending)();
  Pending m_pending;
  Callback m_changed;

public:
  UndoFileChangeTracker() : m_size(0), m_saved(MAPFILE_MAX_CHANGES), m_pending(0)
  {
  }
  void print()
  {
    globalOutputStream() << "saved: " << Unsigned(m_saved) << " size: " << Unsigned(m_size) << "\n";
  }

  void push()
  {
    ++m_size;
    m_changed();
    //print();
  }
  void pop()
  {
    --m_size;
    m_changed();
    //print();
  }
  void pushOperation()
  {
    if(m_size < m_saved)
    {
      // redo queue has been flushed.. it is now impossible to get back to the saved state via undo/redo
      m_saved = MAPFILE_MAX_CHANGES;
    }
    push();
  }
  void clear()
  {
    m_size = 0;
    m_changed();
    //print();
  }
  void begin()
  {
    m_pending = Pending(&UndoFileChangeTracker::pushOperation);
  }
  void undo()
  {
    m_pending = Pending(&UndoFileChangeTracker::pop);
  }
  void redo()
  {
    m_pending = Pending(&UndoFileChangeTracker::push);
  }

  void changed()
  {
    if(m_pending != 0)
    {
      ((*this).*m_pending)();
      m_pending = 0;
    }
  }

  void save()
  {
    m_saved = m_size;
    m_changed();
  }
  bool saved() const
  {
    return m_saved == m_size;
  }

  void setChangedCallback(const Callback& changed)
  {
    m_changed = changed;
    m_changed();
  }

  std::size_t changes() const
  {
    return m_size;
  }
};

#endif
