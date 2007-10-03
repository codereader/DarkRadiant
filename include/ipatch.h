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

#if !defined(INCLUDED_IPATCH_H)
#define INCLUDED_IPATCH_H

#include "imodule.h"

#include "inode.h"
#include "math/Vector2.h"
#include "math/Vector3.h"

template<typename Element>
class ArrayReference
{
  std::size_t m_size;
  Element* m_data;
public:
  typedef Element value_type;
  typedef value_type* iterator;
  typedef const value_type* const_iterator;

  ArrayReference()
    : m_size(0), m_data(0)
  {
  }
  ArrayReference(std::size_t size, Element* data)
    : m_size(size), m_data(data)
  {
  }

  iterator begin()
  {
    return m_data;
  }
  const_iterator begin() const
  {
    return m_data;
  }
  iterator end()
  {
    return m_data + m_size;
  }
  const_iterator end() const
  {
    return m_data + m_size;
  }

  value_type& operator[](std::size_t index)
  {
#if defined(_DEBUG)
    ASSERT_MESSAGE(index < size(), "array index out of bounds");
#endif
    return m_data[index];
  }
  const value_type& operator[](std::size_t index) const
  {
#if defined(_DEBUG)
    ASSERT_MESSAGE(index < size(), "array index out of bounds");
#endif
    return m_data[index];
  }
  value_type* data()
  {
    return m_data;
  }
  const value_type* data() const
  {
    return m_data;
  }
  std::size_t size() const
  {
    return m_size;
  }
  bool empty() const
  {
    return m_size == 0;
  }
};

#if 0
template<typename Element>
class MatrixIterator
{
  Element* m_position;

  void increment()
  {
    ++m_position;
  }

public:
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef std::ptrdiff_t difference_type;
  typedef difference_type distance_type;
  typedef KeyValue<Key, Value> value_type;
  typedef value_type* pointer;
  typedef value_type& reference;

  MatrixIterator(Element* position) : m_position(position)
  {
  }

  Element* position()
  {
    return m_position;
  }

  bool operator==(const MatrixIterator& other) const
  {
    return m_position == other.m_position;
  }
  bool operator!=(const MatrixIterator& other) const
  {
    return !operator==(other);
  }
  MatrixIterator& operator++()
  {
    increment();
    return *this;
  }
  MatrixIterator operator++(int)
  {
    MatrixIterator tmp = *this;
    increment();
    return tmp;
  }
  value_type& operator*() const
  {
    return m_position->m_value;
  }
  value_type* operator->() const
  {
    return &(operator*());
  }
};
#endif

template<typename Element>
class Matrix
{
  std::size_t m_x, m_y;
  Element* m_data;
public:
  typedef Element value_type;
  typedef value_type* iterator;
  typedef const value_type* const_iterator;

  Matrix()
    : m_x(0), m_y(0), m_data(0)
  {
  }
  Matrix(std::size_t x, std::size_t y, Element* data)
    : m_x(x), m_y(y), m_data(data)
  {
  }

  iterator begin()
  {
    return m_data;
  }
  const_iterator begin() const
  {
    return m_data;
  }
  iterator end()
  {
    return m_data + size();
  }
  const_iterator end() const
  {
    return m_data + size();
  }

  value_type& operator[](std::size_t index)
  {
#if defined(_DEBUG)
    ASSERT_MESSAGE(index < size(), "array index out of bounds");
#endif
    return m_data[index];
  }
  const value_type& operator[](std::size_t index) const
  {
#if defined(_DEBUG)
    ASSERT_MESSAGE(index < size(), "array index out of bounds");
#endif
    return m_data[index];
  }
  value_type& operator()(std::size_t x, std::size_t y)
  {
    return m_data[x * m_y + y];
  }
  const value_type& operator()(std::size_t x, std::size_t y) const
  {
    return m_data[x * m_y + y];
  }
  value_type* data()
  {
    return m_data;
  }
  const value_type* data() const
  {
    return m_data;
  }
  std::size_t x() const
  {
    return m_x;
  }
  std::size_t y() const
  {
    return m_y;
  }
  std::size_t size() const
  {
    return m_x * m_y;
  }
  bool empty() const
  {
    return m_x == 0;
  }
};

/* greebo: A PatchControl consists of a vertex and a set of texture coordinates.
 * Multiple PatchControls form a PatchControlArray or (together with width and height) a PatchControlMatrix.
 */
class PatchControl {
public:
	Vector3 m_vertex;	// The coordinates of the control point
	Vector2 m_texcoord;	// The texture coordinates of this point
};

// greebo: This is a matrix of patch controls. Width and Height are needed to construct such a structure
typedef Matrix<PatchControl> PatchControlMatrix;

/* greebo: the abstract base class for a patch-creating class.
 * At the moment, the CommonPatchCreator, Doom3PatchCreator and Doom3PatchDef2Creator derive from this base class.   
 */
class PatchCreator :
	public RegisterableModule
{
public:
	// Create a patch and return the sceneNode 
	virtual scene::INodePtr createPatch() = 0;
	
	// Save the state of the patch to an UndoMemento for eventual reverting.
	virtual void Patch_undoSave(scene::INodePtr patch) const = 0;
	
	// Resize the patch to the specified number rows and columns
	virtual void Patch_resize(scene::INodePtr patch, std::size_t width, std::size_t height) const = 0;
	
	// Returns the control points and the dimensions of the patch
	virtual PatchControlMatrix Patch_getControlPoints(scene::INodePtr patch) const = 0;
	
	// Notify the patch that the control points have changed
	virtual void Patch_controlPointsChanged(scene::INodePtr patch) const = 0;
	
	// Get/Set the shader name of the patch
	virtual const std::string& Patch_getShader(scene::INodePtr patch) const = 0;
	virtual void Patch_setShader(scene::INodePtr patch, const std::string& shader) const = 0;
};

class Patch;
class IPatchNode
{
public:
	/** greebo: Retrieves the actual patch from a PatchNode
	 */
	virtual Patch& getPatch() = 0;
};
typedef boost::shared_ptr<IPatchNode> IPatchNodePtr;

inline bool Node_isPatch(scene::INodePtr node) {
	return boost::dynamic_pointer_cast<IPatchNode>(node) != NULL;
}

// Casts a node onto a patch
inline Patch* Node_getPatch(scene::INodePtr node) {
	IPatchNodePtr patchNode = boost::dynamic_pointer_cast<IPatchNode>(node);
	if (patchNode != NULL) {
		return &patchNode->getPatch();;
	}
	return NULL;
}

const std::string MODULE_PATCH("PatchModule");
const std::string DEF2("Def2");
const std::string DEF3("Def3");

// Acquires the PatchCreator of the given type ("Def2", "Def3")
inline PatchCreator& GlobalPatchCreator(const std::string& defType) {
	boost::shared_ptr<PatchCreator> _patchCreator(
		boost::static_pointer_cast<PatchCreator>(
			module::GlobalModuleRegistry().getModule(MODULE_PATCH + defType) // e.g. "PatchModuleDef2"
		)
	);
	return *_patchCreator;
}

#endif
