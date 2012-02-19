#pragma once

#include "math/AABB.h"

namespace map
{

const std::size_t PLANENUM_LEAF = std::numeric_limits<std::size_t>::max();

// Forward decl.
struct ProcEntity;
typedef boost::shared_ptr<ProcEntity> ProcEntityPtr;

struct ProcFace;
class ProcBrush;
typedef boost::shared_ptr<ProcBrush> ProcBrushPtr;

struct BspTreeNode;
typedef boost::shared_ptr<BspTreeNode> BspTreeNodePtr; 

struct ProcPortal;
typedef boost::shared_ptr<ProcPortal> ProcPortalPtr;

struct ProcPortal
{
	std::size_t		portalId;
	Plane3			plane;
	BspTreeNodePtr	onnode;			// NULL = outside box
	BspTreeNodePtr	nodes[2];		// [0] = front side of plane
	ProcPortalPtr	next[2];		// greebo: these form a linked list of portals
									// next[0] belongs to the linked list of the front node
	ProcWinding		winding;

	static std::size_t nextPortalId;

	ProcPortal() :
		portalId(nextPortalId++),
		plane(0,0,0,0)
	{}

	ProcPortal(const ProcPortal& other) :
		portalId(nextPortalId++),
		plane(other.plane),
		onnode(other.onnode),
		winding(other.winding)
	{
		nodes[0] = other.nodes[0];
		nodes[1] = other.nodes[1];

		next[0] = other.next[0];
		next[1] = other.next[1];
	}
};

struct BspTreeNode
{
	// both leafs and nodes
	std::size_t			planenum;	// PLANENUM_LEAF = leaf node (== std::size_t::max)
	BspTreeNode*		parent;
	AABB				bounds;		// valid after portalization
	std::size_t			nodeId;

	// nodes only
	ProcFace*			side;		// the side that created the node (use raw pointer here?)
	BspTreeNodePtr		children[2];

	int					nodeNumber;	// set after pruning

	// leafs only
	bool				opaque;		// view can never be inside

	typedef std::vector<ProcBrushPtr> Brushes;
	Brushes				brushlist;	// fragments of all brushes in this leaf
									// needed for FindSideForPortal

	std::size_t			area;		// determined by flood filling up to areaportals
	int					occupied;	// 1 or greater can reach entity
	ProcEntityPtr		occupant;	// for leak file testing

	ProcPortalPtr 		portals;	// also on nodes during constructions

	static std::size_t	nextNodeId;

	BspTreeNode() :
		planenum(0),
		parent(NULL),
		nodeId(nextNodeId++),
		side(NULL),
		nodeNumber(0),
		opaque(false),
		area(0),
		occupied(0)
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
