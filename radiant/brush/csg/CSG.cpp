#include "CSG.h"

#include <map>

#include "iundo.h"
#include "igrid.h"
#include "iradiant.h"
#include "iselection.h"
#include "scenelib.h"
#include "shaderlib.h"

#include "brush/Face.h"
#include "brush/Brush.h"
#include "brush/BrushNode.h"
#include "brush/BrushVisit.h"
#include "selection/algorithm/Primitives.h"

#include "gtkutil/dialog.h"

#include "BrushByPlaneClipper.h"

namespace brush {
namespace algorithm {

void Face_makeBrush(Face& face, const Brush& brush, BrushVector& out, float offset, bool makeRoom) {
	if (!face.contributes()) {
		return;
	}

	out.push_back(new Brush(brush));

	FacePtr newFace = out.back()->addFace(face);

	if (newFace != 0) {
		newFace->flipWinding();
		newFace->getPlane().offset(offset);
		newFace->planeChanged();
  
		if (makeRoom) {
			// Retrieve the normal vector of the "source" face
			out.back()->transform(
				Matrix4::getTranslation(face.getPlane().plane3().normal()*offset)
			);
			out.back()->freezeTransform();
		}
	}
}

class FaceMakeBrush
{
	const Brush& _brush;
	BrushVector& _out;
	float _offset;
	bool _makeRoom;
public:
	FaceMakeBrush(const Brush& brush, BrushVector& out, float offset, bool makeRoom = false) : 
		_brush(brush), 
		_out(out), 
		_offset(offset), 
		_makeRoom(makeRoom)
	{}

	void operator()(Face& face) const {
		Face_makeBrush(face, _brush, _out, _offset, _makeRoom);
	}
};

void hollowBrush(const BrushNodePtr& sourceBrush, bool makeRoom) {
	// Create the brushes
	BrushVector out;

	// Hollow the brush using the current grid size
	Brush_forEachFace(
		sourceBrush->getBrush(), 
		FaceMakeBrush(sourceBrush->getBrush(), out, GlobalGrid().getGridSize(), makeRoom)
	);

	scene::INodePtr parent = sourceBrush->getParent();

	for (BrushVector::const_iterator i = out.begin(); i != out.end(); ++i) {
		(*i)->removeEmptyFaces();

		scene::INodePtr newNode = GlobalBrushCreator().createBrush();
		BrushNodePtr brushNode = boost::dynamic_pointer_cast<BrushNode>(newNode);
		assert(brushNode != NULL);

		// Move the child brushes to the same layer as their source
		scene::assignNodeToLayers(brushNode, sourceBrush->getLayers());

		brushNode->getBrush().copy(*(*i));

		delete (*i);
		
		// Add the child to the same parent as the source brush
		parent->addChildNode(brushNode);

		Node_setSelected(brushNode, true);
	}

	// Now unselect and remove the source brush from the scene
	scene::removeNodeFromParent(sourceBrush);
}

void hollowSelectedBrushes() {
	UndoableCommand undo("hollowSelectedBrushes");

	// Find all brushes
	BrushPtrVector brushes = selection::algorithm::getSelectedBrushes();

	// Cycle through the brushes and hollow them
	// We assume that all these selected brushes are visible as well.
	for (std::size_t i = 0; i < brushes.size(); i++) {
		hollowBrush(brushes[i], false);
	}

	SceneChangeNotify();
}

void makeRoomForSelectedBrushes() {
	UndoableCommand undo("brushMakeRoom");

	// Find all brushes
	BrushPtrVector brushes = selection::algorithm::getSelectedBrushes();

	// Cycle through the brushes and hollow them
	// We assume that all these selected brushes are visible as well.
	for (std::size_t i = 0; i < brushes.size(); i++) {
		hollowBrush(brushes[i], true);
	}

	SceneChangeNotify();
}

BrushSplitType Brush_classifyPlane(const Brush& brush, const Plane3& plane) {
	brush.evaluateBRep();
	
	BrushSplitType split;
	for (Brush::const_iterator i(brush.begin()); i != brush.end(); ++i) {
		if ((*i)->contributes()) {
			split += (*i)->getWinding().classifyPlane(plane);
		}
	}

	return split;
}

bool Brush_subtract(const Brush& brush, const Brush& other, BrushVector& ret_fragments) {
	if (aabb_intersects_aabb(brush.localAABB(), other.localAABB())) {
		BrushVector fragments;
		fragments.reserve(other.size());
		Brush back(brush);

		for (Brush::const_iterator i(other.begin()); i != other.end(); ++i) {
			if ((*i)->contributes()) {
				BrushSplitType split = Brush_classifyPlane(back, (*i)->plane3());
				
				if (split.counts[ePlaneFront] != 0 && split.counts[ePlaneBack] != 0) {
					fragments.push_back(new Brush(back));
					FacePtr newFace = fragments.back()->addFace(*(*i));
					if (newFace != 0) {
						newFace->flipWinding();
					}

					back.addFace(*(*i));
				}
				else if(split.counts[ePlaneBack] == 0) {
					for (BrushVector::iterator i = fragments.begin(); i != fragments.end(); ++i) {
						delete(*i);
					}

					return false;
				}
			}
		}

		ret_fragments.insert(ret_fragments.end(), fragments.begin(), fragments.end());
		return true;
	}

	return false;
}

class SubtractBrushesFromUnselected : 
	public scene::Graph::Walker
{
	const BrushPtrVector& _brushlist;
	std::size_t& _before;
	std::size_t& _after;

	mutable std::list<scene::INodePtr> _deleteList;
public:
	SubtractBrushesFromUnselected(const BrushPtrVector& brushlist, std::size_t& before, std::size_t& after) : 
		_brushlist(brushlist), 
		_before(before), 
		_after(after)
	{}

	~SubtractBrushesFromUnselected() {
		for (std::list<scene::INodePtr>::iterator i = _deleteList.begin();
			 i != _deleteList.end(); i++)
		{
			scene::removeNodeFromParent(*i);
		}
	}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		return true;
	}

	void post(const scene::Path& path, const scene::INodePtr& node) const {
		if (!node->visible()) {
			return;
		}

		Brush* brush = Node_getBrush(node);

		if (brush != NULL && !Node_isSelected(node)) {
			BrushVector buffer[2];
			bool swap = false;
			
			Brush* original = new Brush(*brush);
			buffer[static_cast<std::size_t>(swap)].push_back(original);

			for (BrushPtrVector::const_iterator i(_brushlist.begin()); i != _brushlist.end(); ++i) {

				for (BrushVector::iterator j(buffer[static_cast<std::size_t>(swap)].begin()); 
					 j != buffer[static_cast<std::size_t>(swap)].end(); ++j)
				{
					if (Brush_subtract(*(*j), (*i)->getBrush(), buffer[static_cast<std::size_t>(!swap)])) {
						delete (*j);
					}
					else {
						buffer[static_cast<std::size_t>(!swap)].push_back((*j));
					}
				}

				buffer[static_cast<std::size_t>(swap)].clear();
				swap = !swap;
			}

			BrushVector& out = buffer[static_cast<std::size_t>(swap)];

			if (out.size() == 1 && out.back() == original) {
				delete original;
			}
			else {
				_before++;

				for (BrushVector::const_iterator i = out.begin(); i != out.end(); ++i) {
					_after++;
					
					scene::INodePtr newBrush = GlobalBrushCreator().createBrush();

					// Move the new Brush to the same layers as the source node
					scene::assignNodeToLayers(newBrush, node->getLayers());

					(*i)->removeEmptyFaces();
					ASSERT_MESSAGE(!(*i)->empty(), "brush left with no faces after subtract");

					Node_getBrush(newBrush)->copy(*(*i));
					delete (*i);

					path.parent()->addChildNode(newBrush);
				}

			    _deleteList.push_back(node);
			}
		}
	}
};

void subtractBrushesFromUnselected() {
	// Collect all selected brushes
	BrushPtrVector brushes = selection::algorithm::getSelectedBrushes();
	
	if (brushes.empty()) {
		globalOutputStream() << "CSG Subtract: No brushes selected.\n";
		gtkutil::errorDialog("CSG Subtract: No brushes selected.", GlobalRadiant().getMainWindow());
		return;
	}

	globalOutputStream() << "CSG Subtract: Subtracting " << static_cast<int>(brushes.size()) << " brushes.\n";

	UndoableCommand undo("brushSubtract");

	// subtract selected from unselected
	std::size_t before = 0;
	std::size_t after = 0;

	// instantiate a scoped walker class
	{
		SubtractBrushesFromUnselected walker(brushes, before, after);
		GlobalSceneGraph().traverse(walker);
	}

	globalOutputStream() << "CSG Subtract: Result: "
		<< static_cast<int>(after) << " fragment" << (after == 1 ? "" : "s")
		<< " from " << static_cast<int>(before) << " brush" << (before == 1 ? "" : "es") << ".\n";

	SceneChangeNotify();
}

// greebo: TODO: Make this a member method of the Brush class
bool Brush_merge(Brush& brush, const BrushPtrVector& in, bool onlyshape) {
	// gather potential outer faces 
	typedef std::vector<const Face*> Faces;
	Faces faces;

	for (BrushPtrVector::const_iterator i(in.begin()); i != in.end(); ++i) {
		(*i)->getBrush().evaluateBRep();

		for (Brush::const_iterator j((*i)->getBrush().begin()); j != (*i)->getBrush().end(); ++j) {
			if (!(*j)->contributes()) {
				continue;
			}

			const Face& face1 = *(*j);

			bool skip = false;

			// test faces of all input brushes
			//!\todo SPEEDUP: Flag already-skip faces and only test brushes from i+1 upwards.
			for (BrushPtrVector::const_iterator k(in.begin()); !skip && k != in.end(); ++k) {
				if (k != i) { // don't test a brush against itself
					for (Brush::const_iterator l((*k)->getBrush().begin()); !skip && l != (*k)->getBrush().end(); ++l) {
						const Face& face2 = *(*l);

						// face opposes another face
						if (face1.plane3() == -face2.plane3()) {
							// skip opposing planes
							skip  = true;
							break;
						}
					}
				}
			}

			// check faces already stored
			for (Faces::const_iterator m = faces.begin(); !skip && m != faces.end(); ++m) {
				const Face& face2 = *(*m);

				// face equals another face
				if (face1.plane3() == face2.plane3()) {
					// if the texture/shader references should be the same but are not
					if (!onlyshape && !shader_equal(face1.getShader().getShader(), face2.getShader().getShader())) {
						return false;
					}
				
					// skip duplicate planes
					skip = true;
					break;
				}

				// face1 plane intersects face2 winding or vice versa
				if (Winding::planesConcave(face1.getWinding(), face2.getWinding(), face1.plane3(), face2.plane3())) {
					// result would not be convex
					return false;
				}
			}

			if (!skip) {
				faces.push_back(&face1);
			}
		}
	}

	for (Faces::const_iterator i = faces.begin(); i != faces.end(); ++i) {
		if (!brush.addFace(*(*i))) {
			// result would have too many sides
			return false;
		}
	}

	brush.removeEmptyFaces();
	return true;
}

void mergeSelectedBrushes() {
	// Get the current selection
	BrushPtrVector brushes = selection::algorithm::getSelectedBrushes();

	if (brushes.empty()) {
		globalOutputStream() << "CSG Merge: No brushes selected.\n";
		gtkutil::errorDialog("CSG Merge: No brushes selected.", GlobalRadiant().getMainWindow());
		return;
	}

	if (brushes.size() < 2) {
		globalOutputStream() << "CSG Merge: At least two brushes have to be selected.\n";
		gtkutil::errorDialog("CSG Merge: At least two brushes have to be selected.", GlobalRadiant().getMainWindow());
		return;
	}

	globalOutputStream() << "CSG Merge: Merging " << static_cast<int>(brushes.size()) << " brushes.\n";

	UndoableCommand undo("mergeSelectedBrushes");

	// Take the last selected node as reference for layers and parent
	scene::INodePtr merged = GlobalSelectionSystem().ultimateSelected();

	// Create a new BrushNode
	scene::INodePtr node = GlobalBrushCreator().createBrush();

	// Move the new brush to the same layers as the merged one
	scene::assignNodeToLayers(node, merged->getLayers());
		
	// Get the contained brush
	Brush* brush = Node_getBrush(node);

	// Attempt to merge the selected brushes into the new one 
	if (!Brush_merge(*brush, brushes, true)) {
		globalOutputStream() << "CSG Merge: Failed - result would not be convex.\n";
	}
	else {
		ASSERT_MESSAGE(!brush->empty(), "brush left with no faces after merge");

		scene::INodePtr parent = merged->getParent();
		assert(parent != NULL);

		// Remove the original brushes
		for (BrushPtrVector::iterator i = brushes.begin(); i != brushes.end(); i++) {
			scene::removeNodeFromParent(*i);
		}

		// Insert the newly created brush into the (same) parent entity
		parent->addChildNode(node);

		// Select the new brush
		Node_setSelected(node, true);
		
		globalOutputStream() << "CSG Merge: Succeeded.\n";
		SceneChangeNotify();
	}
}

class BrushSetClipPlane : 
	public SelectionSystem::Visitor
{
	Plane3 _plane;
public:
	BrushSetClipPlane(const Plane3& plane) : 
		_plane(plane)
	{}

	void visit(const scene::INodePtr& node) const {
		BrushNodePtr brush = boost::dynamic_pointer_cast<BrushNode>(node);

		if (brush != NULL && node->visible()) {
			brush->setClipPlane(_plane);
		}
	}
};

/**
 * greebo: Sets the "clip plane" of the selected brushes in the scene.
 */
void setBrushClipPlane(const Plane3& plane) {
	BrushSetClipPlane walker(plane);
	GlobalSelectionSystem().foreachSelected(walker);
}

/**
 * greebo: Splits the selected brushes by the given plane.
 */
void splitBrushesByPlane(const Vector3 planePoints[3], const std::string& shader, EBrushSplit split) {
	TextureProjection projection;
	projection.constructDefault();

	{
		// Instantiate a scoped walker
		BrushByPlaneClipper walker(
			planePoints[0], 
			planePoints[1], 
			planePoints[2], 
			shader, 
			projection, 
			split
		);
		GlobalSelectionSystem().foreachSelected(walker);
	}

	SceneChangeNotify();
}

} // namespace algorithm
} // namespace brush
