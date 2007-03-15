#include "Texturable.h"

#include "brush/BrushInstance.h"
#include "brush/Face.h"
#include "patch/Patch.h"
#include "patch/PatchSceneWalk.h"

namespace selection {

Texturable::Texturable() :
	face(NULL),
	brush(NULL),
	patch(NULL)
{}

void Texturable::clear() {
	face = NULL;
	patch = NULL;
	brush = NULL;
}

// True, if all the pointers are NULL
bool Texturable::empty() const {
	return (patch == NULL && face == NULL && brush == NULL);
}

bool Texturable::isPatch() const {
	return (patch != NULL);
}

bool Texturable::isFace() const {
	return (face != NULL);
}

std::string Texturable::getShader() const {
	if (isFace()) {
		return face->GetShader();
	}
	else if (isPatch()) {
		return patch->GetShader();
	}
	else {
		return "";
	}
}

namespace algorithm {

ClosestTexturableFinder::ClosestTexturableFinder(SelectionTest& test, Texturable& texturable) :
	_texturable(texturable),
	_selectionTest(test)
{}

bool ClosestTexturableFinder::pre(const scene::Path& path, scene::Instance& instance) const {
	// Check if the node is filtered
	if (path.top().get().visible()) {	
		// Test the instance for a brush
		BrushInstance* brush = Instance_getBrush(instance);
		
		if (brush != NULL) {
			// Construct the selectiontest
			_selectionTest.BeginMesh(brush->localToWorld());
			
			// Cycle through all the faces
			for (Brush::const_iterator i = brush->getBrush().begin(); 
				 i != brush->getBrush().end(); 
				 i++) 
			{
				// Test the face for selection
				SelectionIntersection intersection;
				(*i)->testSelect(_selectionTest, intersection);
				
				// Any intersection found / is it better than the previous one?
				if (intersection.valid() && 
					SelectionIntersection_closer(intersection, _bestIntersection)) 
				{
					// Yes, store this as new best intersection
					_bestIntersection = intersection;
					
					// Save the face and the parent brush
					_texturable.face = (*i);
					_texturable.brush = &brush->getBrush();
					_texturable.patch = NULL;
				}
			}
		}
		else {
			// No brush, test for a patch
			SelectionTestable* selectionTestable = Instance_getSelectionTestable(instance);
			
			if (selectionTestable != NULL) {
				bool occluded;
				OccludeSelector selector(_bestIntersection, occluded);
				selectionTestable->testSelect(selector, _selectionTest);
				
				if (occluded) {
					Patch* patch = Node_getPatch(path.top());
					if (patch != NULL) {
						_texturable.brush = NULL;
						_texturable.face = NULL;
						_texturable.patch = patch;
					}
					else {
						_texturable = Texturable();
					}
				}
			}
		}
	}
		
	return true;
}
		
} // namespace algorithm
} // namespace selection
