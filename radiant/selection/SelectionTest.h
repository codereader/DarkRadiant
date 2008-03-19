#ifndef SELECTIONTEST_H_
#define SELECTIONTEST_H_

#include "math/matrix.h"
#include "math/Vector3.h"
#include "selectable.h"

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

class testselect_entity_visible : public scene::Graph::Walker {
  Selector& _selector;
  SelectionTest& _test;
public:
  testselect_entity_visible(Selector& selector, SelectionTest& test)
    : _selector(selector), _test(test) {}

  bool pre(const scene::Path& path, const scene::INodePtr& node) const;  
  void post(const scene::Path& path, const scene::INodePtr& node) const;
};

class testselect_primitive_visible : public scene::Graph::Walker {
  Selector& _selector;
  SelectionTest& _test;
	bool _selectChildPrimitives;
public:
	/** greebo: Set the selectChildPrimitives bool to TRUE if child primitives of entities like func_static
	 * should be selected as well. This should be set to TRUE for Manipulator checks.
	 */
	testselect_primitive_visible(Selector& selector, SelectionTest& test, bool selectChildPrimitives) : 
		_selector(selector), 
		_test(test),
		_selectChildPrimitives(selectChildPrimitives) 
	{}

  bool pre(const scene::Path& path, const scene::INodePtr& node) const;
  void post(const scene::Path& path, const scene::INodePtr& node) const;
};

/** greebo: Tests for any primitives/entities matching the selectiontest
 */
class testselect_any_visible : 
	public scene::Graph::Walker 
{
	Selector& _selector;
	SelectionTest& _test;
	bool _selectChildPrimitives;
public:
	testselect_any_visible(Selector& selector, SelectionTest& test, bool selectChildPrimitives) : 
		_selector(selector), 
		_test(test),
		_selectChildPrimitives(selectChildPrimitives)
	{}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const;  
	void post(const scene::Path& path, const scene::INodePtr& node) const;
};

class testselect_component_visible : public scene::Graph::Walker {
  Selector& _selector;
  SelectionTest& _test;
  SelectionSystem::EComponentMode _mode;
public:
  testselect_component_visible(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode)
    : _selector(selector), _test(test), _mode(mode) {}
  
  bool pre(const scene::Path& path, const scene::INodePtr& node) const;
};

class testselect_component_visible_selected : public scene::Graph::Walker {
  Selector& _selector;
  SelectionTest& _test;
  SelectionSystem::EComponentMode _mode;
public:
  testselect_component_visible_selected(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode)
    : _selector(selector), _test(test), _mode(mode) {}
  
  bool pre(const scene::Path& path, const scene::INodePtr& node) const;
};

// --------------------------------------------------------------------------------

void Scene_TestSelect_Primitive(Selector& selector, SelectionTest& test, const VolumeTest& volume, bool selectChildPrimitives = true);
void Scene_TestSelect_Component(Selector& selector, SelectionTest& test, const VolumeTest& volume, SelectionSystem::EComponentMode componentMode);
void Scene_TestSelect_Component_Selected(Selector& selector, SelectionTest& test, const VolumeTest& volume, SelectionSystem::EComponentMode componentMode);

inline void ConstructSelectionTest(View& view, const Rectangle selection_box) {
	view.EnableScissor(selection_box.min[0], selection_box.max[0], 
					   selection_box.min[1], selection_box.max[1]);
}

#endif /*SELECTIONTEST_H_*/
