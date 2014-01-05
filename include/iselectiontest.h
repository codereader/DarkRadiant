#pragma once

#include <cstddef>

#include "math/Vector3.h"
#include "inode.h"
#include "iselection.h"
#include <boost/shared_ptr.hpp>
#include <boost/function/function_fwd.hpp>

class SelectionIntersection
{
  float m_depth;
  float m_distance;
public:
  SelectionIntersection() : m_depth(1), m_distance(2)
  {
  }
  SelectionIntersection(float depth, float distance) : m_depth(depth), m_distance(distance)
  {
  }
  bool operator<(const SelectionIntersection& other) const
  {
    if(m_distance != other.m_distance)
    {
      return m_distance < other.m_distance;
    }
    if(m_depth != other.m_depth)
    {
      return m_depth < other.m_depth;
    }
    return false;
  }
  bool equalEpsilon(const SelectionIntersection& other, float distanceEpsilon, float depthEpsilon) const
  {
    return float_equal_epsilon(m_distance, other.m_distance, distanceEpsilon)
      && float_equal_epsilon(m_depth, other.m_depth, depthEpsilon);
  }
  float depth() const
  {
    return m_depth;
  }
  bool valid() const
  {
    return depth() < 1;
  }
};

// returns true if self is closer than other
inline bool SelectionIntersection_closer(const SelectionIntersection& self, const SelectionIntersection& other)
{
  return self < other;
}

// assigns other to best if other is closer than best
inline void assign_if_closer(SelectionIntersection& best, const SelectionIntersection& other)
{
  if(SelectionIntersection_closer(other, best))
  {
    best = other;
  }
}

/**
 * greebo: A helper class allowing to traverse a sequence of Vector3
 * objects in memory. The Vector3 objects can have a certain distance
 * (stride) which is passed to the constructor. Incrementing the contained
 * iterator object moves from one Vector3 to the next in memory.
 */
class VertexPointer
{
	typedef const unsigned char* byte_pointer;
public:
	typedef const Vector3* vector_pointer;
	typedef const Vector3& vector_reference;

  class iterator
  {
  public:
    iterator() {}
    iterator(byte_pointer vertices, std::size_t stride)
      : m_iter(vertices), m_stride(stride) {}

    bool operator==(const iterator& other) const
    {
      return m_iter == other.m_iter;
    }
    bool operator!=(const iterator& other) const
    {
      return !operator==(other);
    }

    iterator operator+(std::size_t i)
    {
      return iterator(m_iter + i * m_stride, m_stride);
    }
    iterator operator+=(std::size_t i)
    {
      m_iter += i * m_stride;
      return *this;
    }
    iterator& operator++()
    {
      m_iter += m_stride;
      return *this;
    }
    iterator operator++(int)
    {
      iterator tmp = *this;
      m_iter += m_stride;
      return tmp;
    }
    vector_reference operator*() const
    {
      return *reinterpret_cast<vector_pointer>(m_iter);
    }
  private:
    byte_pointer m_iter;
    std::size_t m_stride;
  };

  VertexPointer(vector_pointer vertices, std::size_t stride)
    : m_vertices(reinterpret_cast<byte_pointer>(vertices)), m_stride(stride) {}

  iterator begin() const
  {
    return iterator(m_vertices, m_stride);
  }

  vector_reference operator[](std::size_t i) const
  {
    return *reinterpret_cast<vector_pointer>(m_vertices + m_stride*i);
  }

private:
    // The address of the first Vector3 object
	byte_pointer m_vertices;

	// The distance (in bytes) to the next object in memory
	std::size_t m_stride;
};

class IndexPointer
{
public:
  typedef unsigned int index_type;
  typedef const index_type* pointer;

  class iterator
  {
  public:
    iterator(pointer iter) : m_iter(iter) {}

    bool operator==(const iterator& other) const
    {
      return m_iter == other.m_iter;
    }
    bool operator!=(const iterator& other) const
    {
      return !operator==(other);
    }

    iterator operator+(std::size_t i)
    {
      return m_iter + i;
    }
    iterator operator+=(std::size_t i)
    {
      return m_iter += i;
    }
    iterator operator++()
    {
      return ++m_iter;
    }
    iterator operator++(int)
    {
      return m_iter++;
    }
    const index_type& operator*() const
    {
      return *m_iter;
    }
  private:
    void increment()
    {
      ++m_iter;
    }
    pointer m_iter;
  };

  IndexPointer(pointer indices, std::size_t count)
    : m_indices(indices), m_finish(indices + count) {}

  iterator begin() const
  {
    return m_indices;
  }
  iterator end() const
  {
    return m_finish;
  }

private:
  pointer m_indices;
  pointer m_finish;
};

template<typename Element> class BasicVector3;
typedef BasicVector3<double> Vector3;
class Matrix4;
class VolumeTest;

class SelectionTest
{
public:
  virtual ~SelectionTest() {}
  virtual void BeginMesh(const Matrix4& localToWorld, bool twoSided = false) = 0;
  virtual const VolumeTest& getVolume() const = 0;
  virtual const Vector3& getNear() const = 0;
  virtual const Vector3& getFar() const = 0;
  virtual void TestPoint(const Vector3& point, SelectionIntersection& best) = 0;
  virtual void TestPolygon(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) = 0;
  virtual void TestLineLoop(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) = 0;
  virtual void TestLineStrip(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) = 0;
  virtual void TestLines(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) = 0;
  virtual void TestTriangles(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) = 0;
  virtual void TestQuads(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) = 0;
  virtual void TestQuadStrip(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) = 0;
};

class Selector
{
public:
  virtual ~Selector() {}
  virtual void pushSelectable(Selectable& selectable) = 0;
  virtual void popSelectable() = 0;
  virtual void addIntersection(const SelectionIntersection& intersection) = 0;
};

inline void Selector_add(Selector& selector, Selectable& selectable)
{
  selector.pushSelectable(selectable);
  selector.addIntersection(SelectionIntersection(0, 0));
  selector.popSelectable();
}

inline void Selector_add(Selector& selector, Selectable& selectable, const SelectionIntersection& intersection)
{
  selector.pushSelectable(selectable);
  selector.addIntersection(intersection);
  selector.popSelectable();
}

class VolumeTest;
class SelectionTestable
{
public:
    virtual ~SelectionTestable() {}
	virtual void testSelect(Selector& selector, SelectionTest& test) = 0;
};
typedef boost::shared_ptr<SelectionTestable> SelectionTestablePtr;

inline SelectionTestablePtr Node_getSelectionTestable(const scene::INodePtr& node) {
	return boost::dynamic_pointer_cast<SelectionTestable>(node);
}

class ComponentSelectionTestable {
public:
    virtual ~ComponentSelectionTestable() {}
	virtual bool isSelectedComponents() const = 0;
	virtual void setSelectedComponents(bool select, SelectionSystem::EComponentMode mode) = 0;
	virtual void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) = 0;
};
typedef boost::shared_ptr<ComponentSelectionTestable> ComponentSelectionTestablePtr;

inline ComponentSelectionTestablePtr Node_getComponentSelectionTestable(const scene::INodePtr& node) {
	return boost::dynamic_pointer_cast<ComponentSelectionTestable>(node);
}

class Plane3;
typedef boost::function<void (const Plane3&)> PlaneCallback;

class SelectedPlanes
{
public:
  virtual ~SelectedPlanes() {}
  virtual bool contains(const Plane3& plane) const = 0;
};

class PlaneSelectable
{
public:
    virtual ~PlaneSelectable() {}
	virtual void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) = 0;
	virtual void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) = 0;
};
typedef boost::shared_ptr<PlaneSelectable> PlaneSelectablePtr;

inline PlaneSelectablePtr Node_getPlaneSelectable(const scene::INodePtr& node) {
	return boost::dynamic_pointer_cast<PlaneSelectable>(node);
}
