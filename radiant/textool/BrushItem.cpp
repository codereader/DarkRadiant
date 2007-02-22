#include "BrushItem.h"

#include "FaceItem.h"

namespace selection {
	namespace textool {

class FaceItemCreator :
	public BrushVisitor
{
	// The target vector
	TexToolItemVec& _vector;
public:
	FaceItemCreator(TexToolItemVec& vector) :
		_vector(vector)
	{}
	
	void visit(Face& face) const {
		TexToolItemPtr faceItem(
			new FaceItem(face)
		);
		
		_vector.push_back(faceItem);
	}
};

// Constructor
BrushItem::BrushItem(Brush& sourceBrush) : 
	_sourceBrush(sourceBrush)
{
	// Visit all the brush faces with the FaceItemCreator
	// that populates the _children vector
	_sourceBrush.forEachFace(FaceItemCreator(_children));
}

AABB BrushItem::getExtents() {
	AABB returnValue;
	
	// Cycle through all the children and include their AABB
	for (unsigned int i = 0; i < _children.size(); i++) {
		returnValue.includeAABB(_children[i]->getExtents());
	}
	
	return returnValue;
}

void BrushItem::render() {
	// Cycle through all the children and ask them to render themselves
	for (unsigned int i = 0; i < _children.size(); i++) {
		_children[i]->render();
	}
}

void BrushItem::transform(const Matrix4& matrix) {
	// Cycle through all the children and ask them to render themselves
	for (unsigned int i = 0; i < _children.size(); i++) {
		_children[i]->transform(matrix);
	}
}

void BrushItem::transformSelected(const Matrix4& matrix) {
	// If this object is selected, transform the whole BrushItem and all children
	if (_selected) {
		transform(matrix);
	}
	else {
		// BrushItem is not selected, propagate the call
		for (unsigned int i = 0; i < _children.size(); i++) {
			_children[i]->transformSelected(matrix);
		}
	}
}

bool BrushItem::testSelect(const Rectangle& rectangle) {
	// Cycle through all the children and ask them to render themselves
	for (unsigned int i = 0; i < _children.size(); i++) {
		// Return true on the first selected child
		if (_children[i]->testSelect(rectangle)) {
			return true;
		}
	}
	
	// Nothing selectable, return false
	return false;
}

TexToolItemVec BrushItem::getSelectables(const Rectangle& rectangle) {
	TexToolItemVec returnVector;
	
	for (unsigned int i = 0; i < _children.size(); i++) {
		// Return true on the first selected child
		if (_children[i]->testSelect(rectangle)) {
			returnVector.push_back(_children[i]);
		}
	}
	
	return returnVector;
}

void BrushItem::beginTransformation() {
	_sourceBrush.undoSave();
}

	} // namespace TexTool
} // namespace selection
