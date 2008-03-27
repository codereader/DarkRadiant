#include "BrushItem.h"

#include "FaceItem.h"

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

void BrushItem::beginTransformation() {
	_sourceBrush.undoSave();
}

void BrushItem::selectRelated() {
	// Call the default routine
	TexToolItem::selectRelated();
	
	// Select all the faces, if one is selected
	for (std::size_t i = 0; i < _children.size(); i++) {
		if (_children[i]->isSelected()) {
			// A selected child has been found, select them all
			for (std::size_t j = 0; j < _children.size(); j++) {
				_children[j]->setSelected(true);
			}
			// Stop the loop
			break;
		}
	}
}

} // namespace TexTool
