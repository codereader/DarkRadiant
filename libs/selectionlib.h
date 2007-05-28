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

#if !defined (INCLUDED_SELECTIONLIB_H)
#define INCLUDED_SELECTIONLIB_H

#include "iselection.h"
#include "selectable.h"
#include "generic/callback.h"
#include "scenelib.h"
#include <stdlib.h>

/** greebo: A structure containing information about the current 
 * Selection. An instance of this is maintained by the 
 * RadiantSelectionSystem, and a const reference can be
 * retrieved via the according getSelectionInfo() method.
 */
class SelectionInfo {
public:
	int totalCount; 	// number of selected items
	int patchCount; 	// number of selected patches
	int brushCount; 	// -- " -- brushes
	int entityCount; 	// -- " -- entities
	
	SelectionInfo() :
		totalCount(0),
		patchCount(0),
		brushCount(0),
		entityCount(0)
	{}
	
	// Zeroes all the counters
	void clear() {
		totalCount = 0;
		patchCount = 0;
		brushCount = 0;
		entityCount = 0;
	}
};

class SelectableBool : public Selectable
{
  bool m_selected;
public:
  SelectableBool()
    : m_selected(false)
  {}

  void setSelected(bool select = true)
  {
    m_selected = select;
  }
  bool isSelected() const
  {
    return m_selected;
  }
  void invertSelected() {
  	m_selected = !m_selected;
  }
};

// greebo: A selectable with an attached callback function that is called upon selection (change)
// in the case of the PatchInstance the callback points to PatchInstance::selectedChangedComponent()
class ObservedSelectable : public Selectable
{
  SelectionChangeCallback m_onchanged;
  bool m_selected;
public:
  ObservedSelectable(const SelectionChangeCallback& onchanged) : m_onchanged(onchanged), m_selected(false)
  {
  }
  ObservedSelectable(const ObservedSelectable& other) : Selectable(other), m_onchanged(other.m_onchanged), m_selected(false)
  {
    setSelected(other.isSelected());
  }
  ObservedSelectable& operator=(const ObservedSelectable& other)
  {
    setSelected(other.isSelected());
    return *this;
  }
  ~ObservedSelectable()
  {
    setSelected(false);
  }

  void setSelected(bool select)
  {
    if(select ^ m_selected)
    {
      m_selected = select;

      m_onchanged(*this);
    }
  }
  bool isSelected() const
  {
    return m_selected;
  }
  void invertSelected() {
  	setSelected(!isSelected());
  }
};

class SelectableInstance : 
	public scene::Instance,
	public Selectable
{
  ObservedSelectable m_selectable;
public:
  SelectableInstance(const scene::Path& path, scene::Instance* parent) :
	  Instance(path, parent),
	  m_selectable(SelectedChangedCaller(*this))
  {}

  Selectable& getSelectable()
  {
    return m_selectable;
  }
  const Selectable& getSelectable() const
  {
    return m_selectable;
  }

  void selectedChanged(const Selectable& selectable)
  {
    GlobalSelectionSystem().onSelectedChanged(*this, selectable);

    Instance::selectedChanged();
  }
  typedef MemberCaller1<SelectableInstance, const Selectable&, &SelectableInstance::selectedChanged> SelectedChangedCaller;
  
	/** greebo: Selectable implementation
	 */
	virtual void setSelected(bool select) {
		m_selectable.setSelected(select);
	}
	
	virtual bool isSelected() const {
		return m_selectable.isSelected();
	}
	
	virtual void invertSelected() {
		m_selectable.invertSelected();
	}
};


template<typename Iterator>
inline bool range_check(Iterator start, Iterator finish, Iterator iter)
{
  for(;start != finish; ++start)
  {
    if(start == iter)
    {
      return true;
    }
  }
  return false;
}

#include <list>

template<typename Selected>
class SelectionList
{
  typedef std::list<Selected*> List;
  List m_selection;
public:
  typedef typename List::iterator iterator;
  typedef typename List::const_iterator const_iterator;

  iterator begin()
  {
    return m_selection.begin();
  }
  const_iterator begin() const
  {
    return m_selection.begin();
  }
  iterator end()
  {
    return m_selection.end();
  }
  const_iterator end() const
  {
    return m_selection.end();
  }
  bool empty() const
  {
    return m_selection.empty();
  }
  std::size_t size() const
  {
    return m_selection.size();
  }
  Selected& back()
  {
    return *m_selection.back();
  }
  Selected& back() const
  {
    return *m_selection.back();
  }
  void append(Selected& selected)
  {
    m_selection.push_back(&selected);
  }
  void erase(Selected& selected)
  {
    typename List::reverse_iterator i = std::find(m_selection.rbegin(), m_selection.rend(), &selected);
    ASSERT_MESSAGE(i != m_selection.rend(), "selection-tracking error");
    ASSERT_MESSAGE(range_check(m_selection.begin(), m_selection.end(), --i.base()), "selection-tracking error");
    m_selection.erase(--i.base());
  }
};

class OccludeSelector : public Selector
{
	SelectionIntersection& _bestIntersection;
	bool& _occluded;
public:
	OccludeSelector(SelectionIntersection& bestIntersection, bool& occluded) : 
		_bestIntersection(bestIntersection), 
		_occluded(occluded) 
	{
		_occluded = false;
	}
	
	void pushSelectable(Selectable& selectable) {}
	void popSelectable() {}
	
	void addIntersection(const SelectionIntersection& intersection) {
		if (SelectionIntersection_closer(intersection, _bestIntersection)) {
			_bestIntersection = intersection;
			_occluded = true;
		}
	}
}; // class OccludeSelector

#endif
