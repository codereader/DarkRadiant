#include "CSG.h"

#include "iundo.h"
#include "igrid.h"
#include "scenelib.h"

#include "brush/Face.h"
#include "brush/Brush.h"
#include "brush/BrushNode.h"
#include "brush/BrushVisit.h"
#include "selection/algorithm/Primitives.h"

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

} // namespace csg
} // namespace algorithm
