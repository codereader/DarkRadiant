#pragma once

#include "inode.h"
#include "ientity.h"
#include "ProcFile.h"
#include "math/Vector3.h"

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

	struct ProcPortal;
	typedef boost::shared_ptr<ProcPortal> ProcPortalPtr;

	struct BspFace
	{
		int					planenum;
		bool				portal;			// all portals will be selected before
											// any non-portals
		bool				checked;		// used by SelectSplitPlaneNum()
		ProcWinding			w;
	};
	typedef boost::shared_ptr<BspFace> BspFacePtr;

	struct BspTreeNode;
	typedef boost::shared_ptr<BspTreeNode> BspTreeNodePtr; 

	struct ProcPortal
	{
		Plane3			plane;
		BspTreeNodePtr	onnode;			// NULL = outside box
		BspTreeNodePtr	nodes[2];		// [0] = front side of plane
		ProcPortalPtr	next[2];
		ProcWinding		winding;
	};

	struct BspTreeNode
	{
		// both leafs and nodes
		std::size_t			planenum;	// PLANENUM_LEAF = leaf node (== std::size_t::max)
		BspTreeNode*		parent;
		AABB				bounds;		// valid after portalization

		// nodes only
		ProcFace*			side;		// the side that created the node (use raw pointer here?)
		BspTreeNodePtr		children[2];

		int					nodeNumber;	// set after pruning

		// leafs only
		bool				opaque;		// view can never be inside

		//uBrush_t *			brushlist;	// fragments of all brushes in this leaf
										// needed for FindSideForPortal

		int					area;		// determined by flood filling up to areaportals
		int					occupied;	// 1 or greater can reach entity
		ProcEntity*			occupant;	// for leak file testing

		//struct uPortal_s *	portals;	// also on nodes during construction

		BspTreeNode() :
			parent(NULL),
			occupant(NULL)
		{}
	};

	struct BspTree
	{
		BspTreeNodePtr	head;
		BspTreeNodePtr	outside;
		AABB			bounds;

		std::size_t		numFaceLeafs;

		BspTree() :
			outside(new BspTreeNode),
			numFaceLeafs(0)
		{}
	};

	BspTree _bspTree;

	typedef std::vector<BspFacePtr> BspFaces;
	BspFaces _bspFaces;

	std::size_t _numActivePortals;
	std::size_t _numPeakPortals;

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
	void buildFaceTreeRecursively(const BspTreeNodePtr& node, BspFaces& faces);

	std::size_t selectSplitPlaneNum(const BspTreeNodePtr& node, BspFaces& list);

	void makeTreePortals();

	void makeHeadNodePortals();

	void addPortalToNodes(const ProcPortalPtr& portal, const BspTreeNodePtr& front, const BspTreeNodePtr& back);
};

} // namespace
