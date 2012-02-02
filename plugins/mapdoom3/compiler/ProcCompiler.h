#pragma once

#include "inode.h"
#include "ientity.h"
#include "ProcFile.h"
#include "BspTree.h"
#include "math/Vector3.h"
#include "LeakFile.h"

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
};

} // namespace
