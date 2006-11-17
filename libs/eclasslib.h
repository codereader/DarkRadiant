/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

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

#if !defined (INCLUDED_ECLASSLIB_H)
#define INCLUDED_ECLASSLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <map>
#include <vector>

#include "ieclass.h"
#include "irender.h"

#include "string/string.h"

typedef Vector3 Colour3;

class ListAttributeType
{
  typedef std::pair<CopiedString, CopiedString> ListItem;
  typedef std::vector<ListItem> ListItems;
  ListItems m_items;
public:

  typedef ListItems::const_iterator const_iterator;
  const_iterator begin() const
  {
    return m_items.begin();
  }
  const_iterator end() const
  {
    return m_items.end();
  }

  const ListItem& operator[](std::size_t i) const
  {
    return m_items[i];
  }
  const_iterator findValue(const char* value) const
  {
    for(ListItems::const_iterator i = m_items.begin(); i != m_items.end(); ++i)
    {
      if(string_equal(value, (*i).second.c_str()))
      {
        return i;
      }
    }
    return m_items.end();
  }

  void push_back(const char* name, const char* value)
  {
    m_items.push_back(ListItems::value_type(name, value));
  }
};

class EntityClassAttribute
{
public:
  CopiedString m_type;
  CopiedString m_name;
  CopiedString m_value;
  CopiedString m_description;
  EntityClassAttribute()
  {
  }
  EntityClassAttribute(const char* type, const char* name, const char* value = "", const char* description = "") : m_type(type), m_name(name), m_value(value), m_description(description)
  {
  }
};

typedef std::pair<CopiedString, EntityClassAttribute> EntityClassAttributePair;
typedef std::list<EntityClassAttributePair> EntityClassAttributes;
typedef std::list<CopiedString> StringList;

inline const char* EntityClassAttributePair_getName(const EntityClassAttributePair& attributePair)
{
  if(!string_empty(attributePair.second.m_name.c_str()))
  {
    return attributePair.second.m_name.c_str();
  }
  return attributePair.first.c_str();
}

inline const char* EntityClassAttributePair_getDescription(const EntityClassAttributePair& attributePair)
{
  if(!string_empty(attributePair.second.m_description.c_str()))
  {
    return attributePair.second.m_description.c_str();
  }
  return EntityClassAttributePair_getName(attributePair);
}

/** Represents a particular Entity type. The entity creation code creates a
 * specific instance of an EntityClass.
 */

class EntityClass
{
    
    // Should this entity type be treated as a light?
    bool _isLight;
    
	// Colour of this entity
	Colour3	_colour;
	
public:

	CopiedString m_name;
  StringList m_parent;
	bool	fixedsize;
	bool	unknown;		// wasn't found in source
	Vector3	mins;
	Vector3 maxs;

	// Shader versions of the colour
	Shader* _fillShader;
	Shader* _wireShader;

	CopiedString m_comments;
	char	flagnames[MAX_FLAGS][32];

  CopiedString m_modelpath;
  CopiedString m_skin;

  EntityClassAttributes m_attributes;

  bool inheritanceResolved;
  bool sizeSpecified;
  bool colorSpecified;

private:

	// Capture the shaders corresponding to the current colour
	void captureColour() {
		// Capture fill and wire versions of the entity colour
		std::string fillCol = (boost::format("(%g %g %g)") % _colour[0] % _colour[1] % _colour[2]).str();
		std::string wireCol = (boost::format("<%g %g %g>") % _colour[0] % _colour[1] % _colour[2]).str();

		_fillShader = GlobalShaderCache().capture(fillCol);
		_wireShader = GlobalShaderCache().capture(wireCol);
	}

	// Release the shaders associated with the current colour
	void releaseColour() {
		// Release fill and wire versions of the entity colour
		std::string fillCol = (boost::format("(%g %g %g)") % _colour[0] % _colour[1] % _colour[2]).str();
		std::string wireCol = (boost::format("<%g %g %g>") % _colour[0] % _colour[1] % _colour[2]).str();

		GlobalShaderCache().release(fillCol);
		GlobalShaderCache().release(wireCol);
	}

public:

    /** Default constructor.
     * 
     * @param colour
     * Display colour for this entity
     */
    EntityClass(const Colour3& colour)
    : _isLight(false),
	  _colour(colour),
 	  fixedsize(false),
	  unknown(false),
	  mins(1, 1, 1),
	  maxs(-1,-1,-1),
	  inheritanceResolved(true),
	  sizeSpecified(false),
	  colorSpecified(false)
    {
		memset(flagnames, 0, MAX_FLAGS*32);

		// Capture the shaders
		captureColour();		
    }

    /** Destructor.
     */
	~EntityClass() {
		// Release the shaders
		releaseColour();
	}		
    
    /** Get whether this entity type is a light entity
     * 
     * @returns
     * true if this is a light, false otherwise
     */
    bool isLight() const {
        return _isLight;
    }
    
    /** Set whether this entity type is a light entity
     * 
     * @param val
     * true to set this as a light entity, false to disable
     */
    void isLight(bool val) {
        _isLight = val;
    }

	/** Set the display colour for this entity.
	 * 
	 * @param colour
	 * The new colour to use.
	 */
	void setColour(const Vector3& colour) {
		// Release the current shaders, then capture the new ones
		releaseColour();
		_colour = colour;
		captureColour();
	}
     
	/** Get this entity's colour.
	 * 
	 * @returns
	 * A Vector3 containing the current colour.
	 */
	const Vector3& getColour() const {
		return _colour;
	}

	/** Return this entity's wireframe shader.
	 */
	Shader* getWireShader() const {
		return _wireShader;
	}

	/** Return this entity's fill shader.
	 */
	Shader* getFillShader() const {
		return _fillShader;
	}

  const char* name() const
  {
    return m_name.c_str();
  }
  const char* comments() const
  {
    return m_comments.c_str();
  }
  const char* modelpath() const
  {
    return m_modelpath.c_str();
  }
  const char* skin() const
  {
    return m_skin.c_str();
  }
};

inline const char* EntityClass_valueForKey(const EntityClass& entityClass, const char* key)
{
  for(EntityClassAttributes::const_iterator i = entityClass.m_attributes.begin(); i != entityClass.m_attributes.end(); ++i)
  {
    if(string_equal(key, (*i).first.c_str()))
    {
      return (*i).second.m_value.c_str();
    }
  }
  return "";
}

inline EntityClassAttributePair& EntityClass_insertAttribute(EntityClass& entityClass, const char* key, const EntityClassAttribute& attribute = EntityClassAttribute())
{
  entityClass.m_attributes.push_back(EntityClassAttributePair(key, attribute));
  return entityClass.m_attributes.back();
}


inline bool classname_equal(const char* classname, const char* other)
{
  return string_equal(classname, other);
}

/** Create a new EntityClass.
 */

inline EntityClass* EClass_Create(const char* name, const Vector3& colour, const char* comments)
{
	EntityClass *e = new EntityClass(colour);

	e->m_name = name;

	if (comments)
		e->m_comments = comments;

	return e;
}

inline EntityClass* EClass_Create_FixedSize(const char* name, const Vector3& colour, const Vector3& mins, const Vector3& maxs, const char* comments)
{
	EntityClass *e = new EntityClass(colour);

	e->m_name = name;

	e->fixedsize = true;
	e->mins = mins;
	e->maxs = maxs;

	if (comments)
		e->m_comments = comments;

	return e;
}

const Vector3 smallbox[2] = {
  Vector3(-8,-8,-8),
  Vector3( 8, 8, 8),
};

inline EntityClass *EntityClass_Create_Default(const char *name, bool has_brushes)
{
	// create a new class for it
	if (has_brushes)
	{
    return EClass_Create(name, Vector3(0.0f, 0.5f, 0.0f), "Not found in source.");
	}
	else
	{
    return EClass_Create_FixedSize(name, Vector3(0.0f, 0.5f, 0.0f), smallbox[0], smallbox[1], "Not found in source.");
	}
}

#endif
