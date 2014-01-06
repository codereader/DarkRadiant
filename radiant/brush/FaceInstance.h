#pragma once

#include "math/Plane3.h"
#include "math/Line.h"
#include "render.h"
#include "render/VectorLightList.h"
#include "ObservedSelectable.h"

#include "SelectableComponents.h"
#include "VertexSelection.h"
#include "Face.h"
#include "Brush.h"

typedef const Plane3* PlanePointer;
typedef PlanePointer* PlanesIterator;
class RenderableCollector;

class FaceInstance;
typedef std::list<FaceInstance*> FaceInstanceSet;

class FaceInstance
{
private:
	Face* m_face;
	selection::ObservedSelectable m_selectable;
	selection::ObservedSelectable m_selectableVertices;
	selection::ObservedSelectable m_selectableEdges;
	SelectionChangedSlot m_selectionChanged;

	VertexSelection m_vertexSelection;
	VertexSelection m_edgeSelection;

	static FaceInstanceSet _selectedFaceInstances;

public:
	mutable render::lib::VectorLightList m_lights;

	FaceInstance(Face& face, const SelectionChangedSlot& observer);
	FaceInstance(const FaceInstance& other);

	FaceInstance& operator=(const FaceInstance& other);

	Face& getFace();

	const Face& getFace() const;

	void selectedChanged(const Selectable& selectable);

	bool selectedVertices() const;
	bool selectedEdges() const;

	bool isSelected() const;

	bool selectedComponents() const;
	bool selectedComponents(SelectionSystem::EComponentMode mode) const;

	void setSelected(SelectionSystem::EComponentMode mode, bool select);
	void invertSelected();

	template<typename Functor>
	void SelectedVertices_foreach(Functor functor) const {
		for (VertexSelection::const_iterator i = m_vertexSelection.begin(); i != m_vertexSelection.end(); ++i) {
			std::size_t index = getFace().getWinding().findAdjacent(*i);
			if (index != c_brush_maxFaces) {
					functor(getFace().getWinding()[index].vertex);
				}
		}
	}

	template<typename Functor>
	void SelectedEdges_foreach(Functor functor) const {
		for (VertexSelection::const_iterator i = m_edgeSelection.begin(); i != m_edgeSelection.end(); ++i) {
			std::size_t index = getFace().getWinding().findAdjacent(*i);
			if (index != c_brush_maxFaces) {
					const Winding& winding = getFace().getWinding();
					std::size_t adjacent = winding.next(index);
					functor(winding[index].vertex.mid(winding[adjacent].vertex));
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

	class RenderablePointVectorPushBack {
		RenderablePointVector& m_points;
	public:
		RenderablePointVectorPushBack(RenderablePointVector& points) : m_points(points) {}
		void operator()(const Vector3& point) const {
			const Colour4b colour_selected(0, 0, 255, 255);
			m_points.push_back(VertexCb(point, colour_selected));
		}
	};

	void iterate_selected(RenderablePointVector& points) const;

	bool intersectVolume(const VolumeTest& volume) const;
	bool intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const;

	/**
     * \brief
     * Submit renderable geometry to a RenderableCollector (without l2w transform).
     */
	void submitRenderables(RenderableCollector& collector,
                           const VolumeTest& volume,
						   const IRenderEntity& entity) const;

    /**
     * \brief
     * Submit renderable geometry to a RenderableCollector (with l2w transform).
     */
	void submitRenderables(RenderableCollector& collector,
                           const VolumeTest& volume,
                           const Matrix4& localToWorld,
						   const IRenderEntity& entity) const;

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

	void addLight(const Matrix4& localToWorld, const RendererLight& light);

	bool faceIsVisible() const
	{
		return m_face->faceIsVisible();
	}

	void updateFaceVisibility();

	// greebo: Provides access to the set of selected face instances
	// This should be a replacement for the old g_SelectedFaceInstances global.
	static FaceInstanceSet& Selection();

}; // class FaceInstance

// ===========================================================================================

typedef std::vector<FaceInstance> FaceInstances;

/*class FaceInstanceSet
{
	typedef SelectionList<FaceInstance> FaceInstances;
	FaceInstances m_faceInstances;
public:
	void insert(FaceInstance& faceInstance) {
		m_faceInstances.append(faceInstance);
	}
	void erase(FaceInstance& faceInstance) {
		m_faceInstances.erase(faceInstance);
	}

	template<typename Functor>
	void foreach(Functor& functor) {
		for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
			functor(*(*i));
		}
	}

	bool empty() const {
		return m_faceInstances.empty();
	}

	FaceInstance& last() const {
		return m_faceInstances.back();
	}

	std::size_t size() const {
		return m_faceInstances.size();
	}
};*/
