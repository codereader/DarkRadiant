#include "CSG.h"

#include "iundo.h"
#include "igrid.h"
#include "iradiant.h"
#include "scenelib.h"

#include "brush/Face.h"
#include "brush/Brush.h"
#include "brush/BrushNode.h"
#include "brush/BrushVisit.h"
#include "selection/algorithm/Primitives.h"

#include "gtkutil/dialog.h"

namespace algorithm {
namespace csg {

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
	Node_setSelected(sourceBrush, false);
	parent->removeChildNode(sourceBrush);
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
			scene::INodePtr parent = (*i)->getParent();
			parent->removeChildNode(*i);
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
					
					scene::INodePtr node = GlobalBrushCreator().createBrush();
					(*i)->removeEmptyFaces();
					ASSERT_MESSAGE(!(*i)->empty(), "brush left with no faces after subtract");

					Node_getBrush(node)->copy(*(*i));
					delete (*i);

					path.parent()->addChildNode(node);
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
		<< " from " << static_cast<int>(before) << " brush" << (before == 1? "" : "es") << ".\n";

	SceneChangeNotify();
}

} // namespace csg
} // namespace algorithm
