#ifndef BRUSH_BRUSH_H_
#define BRUSH_BRUSH_H_

#include "scenelib.h"
#include "cullable.h"
#include "editable.h"

#include "container/array.h"

#include "Face.h"
#include "SelectableComponents.h"
#include "RenderableWireFrame.h"

const std::size_t c_brush_maxFaces = 1024;

typedef std::size_t faceIndex_t;

inline double quantiseFloating(double f) {
	return float_snapped(f, 1.f / (1 << 16));
}

/// \brief Returns true if 'self' takes priority when building brush b-rep.
inline bool plane3_inside(const Plane3& self, const Plane3& other) {
	if (vector3_equal_epsilon(self.normal(), other.normal(), 0.001)) {
		return self.dist() < other.dist();
	}
	return true;
}

/// \brief Returns the unique-id of the edge adjacent to \p faceVertex in the edge-pair for the set of \p faces.
inline FaceVertexId next_edge(const Faces& faces, FaceVertexId faceVertex) {
	std::size_t adjacent_face = faces[faceVertex.getFace()]->getWinding()[faceVertex.getVertex()].adjacent;
	std::size_t adjacent_vertex = faces[adjacent_face]->getWinding().findAdjacent(faceVertex.getFace());

	ASSERT_MESSAGE(adjacent_vertex != c_brush_maxFaces, "connectivity data invalid");
	
	if (adjacent_vertex == c_brush_maxFaces) {
		return faceVertex;
	}

	return FaceVertexId(adjacent_face, adjacent_vertex);
}

/// \brief Returns the unique-id of the vertex adjacent to \p faceVertex in the vertex-ring for the set of \p faces.
inline FaceVertexId next_vertex(const Faces& faces, FaceVertexId faceVertex) {
	FaceVertexId nextEdge = next_edge(faces, faceVertex);
	return FaceVertexId(nextEdge.getFace(), faces[nextEdge.getFace()]->getWinding().next(nextEdge.getVertex()));
}

struct EdgeFaces {
	faceIndex_t first;
	faceIndex_t second;

	EdgeFaces()
		: first(c_brush_maxFaces), second(c_brush_maxFaces)
	{}

	EdgeFaces(const faceIndex_t _first, const faceIndex_t _second)
		: first(_first), second(_second)
	{}
};

class BrushObserver {
public:
	virtual void reserve(std::size_t size) = 0;
	virtual void clear() = 0;
	virtual void push_back(Face& face) = 0;
	virtual void pop_back() = 0;
	virtual void erase(std::size_t index) = 0;
	virtual void connectivityChanged() = 0;
	
	virtual void edge_clear() = 0;
	virtual void edge_push_back(SelectableEdge& edge) = 0;
	
	virtual void vertex_clear() = 0;
	virtual void vertex_push_back(SelectableVertex& vertex) = 0;
	
	virtual void DEBUG_verify() = 0;
};

class BrushVisitor {
public:
	virtual void visit(Face& face) const = 0;
};

class Brush :
	public TransformNode,
	public Bounded,
	public Cullable,
	public Snappable,
	public Undoable,
	public FaceObserver,
	public BrushDoom3
{
private:
	scene::Node* m_node;
	typedef UniqueSet<BrushObserver*> Observers;
	Observers m_observers;
	UndoObserver* m_undoable_observer;
	MapFile* m_map;
	
	// state
	Faces m_faces;
	// ----
	
	// cached data compiled from state
	Array<PointVertex> m_faceCentroidPoints;
	RenderablePointArray m_render_faces;
	
	Array<PointVertex> m_uniqueVertexPoints;
	typedef std::vector<SelectableVertex> SelectableVertices;
	SelectableVertices m_select_vertices;
	RenderablePointArray m_render_vertices;
	
	Array<PointVertex> m_uniqueEdgePoints;
	typedef std::vector<SelectableEdge> SelectableEdges;
	SelectableEdges m_select_edges;
	RenderablePointArray m_render_edges;
	
	Array<EdgeRenderIndices> m_edge_indices;
	Array<EdgeFaces> m_edge_faces;
	
	AABB m_aabb_local;
	// ----
	
	Callback m_evaluateTransform;
	Callback m_boundsChanged;
	
	mutable bool m_planeChanged; // b-rep evaluation required
	mutable bool m_transformChanged; // transform evaluation required
	// ----

public:  
	/// \brief The undo memento for a brush stores only the list of face references - the faces are not copied.
	class BrushUndoMemento : public UndoMemento {
	public:
		BrushUndoMemento(const Faces& faces) : m_faces(faces) {}
		
		void release() {
			delete this;
		}
	
		Faces m_faces;
	};
	
	Callback m_lightsChanged;
	
	// static data
	static ShaderPtr m_state_point;
	// ----
	
	static double m_maxWorldCoord;
	
	// Constructors
	Brush(scene::Node& node, const Callback& evaluateTransform, const Callback& boundsChanged);
	Brush(const Brush& other, scene::Node& node, const Callback& evaluateTransform, const Callback& boundsChanged);
	
	// Copy Constructor
	Brush(const Brush& other);
	
	// Destructor
	~Brush();
	
	// assignment not supported
	Brush& operator=(const Brush& other);
	
	/** greebo: This translates the brush about the given translation vector,
	 * this is used by the Doom3Group entity to add/substract the origin from
	 * their child brushes. The translation is TextureLock-sensitive.
	 */
	void translateDoom3Brush(const Vector3& translation);
	
	void attach(BrushObserver& observer);
	void detach(BrushObserver& observer);
	
	void forEachFace(const BrushVisitor& visitor) const;
	
	void forEachFace_instanceAttach(MapFile* map) const;
	void forEachFace_instanceDetach(MapFile* map) const;
	
	InstanceCounter m_instanceCounter;
	
	void instanceAttach(const scene::Path& path);
	void instanceDetach(const scene::Path& path);
	
	// observer
	void planeChanged();
	void shaderChanged();
	
	// Sets the shader of all faces to the given name
	void setShader(const std::string& newShader);

	void evaluateBRep() const;
	
	void transformChanged();
	typedef MemberCaller<Brush, &Brush::transformChanged> TransformChangedCaller;
	
	void evaluateTransform();
	
	const Matrix4& localToParent() const;
	
	void aabbChanged();
	
	const AABB& localAABB() const;
	
	VolumeIntersectionValue intersectVolume(const VolumeTest& test, const Matrix4& localToWorld) const;
	
	void renderComponents(SelectionSystem::EComponentMode mode, Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;
	
	void transform(const Matrix4& matrix);
	
	void snapto(float snap);
	
	void revertTransform();
	void freezeTransform();
	
	/// \brief Returns the absolute index of the \p faceVertex.
	std::size_t absoluteIndex(FaceVertexId faceVertex);
	
	void appendFaces(const Faces& other);

	void undoSave();
	UndoMemento* exportState() const;
	void importState(const UndoMemento* state);
	
	/// \brief Appends a copy of \p face to the end of the face list.
	FacePtr addFace(const Face& face);

	/// \brief Appends a new face constructed from the parameters to the end of the face list.
	FacePtr addPlane(const Vector3& p0, const Vector3& p1, const Vector3& p2, const std::string& shader, const TextureProjection& projection);
	
	static void constructStatic();
	static void destroyStatic();

	std::size_t DEBUG_size();

	typedef Faces::const_iterator const_iterator;

	const_iterator begin() const;
	const_iterator end() const;

	FacePtr back();
	const FacePtr back() const;
	
	void reserve(std::size_t count);
	
	void push_back(Faces::value_type face);
	
	void pop_back();
	void erase(std::size_t index);
	
	void connectivityChanged();

	void clear();
	
	std::size_t size() const;
	
	bool empty() const;

	/// \brief Returns true if any face of the brush contributes to the final B-Rep.
	bool hasContributingFaces() const;

	/// \brief Removes faces that do not contribute to the brush. This is useful for cleaning up after CSG operations on the brush.
	/// Note: removal of empty faces is not performed during direct brush manipulations, because it would make a manipulation irreversible if it created an empty face.
	void removeEmptyFaces();

	/// \brief Constructs \p winding from the intersection of \p plane with the other planes of the brush.
	void windingForClipPlane(Winding& winding, const Plane3& plane) const;

	void update_wireframe(RenderableWireframe& wire, const bool* faces_visible) const;

	void update_faces_wireframe(Array<PointVertex>& wire, const bool* faces_visible) const;

	/// \brief Makes this brush a deep-copy of the \p other.
	void copy(const Brush& other);

private:
	void edge_push_back(FaceVertexId faceVertex);
	
	void edge_clear();
	
	void vertex_push_back(FaceVertexId faceVertex);
	
	void vertex_clear();

	/// \brief Returns true if the face identified by \p index is preceded by another plane that takes priority over it.
	bool plane_unique(std::size_t index) const;

	/// \brief Removes edges that are smaller than the tolerance used when generating brush windings.
	void removeDegenerateEdges();

	/// \brief Invalidates faces that have only two vertices in their winding, while preserving edge-connectivity information.
	void removeDegenerateFaces();

	/// \brief Removes edges that have the same adjacent-face as their immediate neighbour.
	void removeDuplicateEdges();

	/// \brief Removes edges that do not have a matching pair in their adjacent-face.
	void verifyConnectivityGraph();

	/// \brief Returns true if the brush is a finite volume. A brush without a finite volume extends past the maximum world bounds and is not valid.
	bool isBounded();

	/// \brief Constructs the polygon windings for each face of the brush. Also updates the brush bounding-box and face texture-coordinates.
	bool buildWindings();

	/// \brief Constructs the face windings and updates anything that depends on them.
	void buildBRep();
}; // class Brush

typedef std::vector<Brush*> BrushVector;

/**
 * Stream insertion for Brush objects.
 */
inline std::ostream& operator<< (std::ostream& os, const Brush& b) {
    os << "Brush { size = " << b.size() << ", localAABB = " << b.localAABB()
       << " }";
    return os;
}

#endif /*BRUSH_BRUSH_H_*/
