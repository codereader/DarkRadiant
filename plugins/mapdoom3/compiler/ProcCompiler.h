#pragma once

#include "inode.h"
#include "ientity.h"
#include "ProcFile.h"
#include "BspTree.h"
#include "math/Vector3.h"
#include "LeakFile.h"
#include "TriangleHash.h"

namespace map
{

/** 
 * Processor class that takes a root node (representing a map)
 * to generate a ProcFile containing the model and shadow data.
 */
class ProcCompiler
{
private:
	// The map node we're processing
	scene::INodePtr _root;

	// The working copy
	ProcFilePtr _procFile;

	struct BspFace
	{
		int					planenum;
		bool				portal;			// all portals will be selected before
											// any non-portals
		bool				checked;		// used by selectSplitPlaneNum()
		ProcWinding			w;

		BspFace() :
			planenum(0),
			portal(false),
			checked(false)
		{}
	};
	typedef boost::shared_ptr<BspFace> BspFacePtr;

	typedef std::vector<BspFacePtr> BspFaces;
	BspFaces _bspFaces;

	std::size_t _numActivePortals;
	std::size_t _numPeakPortals;
	std::size_t _numTinyPortals;

	std::size_t _numUniqueBrushes;
	std::size_t _numClusters;

	std::size_t _numFloodedLeafs;

	std::size_t _numOutsideLeafs;
	std::size_t _numInsideLeafs;
	std::size_t _numSolidLeafs;

	std::size_t _numAreas;
	std::size_t _numAreaFloods;

	TriangleHashPtr _triangleHash;

	AABB		_optBounds;

	typedef std::vector<OriginalEdge> OriginalEdges;
	OriginalEdges	_originalEdges;

	typedef std::vector<OptEdge> OptEdges;
	OptEdges		_optEdges;

	typedef std::vector<OptVertex> OptVertices;
	OptVertices		_optVerts;

	enum ShadowGenType
	{
		SG_DYNAMIC,		// use infinite projections
		SG_STATIC,		// clip to bounds
		SG_OFFLINE		// perform very time consuming optimizations
	};

	std::size_t _numShadowIndices;
	std::size_t _numShadowVerts;
	std::size_t _numClipSilEdges;
	bool _overflowed;
	std::vector<Vector4> _shadowVerts;
	std::vector<std::size_t> _shadowIndices;

#define	MAX_CLIP_SIL_EDGES		2048

	int	_clipSilEdges[MAX_CLIP_SIL_EDGES][2];

#define	MAX_CLIPPED_POINTS	20

	struct ClipTri
	{
		int		numVerts;
		Vector3	verts[MAX_CLIPPED_POINTS];
		int		edgeFlags[MAX_CLIPPED_POINTS];
	};

	struct IndexRef 
	{
		std::size_t	frontCapStart;
		std::size_t	rearCapStart;
		std::size_t	silStart;
		std::size_t	end;
	};
	
	IndexRef _indexRef[6];
	std::size_t _indexFrustumNumber;		// which shadow generating side of a light the indexRef is for

public:
	ProcCompiler(const scene::INodePtr& root);

	// Generate the .proc file
	ProcFilePtr generateProcFile();

private:
	void generateBrushData();

	bool processModels();
	bool processModel(ProcEntity& entity, bool floodFill);

	// Create a list of all faces that are relevant for faceBSP()
	void makeStructuralProcFaceList(const ProcEntity::Primitives& primitives);

	// Sort all structural faces into a bsp tree
	void faceBsp(ProcEntity& entity);

	// Split the given face list, and assign them to node->children[], then enter recursion
	// The given face list will be emptied before returning
	void buildFaceTreeRecursively(const BspTreeNodePtr& node, BspFaces& faces, BspTree& tree);

	std::size_t selectSplitPlaneNum(const BspTreeNodePtr& node, BspFaces& list);

	void makeTreePortals(BspTree& tree);

	void makeHeadNodePortals(BspTree& tree);
	void makeTreePortalsRecursively(const BspTreeNodePtr& node);

	// create the new portal by taking the full plane winding for the cutting plane
	// and clipping it by all of parents of this node
	void makeNodePortal(const BspTreeNodePtr& node);
	void splitNodePortals(const BspTreeNodePtr& node);

	// Mark the leafs as opaque and areaportals and put brush
	// fragments in each leaf so portal surfaces can be matched to materials
	void filterBrushesIntoTree(ProcEntity& entity);

	// Generates two new brushes, leaving the original unchanged
	void splitBrush(const ProcBrushPtr& brush, std::size_t planenum, ProcBrushPtr& front, ProcBrushPtr& back);
	std::size_t filterBrushIntoTreeRecursively(const ProcBrushPtr& brush, const BspTreeNodePtr& node);

	float calculateBrushVolume(const ProcBrushPtr& brush);

	bool floodEntities(BspTree& tree);
	bool placeOccupant(const BspTreeNodePtr& node, const Vector3& origin, const ProcEntityPtr& entity);
	void floodPortalsRecursively(const BspTreeNodePtr& node, int dist);

	// Fill (set node->opaque = true) all nodes that can't be reached by entities
	void fillOutside(const ProcEntity& entity);
	void fillOutsideRecursively(const BspTreeNodePtr& node);

	ProcWinding getBaseWindingForNode(const BspTreeNodePtr& node);

	void calculateNodeBounds(const BspTreeNodePtr& node);
	void addPortalToNodes(const ProcPortalPtr& portal, const BspTreeNodePtr& front, const BspTreeNodePtr& back);
	void removePortalFromNode(const ProcPortalPtr& portal, const BspTreeNodePtr& node);

	/**
	 * Creates side->visibleHull for all visible sides
	 * The visible hull for a side will consist of the convex hull of all points 
	 * in non-opaque clusters, which allows overlaps to be trimmed off automatically.
	 */
	void clipSidesByTree(ProcEntity& entity);

	// Adds non-opaque leaf fragments to the convex hull
	void clipSideByTreeRecursively(ProcWinding& winding, ProcFace& side, const BspTreeNodePtr& node);

	// Mark each leaf with an area, bounded by Material::SURF_AREAPORTAL
	// sets entity.areas
	void floodAreas(ProcEntity& entity);
	
	// Set all the areas to -1 before filling
	void clearAreasRecursively(const BspTreeNodePtr& node);

	// Just decend the tree, and for each node that hasn't had an area set, flood fill out from there
	void findAreasRecursively(const BspTreeNodePtr& node);

	void floodAreasRecursively(const BspTreeNodePtr& node);

	ProcFace* findSideForPortal(const ProcPortalPtr& portal);
	void checkAreasRecursively(const BspTreeNodePtr& node);
	void findInterAreaPortalsRecursively(const BspTreeNodePtr& node);

	// returns true if the portal has non-opaque leafs on both sides
	static bool portalIsPassable(const ProcPortal& portal);

	void putPrimitivesInAreas(ProcEntity& entity);

	// Clips a winding down into the bsp tree, then converts
	// the fragments to triangles and adds them to the area lists
	void putWindingIntoAreasRecursively(ProcEntity& entity, const ProcWinding& winding, 
										ProcFace& side, const BspTreeNodePtr& node);

	// Returns the area number that the winding is in, or MULTIAREA_CROSS if it crosses multiple areas.
	// Empty windings are not allowed!
	std::size_t checkWindingInAreasRecursively(const ProcWinding& winding, const BspTreeNodePtr& node);

	ProcTris triangleListForSide(const ProcFace& side, const ProcWinding& winding);

	// The triList is appended to the apropriate optimzeGroup_t,
	// creating a new one if needed.
	// The entire list is assumed to come from the same planar primitive
	void addTriListToArea(ProcEntity& entity, const ProcTris& triList, 
						  std::size_t planeNum, std::size_t areaNum, 
						  Vector4 texVec[2]);

	void addMapTrisToAreas(const ProcTris& tris, ProcEntity& e);

	void clipTriIntoTreeRecursively(const ProcWinding& winding, const ProcTri& originalTri, 
								  ProcEntity& entity, const BspTreeNodePtr& node);

	// Break optimize groups up into additional groups at light boundaries, so
	// optimization won't cross light bounds
	void preLight(ProcEntity& entity);
	void boundOptimizeGroup(ProcOptimizeGroup& group);

	// Build the beam tree and shadow volume surface for a light
	void buildLightShadows(ProcEntity& entity, ProcLight& light);
	void clipTriByLight(const ProcLight& light, const ProcTri& tri, ProcTris& in, ProcTris& out);

	// shadowerGroups should be exactly clipped to the light frustum before calling.
	// shadowerGroups is optimized by this function, but the contents can be freed, because the returned
	// lightShadow_t list is a further culling and optimization of the data.
	Surface createLightShadow(ProcArea::OptimizeGroups& shadowerGroups, const ProcLight& light);

	// This will also fix tjunctions
	void optimizeGroupList(ProcArea::OptimizeGroups& groupList);
	std::size_t countGroupListTris(ProcArea::OptimizeGroups& groupList);
	void optimizeOptList(ProcOptimizeGroup& group);
	void fixAreaGroupsTjunctions(ProcArea::OptimizeGroups& groups);

	// removes triangles that are degenerated or flipped backwards
	void hashTriangles(ProcArea::OptimizeGroups& groups);
	void addOriginalEdges(ProcOptimizeGroup& group);

	OptVertex* findOptVertex(const ArbitraryMeshVertex& vertex, ProcOptimizeGroup& group);
	void addOriginalTriangle(OptVertex* v[3]);
	void splitOriginalEdgesAtCrossings(ProcOptimizeGroup& group);

	// Creates a new OptVertex where the line segments cross.
	// this should only be called if PointsStraddleLine returned true
	// will return NULL if the lines are colinear
	OptVertex* getEdgeIntersection(const OptVertex* p1, const OptVertex* p2,
						const OptVertex* l1, const OptVertex* l2, ProcOptimizeGroup& opt);

	void addEdgeIfNotAlready(OptVertex* v1, OptVertex* v2);

	void dontSeparateIslands(ProcOptimizeGroup& group);

	// Copies the group planeNum to every triangle in each group
	void setGroupTriPlaneNums(ProcArea::OptimizeGroups& groupList);

	Surface shareMapTriVerts(const ProcTris& tris);

	/*
	 * The returned surface will have a valid bounds and radius for culling.
	 * 
	 * Triangles are clipped to the light frustum before projecting.
	 * 
	 * A single triangle can clip to as many as 7 vertexes, so
	 * the worst case expansion is 2*(numindexes/3)*7 verts when counting both
	 * the front and back caps, although it will usually only be a modest
	 * increase in vertexes for closed modesl
	 * 
	 * The worst case index count is much larger, when the 7 vertex clipped triangle
	 * needs 15 indexes for the front, 15 for the back, and 42 (a quad on seven sides)
	 * for the sides, for a total of 72 indexes from the original 3.  Ouch.
	 * 
	 * NULL may be returned if the surface doesn't create a shadow volume at all,
	 * as with a single face that the light is behind.
	 * 
	 * If an edge is within an epsilon of the border of the volume, it must be treated
	 * as if it is clipped for triangles, generating a new sil edge, and act
	 * as if it was culled for edges, because the sil edge will have been
	 * generated by the triangle irregardless of if it actually was a sil edge.
	*/
	Surface createShadowVolume(const Matrix4& transform, const Surface& tri, const ProcLight& light,
							 ShadowGenType optimize, Surface::CullInfo& cullInfo);

	// Stubs, not implemented yet
	Surface createVertexProgramTurboShadowVolume(const Matrix4& transform, const Surface& tri, 
								const ProcLight& light, Surface::CullInfo& cullInfo);
	Surface createTurboShadowVolume(const Matrix4& transform, const Surface& tri, 
								const ProcLight& light, Surface::CullInfo& cullInfo);

	// Determines which triangles of the surface are facing towards the light origin.
	// The facing array should be allocated with one extra index than
	// the number of surface triangles, which will be used to handle dangling
	void calcInteractionFacing(const Matrix4& transform, const Surface& tri, const ProcLight& light,
							 Surface::CullInfo& cullInfo);

	// Adds new verts and indexes to the shadow volume.
	// 
	// If the frustum completely defines the projected light,
	// makeClippedPlanes should be true, which will cause sil quads to
	// be added along all clipped edges.
	// 
	// If the frustum is just part of a point light, clipped planes don't
	// need to be added.
	void createShadowVolumeInFrustum(const Matrix4& transform, const Surface& tri,
									const ProcLight& light, const Vector3& lightOrigin, const Plane3 frustum[6],
									const Plane3 &farPlane, bool makeClippedPlanes, int* remap, 
									unsigned char* faceCastsShadow, std::vector<unsigned char>& globalFacing);

	// Also inits the remap[] array to all -1
	void calcPointCull(const Surface& tri, const Plane3 frustum[6], unsigned short* pointCull, int* remap);

	bool clipTriangleToLight(const Vector3& a, const Vector3& b, const Vector3& c, int planeBits, const Plane3 frustum[6]);

	// Clips a triangle from one buffer to another, setting edge flags
	// The returned buffer may be the same as inNum if no clipping is done
	// If entirely clipped away, clipTris[returned].numVerts == 0
	// 
	// I have some worries about edge flag cases when polygons are clipped
	// multiple times near the epsilon.
	int chopWinding(ClipTri clipTris[2], int inNum, const Plane3& plane);

	// Add sil edges for each triangle clipped to the side of the frustum.
	// Only done for simple projected lights, not point lights.
	void addClipSilEdges();

	// Add quads from the front points to the projected points
	// for each silhouette edge in the light
	void addSilEdges(const Surface& tri, unsigned short* pointCull, const Plane3 frustum[6], 
					 int* remap, unsigned char* faceCastsShadow);

	// If neither point is clearly behind the clipping
	// plane, the edge will be passed unmodified.  A sil edge that
	// is on a border plane must be drawn.
	// 
	// If one point is clearly clipped by the plane and the
	// other point is on the plane, it will be completely removed.
	bool clipLineToLight(const Vector3& a, const Vector3& b, const Plane3 frustum[4], Vector3& p1, Vector3& p2);

	// make a projected copy of the even verts into the odd spots
	// that is on the far light clip plane
	void projectPointsToFarPlane(const Matrix4& transform, const ProcLight& light, 
								const Plane3& lightPlaneLocal, std::size_t firstShadowVert, std::size_t numShadowVerts);

	void optimizeEntity(ProcEntity& entity);

	void fixGlobalTjunctions(ProcEntity& entity);

	// Any nodes that have all children with the same
	// area can be combined into a single leaf node
	//
	// Returns the area number of all children, or
	// AREANUM_DIFFERENT if not the same.
	std::size_t pruneNodesRecursively(const BspTreeNodePtr& node);
	void freeTreePortalsRecursively(const BspTreeNodePtr& node);
};

} // namespace
