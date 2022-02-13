#pragma once

#include "math/Plane3.h"
#include "math/Line.h"
#include "render.h"
#include "ObservedSelectable.h"

#include "SelectableComponents.h"
#include "VertexSelection.h"
#include "Face.h"
#include "Brush.h"

typedef const Plane3* PlanePointer;
typedef PlanePointer* PlanesIterator;
class IRenderableCollector;

class FaceInstance;
typedef std::list<FaceInstance*> FaceInstanceSet;

class FaceInstance
{
private:
	Face* m_face;
    SelectionChangedSlot m_selectionChanged;
	selection::ObservedSelectable m_selectable;
	selection::ObservedSelectable m_selectableVertices;
	selection::ObservedSelectable m_selectableEdges;

	VertexSelection m_vertexSelection;
	VertexSelection m_edgeSelection;

	static FaceInstanceSet _selectedFaceInstances;

public:
	FaceInstance(Face& face, const SelectionChangedSlot& observer);
	FaceInstance(const FaceInstance& other);

	FaceInstance& operator=(const FaceInstance& other);

	Face& getFace();

	const Face& getFace() const;

	void selectedChanged(const ISelectable& selectable);

	bool selectedVertices() const;
	bool selectedEdges() const;

	bool isSelected() const;

	bool selectedComponents() const;
	bool selectedComponents(selection::ComponentSelectionMode mode) const;

	void setSelected(selection::ComponentSelectionMode mode, bool select);
	void invertSelected();

	template<typename Functor>
	void SelectedVertices_foreach(Functor functor) const {
		for (VertexSelection::const_iterator i = m_vertexSelection.begin(); i != m_vertexSelection.end(); ++i) {
			std::size_t index = getFace().getWinding().findAdjacent(*i);
			if (index != brush::c_brush_maxFaces) {
					functor(getFace().getWinding()[index].vertex);
				}
		}
	}

	template<typename Functor>
	void SelectedEdges_foreach(Functor functor) const {
		for (VertexSelection::const_iterator i = m_edgeSelection.begin(); i != m_edgeSelection.end(); ++i) {
			std::size_t index = getFace().getWinding().findAdjacent(*i);
			if (index != brush::c_brush_maxFaces) {
					const Winding& winding = getFace().getWinding();
					std::size_t adjacent = winding.next(index);
					functor(math::midPoint(winding[index].vertex, winding[adjacent].vertex));
				}
		}
	}

	template<typename Functor>
	void SelectedFaces_foreach(Functor functor) const {
		if (isSelected()) {
			functor(centroid());
		}
	}

	template<typename Functor>
	void SelectedComponents_foreach(Functor functor) const {
		SelectedVertices_foreach(functor);
		SelectedEdges_foreach(functor);
		SelectedFaces_foreach(functor);
	}

	void iterate_selected(AABB& aabb) const;

	bool intersectVolume(const VolumeTest& volume) const;
	bool intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const;

	void testSelect(SelectionTest& test, SelectionIntersection& best);

	void testSelect(Selector& selector, SelectionTest& test);
	void testSelect_centroid(Selector& selector, SelectionTest& test);

	void selectPlane(Selector& selector, const Line& line, PlanesIterator first, PlanesIterator last, const PlaneCallback& selectedPlaneCallback);
	void selectReversedPlane(Selector& selector, const SelectedPlanes& selectedPlanes);

	void transformComponents(const Matrix4& matrix);

	void snapto(float snap);

	void snapComponents(float snap);
	void update_move_planepts_vertex(std::size_t index);
	void update_move_planepts_vertex2(std::size_t index, std::size_t other);
	void update_selection_vertex();
	void select_vertex(std::size_t index, bool select);

	bool selected_vertex(std::size_t index) const;

	void update_move_planepts_edge(std::size_t index);
	void update_selection_edge();
	void select_edge(std::size_t index, bool select);

	bool selected_edge(std::size_t index) const;

	const Vector3& centroid() const;

	void connectivityChanged();

	bool faceIsVisible() const
	{
		return m_face->isVisible();
	}

	void updateFaceVisibility();

	// greebo: Provides access to the set of selected face instances
	// This should be a replacement for the old g_SelectedFaceInstances global.
	static FaceInstanceSet& Selection();

}; // class FaceInstance

// ===========================================================================================

typedef std::vector<FaceInstance> FaceInstances;

