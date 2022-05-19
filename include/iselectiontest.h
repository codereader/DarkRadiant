#pragma once

#include <cstddef>

#include "math/FloatTools.h"
#include "inode.h"
#include "iselection.h"
#include <memory>
#include <functional>

class SelectionIntersection
{
private:
	float _depth;
	float _distance;
public:
	SelectionIntersection() :
		_depth(1),
		_distance(2)
	{}

	SelectionIntersection(float depth, float distance) :
		_depth(depth),
		_distance(distance)
	{}

	bool operator<(const SelectionIntersection& other) const
	{
		if (_distance != other._distance)
		{
			return _distance < other._distance;
		}

		if (_depth != other._depth)
		{
			return _depth < other._depth;
		}

		return false;
	}

	bool equalEpsilon(const SelectionIntersection& other, float distanceEpsilon, float depthEpsilon) const
	{
		return float_equal_epsilon(_distance, other._distance, distanceEpsilon) &&
			   float_equal_epsilon(_depth, other._depth, depthEpsilon);
	}

	float depth() const
	{
		return _depth;
	}

	bool isValid() const
	{
		return depth() < 1;
	}

	// returns true if self is closer than other
	bool isCloserThan(const SelectionIntersection& other) const
	{
		return *this < other;
	}

	// assigns other to *this if other is closer than *this
	void assignIfCloser(const SelectionIntersection& other)
	{
		if (other.isCloserThan(*this))
		{
			*this = other;
		}
	}
};

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
  virtual void TestLineStrip(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) = 0;
  virtual void TestTriangles(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) = 0;
  virtual void TestQuads(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) = 0;
  virtual void TestQuadStrip(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) = 0;
};
typedef std::shared_ptr<SelectionTest> SelectionTestPtr;

/**
 * @brief Abstract interface for an object which collects possibly-selected
 * objects, keeping track of the best intersection which typically determines
 * which candidate object should be selected when more than one is possible.
 *
 * This object exposes a stateful interface. A call to pushSelectable() sets the
 * given ISelectable object as the "current" selectable. Subsequent calls to
 * addIntersection() apply to this current selectable, replacing the current
 * intersection if the newly-submitted one is better. The operation is completed
 * when popSelectable() is called, and the current selectable along with its
 * best intersection is added to the internal list(s).
 *
 * Despite the usage of "push" and "pop" terminology, there is only one current
 * selectable, and no internal stack.
 */
class Selector
{
public:
    virtual ~Selector() {}

    /// Set the given object as the current selectable
    virtual void pushSelectable(ISelectable& selectable) = 0;

    /// Commit the current selectable, storing it along with its best intersection
    virtual void popSelectable() = 0;

    /**
     * @brief Add a candidate intersection for the current selectable.
     *
     * The candidate intersection is only stored if it is a better fit than the
     * best intersection seen so far.
     */
    virtual void addIntersection(const SelectionIntersection& intersection) = 0;

    /// Add a selectable object and immediately commit it with a null intersection
    void addWithNullIntersection(ISelectable& selectable)
    {
        pushSelectable(selectable);
        addIntersection(SelectionIntersection(0, 0));
        popSelectable();
    }

    /// Add a selectable object and immediately commit it with the given intersection
    void addWithIntersection(ISelectable& selectable, const SelectionIntersection& intersection)
    {
        pushSelectable(selectable);
        addIntersection(intersection);
        popSelectable();
    }
};

class VolumeTest;
class SelectionTestable
{
public:
    virtual ~SelectionTestable() {}
	virtual void testSelect(Selector& selector, SelectionTest& test) = 0;
};
typedef std::shared_ptr<SelectionTestable> SelectionTestablePtr;

inline SelectionTestablePtr Node_getSelectionTestable(const scene::INodePtr& node) {
	return std::dynamic_pointer_cast<SelectionTestable>(node);
}

class ComponentSelectionTestable {
public:
    virtual ~ComponentSelectionTestable() {}
	virtual bool isSelectedComponents() const = 0;
	virtual void setSelectedComponents(bool select, selection::ComponentSelectionMode mode) = 0;
	virtual void invertSelectedComponents(selection::ComponentSelectionMode mode) = 0;
	virtual void testSelectComponents(Selector& selector, SelectionTest& test, selection::ComponentSelectionMode mode) = 0;
};
typedef std::shared_ptr<ComponentSelectionTestable> ComponentSelectionTestablePtr;

inline ComponentSelectionTestablePtr Node_getComponentSelectionTestable(const scene::INodePtr& node) {
	return std::dynamic_pointer_cast<ComponentSelectionTestable>(node);
}

class Plane3;
typedef std::function<void (const Plane3&)> PlaneCallback;

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
typedef std::shared_ptr<PlaneSelectable> PlaneSelectablePtr;

inline PlaneSelectablePtr Node_getPlaneSelectable(const scene::INodePtr& node) {
	return std::dynamic_pointer_cast<PlaneSelectable>(node);
}
