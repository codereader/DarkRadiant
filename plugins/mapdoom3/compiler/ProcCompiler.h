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

	struct BspFace
	{
		int					planenum;
		bool				portal;			// all portals will be selected before
											// any non-portals
		bool				checked;		// used by SelectSplitPlaneNum()
		ProcWinding			w;
	};

	struct BspTreeNode;
	typedef boost::shared_ptr<BspTreeNode> BspTreeNodePtr; 

	struct BspTreeNode
	{
		// both leafs and nodes
		int					planenum;	// -1 = leaf node
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
		BspTreeNode		outside;
		AABB			bounds;

		std::size_t		numFaceLeafs;

		BspTree() :
			numFaceLeafs(0)
		{}
	};

	BspTree _bspTree;

	typedef std::vector<BspFace> BspFaces;
	BspFaces _bspFaces;

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
};

} // namespace
