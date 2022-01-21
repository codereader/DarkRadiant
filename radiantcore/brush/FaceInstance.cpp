#include "FaceInstance.h"

#include "ifilter.h"
#include "ibrush.h"
#include "irenderable.h"
#include "iscenegraph.h"

#include "math/Frustum.h"
#include <functional>

inline bool triangle_reversed(std::size_t x, std::size_t y, std::size_t z) {
	return !((x < y && y < z) || (z < x && x < y) || (y < z && z < x));
}

template<typename Element>
inline Vector3 triangle_cross(const BasicVector3<Element>& x, const BasicVector3<Element> y, const BasicVector3<Element>& z) {
	return (y - x).cross(z - x);
}
template<typename Element>
inline bool triangles_same_winding(const BasicVector3<Element>& x1, const BasicVector3<Element> y1, const BasicVector3<Element>& z1,
								   const BasicVector3<Element>& x2, const BasicVector3<Element> y2, const BasicVector3<Element>& z2)
{
	return triangle_cross(x1, y1, z1).dot(triangle_cross(x2, y2, z2)) > 0;
}

// -------------- FaceInstance implementation ---------------------------------------

// Static member definition
FaceInstanceSet FaceInstance::_selectedFaceInstances;

FaceInstance::FaceInstance(Face& face, const SelectionChangedSlot& observer) :
	m_face(&face),
	m_selectionChanged(observer),
	m_selectable(std::bind(&FaceInstance::selectedChanged, this, std::placeholders::_1)),
	m_selectableVertices(observer),
	m_selectableEdges(observer)
{}

FaceInstance::FaceInstance(const FaceInstance& other) :
	m_face(other.m_face),
	m_selectionChanged(other.m_selectionChanged),
	m_selectable(std::bind(&FaceInstance::selectedChanged, this, std::placeholders::_1)),
	m_selectableVertices(other.m_selectableVertices),
	m_selectableEdges(other.m_selectableEdges)
{}

FaceInstance& FaceInstance::operator=(const FaceInstance& other) {
	m_face = other.m_face;
	return *this;
}

Face& FaceInstance::getFace() {
	return *m_face;
}

const Face& FaceInstance::getFace() const {
	return *m_face;
}

void FaceInstance::selectedChanged(const ISelectable& selectable)
{
	if (selectable.isSelected())
	{
		Selection().push_back(this);
	}
	else
	{
		FaceInstanceSet::reverse_iterator found = std::find(Selection().rbegin(), Selection().rend(), this);

		// Emit an error if the instance is not in the list
		ASSERT_MESSAGE(found != Selection().rend(), "selection-tracking error");

		Selection().erase(--found.base());
	}

	if (m_selectionChanged)
	{
		m_selectionChanged(selectable);
	}
}

bool FaceInstance::selectedVertices() const {
	return !m_vertexSelection.empty();
}

bool FaceInstance::selectedEdges() const {
	return !m_edgeSelection.empty();
}

bool FaceInstance::isSelected() const {
	return m_selectable.isSelected();
}

bool FaceInstance::selectedComponents() const
{
	return !m_vertexSelection.empty() || !m_edgeSelection.empty() || m_selectable.isSelected();
}

bool FaceInstance::selectedComponents(selection::ComponentSelectionMode mode) const
{
	switch (mode)
    {
		case selection::ComponentSelectionMode::Vertex:
			return selectedVertices();
		case selection::ComponentSelectionMode::Edge:
			return selectedEdges();
		case selection::ComponentSelectionMode::Face:
			return isSelected();
		default:
			return false;
	}
}

void FaceInstance::setSelected(selection::ComponentSelectionMode mode, bool select)
{
	switch (mode)
    {
		case selection::ComponentSelectionMode::Face:
			m_selectable.setSelected(select);
			break;
		case selection::ComponentSelectionMode::Vertex:
			ASSERT_MESSAGE(!select, "select-all not supported");

			m_vertexSelection.clear();
			m_selectableVertices.setSelected(false);
			break;
		case selection::ComponentSelectionMode::Edge:
			ASSERT_MESSAGE(!select, "select-all not supported");

			m_edgeSelection.clear();
			m_selectableEdges.setSelected(false);
			break;
		default:
			break;
	}
}

void FaceInstance::invertSelected()
{
	switch (GlobalSelectionSystem().ComponentMode()) 
    {
        case selection::ComponentSelectionMode::Face:
			m_selectable.setSelected(!m_selectable.isSelected());
			break;
		case selection::ComponentSelectionMode::Vertex:
			break;
		case selection::ComponentSelectionMode::Edge:
			break;
		default:
			break;
	}
}

void FaceInstance::iterate_selected(AABB& aabb) const {
	SelectedComponents_foreach(AABBExtendByPoint(aabb));
}

bool FaceInstance::intersectVolume(const VolumeTest& volume) const
{
	return m_face->intersectVolume(volume);
}

bool FaceInstance::intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const
{
	return m_face->intersectVolume(volume, localToWorld);
}

void FaceInstance::testSelect(SelectionTest& test, SelectionIntersection& best) {
	if (getFace().getFaceShader().getGLShader()->getMaterial()->isVisible()) {
		m_face->testSelect(test, best);
	}
}

void FaceInstance::testSelect(Selector& selector, SelectionTest& test) {
	SelectionIntersection best;
	testSelect(test, best);
	if (best.isValid()) {
		Selector_add(selector, m_selectable, best);
	}
}

void FaceInstance::testSelect_centroid(Selector& selector, SelectionTest& test) {
	if (m_face->contributes()) {
		SelectionIntersection best;
		m_face->testSelect_centroid(test, best);
		if (best.isValid()) {
			Selector_add(selector, m_selectable, best);
		}
	}
}

void FaceInstance::selectPlane(Selector& selector, const Line& line, PlanesIterator first, PlanesIterator last, const PlaneCallback& selectedPlaneCallback)
{
	for (Winding::const_iterator i = getFace().getWinding().begin(); i != getFace().getWinding().end(); ++i) {
		Vector3 v(line.getClosestPoint(i->vertex) - i->vertex);
		auto dot = getFace().plane3().normal().dot(v);
		if (dot <= 0) {
			return;
		}
	}

	Selector_add(selector, m_selectable);

	selectedPlaneCallback(getFace().plane3());
}

void FaceInstance::selectReversedPlane(Selector& selector, const SelectedPlanes& selectedPlanes) {
	if (selectedPlanes.contains(-(getFace().plane3()))) {
		Selector_add(selector, m_selectable);
	}
}

void FaceInstance::transformComponents(const Matrix4& matrix) {
	if (isSelected()) {
		m_face->transform(matrix);
	}

	if (selectedVertices())
	{
		if (m_vertexSelection.size() == 1)
		{
			m_face->m_move_planeptsTransformed[1] = matrix.transformPoint(m_face->m_move_planeptsTransformed[1]);
			m_face->assign_planepts(m_face->m_move_planeptsTransformed);
		}
		else if (m_vertexSelection.size() == 2)
		{
			m_face->m_move_planeptsTransformed[1] = matrix.transformPoint(m_face->m_move_planeptsTransformed[1]);
			m_face->m_move_planeptsTransformed[2] = matrix.transformPoint(m_face->m_move_planeptsTransformed[2]);
			m_face->assign_planepts(m_face->m_move_planeptsTransformed);
		}
		else if (m_vertexSelection.size() >= 3)
		{
			m_face->m_move_planeptsTransformed[0] = matrix.transformPoint(m_face->m_move_planeptsTransformed[0]);
			m_face->m_move_planeptsTransformed[1] = matrix.transformPoint(m_face->m_move_planeptsTransformed[1]);
			m_face->m_move_planeptsTransformed[2] = matrix.transformPoint(m_face->m_move_planeptsTransformed[2]);
			m_face->assign_planepts(m_face->m_move_planeptsTransformed);
		}
	}

	if (selectedEdges())
	{
		if (m_edgeSelection.size() == 1)
		{
			m_face->m_move_planeptsTransformed[0] = matrix.transformPoint(m_face->m_move_planeptsTransformed[0]);
			m_face->m_move_planeptsTransformed[1] = matrix.transformPoint(m_face->m_move_planeptsTransformed[1]);
			m_face->assign_planepts(m_face->m_move_planeptsTransformed);
		}
		else if (m_edgeSelection.size() >= 2)
		{
			m_face->m_move_planeptsTransformed[0] = matrix.transformPoint(m_face->m_move_planeptsTransformed[0]);
			m_face->m_move_planeptsTransformed[1] = matrix.transformPoint(m_face->m_move_planeptsTransformed[1]);
			m_face->m_move_planeptsTransformed[2] = matrix.transformPoint(m_face->m_move_planeptsTransformed[2]);
			m_face->assign_planepts(m_face->m_move_planeptsTransformed);
		}
	}
}

void FaceInstance::snapto(float snap) {
	m_face->snapto(snap);
}

void FaceInstance::snapComponents(float snap) {
	if (isSelected()) {
		snapto(snap);
	}

	if (selectedVertices()) {
		m_face->m_move_planepts[0].snap(snap);
		m_face->m_move_planepts[1].snap(snap);
		m_face->m_move_planepts[2].snap(snap);
		m_face->assign_planepts(m_face->m_move_planepts);
		planepts_assign(m_face->m_move_planeptsTransformed, m_face->m_move_planepts);
		m_face->freezeTransform();
	}

	if (selectedEdges()) {
		m_face->m_move_planepts[0].snap(snap);
		m_face->m_move_planepts[1].snap(snap);
		m_face->m_move_planepts[2].snap(snap);
		m_face->assign_planepts(m_face->m_move_planepts);
		planepts_assign(m_face->m_move_planeptsTransformed, m_face->m_move_planepts);
		m_face->freezeTransform();
	}
}

void FaceInstance::update_move_planepts_vertex(std::size_t index) {
	m_face->update_move_planepts_vertex(index, m_face->m_move_planepts);
}

void FaceInstance::update_move_planepts_vertex2(std::size_t index, std::size_t other)
{
	ASSERT_MESSAGE(index < m_face->getWinding().size(), "select_vertex: invalid index");

	const std::size_t opposite = m_face->getWinding().opposite(index, other);

	if (triangle_reversed(index, other, opposite)) {
		std::swap(index, other);
	}

	ASSERT_MESSAGE(
		triangles_same_winding(
			m_face->getWinding()[opposite].vertex,
			m_face->getWinding()[index].vertex,
			m_face->getWinding()[other].vertex,
			m_face->getWinding()[0].vertex,
			m_face->getWinding()[1].vertex,
			m_face->getWinding()[2].vertex
		),
		"update_move_planepts_vertex2: error"
	)

	m_face->m_move_planepts[0] = m_face->getWinding()[opposite].vertex;
	m_face->m_move_planepts[1] = m_face->getWinding()[index].vertex;
	m_face->m_move_planepts[2] = m_face->getWinding()[other].vertex;
	planepts_quantise(m_face->m_move_planepts, GRID_MIN); // winding points are very inaccurate
}

void FaceInstance::update_selection_vertex() {
	if (m_vertexSelection.size() == 0) {
		m_selectableVertices.setSelected(false);
	}
	else {
		m_selectableVertices.setSelected(true);

		if (m_vertexSelection.size() == 1) {
				std::size_t index = getFace().getWinding().findAdjacent(*m_vertexSelection.begin());

				if (index != brush::c_brush_maxFaces) {
						update_move_planepts_vertex(index);
					}
			}
		else if (m_vertexSelection.size() == 2) {
				std::size_t index = getFace().getWinding().findAdjacent(*m_vertexSelection.begin());
				std::size_t other = getFace().getWinding().findAdjacent(*(++m_vertexSelection.begin()));

				if (index != brush::c_brush_maxFaces
						&& other != brush::c_brush_maxFaces) {
						update_move_planepts_vertex2(index, other);
					}
			}
	}
}

void FaceInstance::select_vertex(std::size_t index, bool select) {
	if (select) {
		VertexSelection_insert(m_vertexSelection, getFace().getWinding()[index].adjacent);
	}
	else {
		VertexSelection_erase(m_vertexSelection, getFace().getWinding()[index].adjacent);
	}

	SceneChangeNotify();
	update_selection_vertex();
}

bool FaceInstance::selected_vertex(std::size_t index) const {
	return VertexSelection_find(m_vertexSelection, getFace().getWinding()[index].adjacent) != m_vertexSelection.end();
}

void FaceInstance::update_move_planepts_edge(std::size_t index)
{
	ASSERT_MESSAGE(index < m_face->getWinding().size(), "select_edge: invalid index");

	std::size_t adjacent = m_face->getWinding().next(index);
	std::size_t opposite = m_face->getWinding().opposite(index);
	m_face->m_move_planepts[0] = m_face->getWinding()[index].vertex;
	m_face->m_move_planepts[1] = m_face->getWinding()[adjacent].vertex;
	m_face->m_move_planepts[2] = m_face->getWinding()[opposite].vertex;
	planepts_quantise(m_face->m_move_planepts, GRID_MIN); // winding points are very inaccurate
}

void FaceInstance::update_selection_edge() {
	if (m_edgeSelection.size() == 0) {
		m_selectableEdges.setSelected(false);
	}
	else {
		m_selectableEdges.setSelected(true);

		if (m_edgeSelection.size() == 1) {
				std::size_t index = getFace().getWinding().findAdjacent(*m_edgeSelection.begin());

				if (index != brush::c_brush_maxFaces) {
						update_move_planepts_edge(index);
					}
			}
	}
}

void FaceInstance::select_edge(std::size_t index, bool select) {
	if (select) {
		VertexSelection_insert(m_edgeSelection, getFace().getWinding()[index].adjacent);
	}
	else {
		VertexSelection_erase(m_edgeSelection, getFace().getWinding()[index].adjacent);
	}

	SceneChangeNotify();
	update_selection_edge();
}

bool FaceInstance::selected_edge(std::size_t index) const {
	return VertexSelection_find(m_edgeSelection, getFace().getWinding()[index].adjacent) != m_edgeSelection.end();
}

const Vector3& FaceInstance::centroid() const {
	return m_face->centroid();
}

void FaceInstance::connectivityChanged() {
	// This occurs when a face is added or removed.
	// The current vertex and edge selections no longer valid and must be cleared.
	m_vertexSelection.clear();
	m_selectableVertices.setSelected(false);
	m_edgeSelection.clear();
	m_selectableEdges.setSelected(false);
}

void FaceInstance::updateFaceVisibility()
{
	getFace().updateFaceVisibility();
}

FaceInstanceSet& FaceInstance::Selection()
{
	return _selectedFaceInstances;
}
