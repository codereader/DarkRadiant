#pragma once

#include "math/AABB.h"

namespace map
{

// Forward decl.
struct ProcEntity;
struct ProcFace;
class ProcBrush;
typedef boost::shared_ptr<ProcBrush> ProcBrushPtr;

struct BspTreeNode;
typedef boost::shared_ptr<BspTreeNode> BspTreeNodePtr; 

struct ProcPortal;
typedef boost::shared_ptr<ProcPortal> ProcPortalPtr;

struct ProcPortal
{
	Plane3			plane;
	BspTreeNodePtr	onnode;			// NULL = outside box
	BspTreeNodePtr	nodes[2];		// [0] = front side of plane
	ProcPortalPtr	next[2];
	ProcWinding		winding;

	ProcPortal() :
		plane(0,0,0,0)
	{}
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

	typedef std::vector<ProcBrushPtr> Brushes;
	Brushes				brushlist;	// fragments of all brushes in this leaf
									// needed for FindSideForPortal

	int					area;		// determined by flood filling up to areaportals
	int					occupied;	// 1 or greater can reach entity
	ProcEntity*			occupant;	// for leak file testing

	ProcPortalPtr 		portals;	// also on nodes during constructions

	BspTreeNode() :
		planenum(0),
		parent(NULL),
		side(NULL),
		nodeNumber(0),
		opaque(false),
		area(0),
		occupied(0),
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

} // namespace
