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
#pragma once

#include "iselection.h"
#include "iselectable.h"
#include "iselectiontest.h"
#include <stdlib.h>
#include <list>
#include <boost/bind.hpp>
#include "scene/Node.h"
#include "math/AABB.h"

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
	int componentCount;	// -- " -- components (faces, edges, vertices)

	SelectionInfo() :
		totalCount(0),
		patchCount(0),
		brushCount(0),
		entityCount(0),
		componentCount(0)
	{}

	// Zeroes all the counters
	void clear() {
		totalCount = 0;
		patchCount = 0;
		brushCount = 0;
		entityCount = 0;
		componentCount = 0;
	}
};

namespace selection
{

/**
 * The selection "WorkZone" defines the bounds of the most
 * recent selection. On each selection, the workzone is
 * recalculated, nothing happens on deselection.
 */
struct WorkZone
{
	// The corner points defining the selection workzone
	Vector3 min;
	Vector3 max;

	// The bounds of the selection workzone (equivalent to min/max)
	AABB bounds;

	WorkZone() :
		min(-64,-64,-64),
		max(64,64,64),
		bounds(AABB::createFromMinMax(min, max))
	{}
};

} // namespace selection

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

/**
 * \brief
 * Implementation of the Selectable interface which invokes a user-specified
 * callback function when the selection state is changed.
 */
class ObservedSelectable
: public Selectable
{
    // Callback to invoke on selection changed
    SelectionChangedSlot m_onchanged;

    // Current selection state
    bool m_selected;

public:

    /**
     * \brief
     * Construct an ObservedSelectable with the given callback function.
     */
    ObservedSelectable(const SelectionChangedSlot& onchanged)
    : m_onchanged(onchanged), m_selected(false)
    { }

    /**
     * \brief
     * Copy constructor.
     */
    ObservedSelectable(const ObservedSelectable& other)
    : Selectable(other), m_onchanged(other.m_onchanged), m_selected(false)
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

    /**
     * \brief
     * Set the selection state.
     */
    void setSelected(bool select)
    {
        // Change state and invoke callback only if the new state is different
        // from the current state
        if (select ^ m_selected)
        {
            m_selected = select;

			if (m_onchanged)
			{
				m_onchanged(*this);
			}
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
