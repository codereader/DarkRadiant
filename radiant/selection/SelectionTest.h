#ifndef SELECTIONTEST_H_
#define SELECTIONTEST_H_

#include "math/matrix.h"
#include "math/Vector3.h"
#include "iselectable.h"

#include "view.h"
#include "BestPoint.h"
#include "SelectionBox.h"

class SelectionVolume : public SelectionTest {
  Matrix4 _local2view;
  const View& _view;
  clipcull_t _cull;
  Vector3 _near;
  Vector3 _far;
public:
  SelectionVolume(const View& view): _view(view) {}

  const VolumeTest& getVolume() const {
    return _view;
  }

  const Vector3& getNear() const {
    return _near;
  }
  
  const Vector3& getFar() const {
    return _far;
  }

  void BeginMesh(const Matrix4& localToWorld, bool twoSided);
  void TestPoint(const Vector3& point, SelectionIntersection& best);
  void TestPolygon(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best);
  void TestLineLoop(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best);
  void TestLineStrip(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best);
  void TestLines(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best);
  void TestTriangles(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best);
  void TestQuads(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best);
  void TestQuadStrip(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best);
};

// --------------------------------------------------------------------------------

// Base class for SelectionTesters, provides some convenience methods
class SelectionTestWalker :
	public scene::Graph::Walker
{
protected:
	void printNodeName(const scene::INodePtr& node);

	// Returns non-NULL if the given node is an Entity
	scene::INodePtr getEntityNode(const scene::INodePtr& node);

	// Returns non-NULL if the given node's parent is a GroupNode
	scene::INodePtr getParentGroupEntity(const scene::INodePtr& node);

	// Returns true if the node is worldspawn
	bool entityIsWorldspawn(const scene::INodePtr& node);
};

// A Selector which is testing for entities. This successfully
// checks for selections of child primitives of func_* entities too.
class EntitySelector :
	public SelectionTestWalker
{
private:
	Selector& _selector;
	SelectionTest& _test;

public:
	EntitySelector(Selector& selector, SelectionTest& test) :
		_selector(selector),
		_test(test)
	{}

	bool visit(const scene::INodePtr& node);
};

// A Selector looking for worldspawn primitives only. 
class PrimitiveSelector :
	public SelectionTestWalker
{
private:
	Selector& _selector;
	SelectionTest& _test;

public:
	PrimitiveSelector(Selector& selector, SelectionTest& test) :
		_selector(selector),
		_test(test)
	{}

	bool visit(const scene::INodePtr& node);
};

// A selector testing for all kinds of selectable items, entities and primitives.
// Worldspawn primitives are selected directly, for child primitives of func_* ents
// the selection will be "relayed" to the parent entity.
class AnySelector :
	public SelectionTestWalker
{
private:
	Selector& _selector;
	SelectionTest& _test;

public:
	AnySelector(Selector& selector, SelectionTest& test) :
		_selector(selector),
		_test(test)
	{}

	bool visit(const scene::INodePtr& node);
};

// A class seeking for components, can be used either to traverse the
// selectionsystem or the scene graph as a whole.
class ComponentSelector :
	public SelectionTestWalker,
	public SelectionSystem::Visitor
{
private:
	Selector& _selector;
	SelectionTest& _test;
	SelectionSystem::EComponentMode _mode;

public:
	ComponentSelector(Selector& selector, SelectionTest& test, 
					  SelectionSystem::EComponentMode mode) :
		_selector(selector), 
		_test(test), 
		_mode(mode)
	{}

	// scene::Graph::Walker implementation
	bool visit(const scene::INodePtr& node);

	// SelectionSystem::Visitor implementation
	void visit(const scene::INodePtr& node) const;
};

inline void ConstructSelectionTest(View& view, const Rectangle& selection_box)
{
	view.EnableScissor(selection_box.min[0], selection_box.max[0], 
					   selection_box.min[1], selection_box.max[1]);
}

#endif /*SELECTIONTEST_H_*/
