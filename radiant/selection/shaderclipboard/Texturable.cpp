#include "Texturable.h"

#include "ifilter.h"
#include "nameable.h"
#include "selectionlib.h"
#include "brush/Face.h"
#include "brush/Brush.h"
#include "patch/Patch.h"
#include "patch/PatchSceneWalk.h"

namespace selection {

Texturable::Texturable() :
	face(NULL),
	brush(NULL),
	patch(NULL),
	shader("")
{}

void Texturable::clear() {
	face = NULL;
	patch = NULL;
	brush = NULL;
	shader = "";
}

// True, if all the pointers are NULL
bool Texturable::empty() const
{
	if (patch != NULL || face != NULL || brush != NULL)
	{
		// For primitive types, return true if the node has been removed
		return (node.lock() == NULL);
	}

	// For non-primitives (all source pointers NULL), check the shader
	return shader.empty();
}

bool Texturable::checkValid()
{
	// For non-pure shader strings, check if the node has been removed
	if (patch != NULL || face != NULL || brush != NULL)
	{
		if (node.lock() == NULL)
		{
			clear();
			return false; // changed
		}
	}

	// no change
	return true;
}

bool Texturable::isPatch() const
{
	return (node.lock() != NULL && patch != NULL);
}

bool Texturable::isFace() const {
	return (node.lock() != NULL && face != NULL);
}

bool Texturable::isShader() const {
	return !shader.empty();
}

std::string Texturable::getShader() const {
	if (isFace()) {
		return face->getShader();
	}
	else if (isPatch()) {
		return patch->getShader();
	}
	else {
		// Shader might be empty as well
		return shader;
	}
}

namespace algorithm {

ClosestTexturableFinder::ClosestTexturableFinder(SelectionTest& test, Texturable& texturable) :
	_texturable(texturable),
	_selectionTest(test)
{}

bool ClosestTexturableFinder::pre(const scene::INodePtr& node) {
	// Don't traverse invisible nodes and items
	if (!node->visible()) {
		return false;
	}

	// Check if this node is an entity
	bool isEntity = Node_isEntity(node);

	// Check if the node is an entity
	if (!isEntity) {
		// Test the instance for a brush
		Brush* brush = Node_getBrush(node);

		if (brush != NULL) {
			// Construct the selectiontest
			_selectionTest.BeginMesh(node->localToWorld());

			// Cycle through all the faces
			for (Brush::const_iterator i = brush->begin();
				 i != brush->end();
				 ++i)
			{
				// Check for filtered faces, don't select them
				if (!GlobalFilterSystem().isVisible("texture", (*i)->getShader())) {
					continue;
				}

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
					_texturable.face = (*i).get();
					_texturable.brush = brush;
					_texturable.patch = NULL;
					_texturable.node = node;
				}
			}
		}
		else {
			// No brush, test for a patch
			SelectionTestablePtr selectionTestable = Node_getSelectionTestable(node);

			if (selectionTestable != NULL) {
				bool occluded;
				OccludeSelector selector(_bestIntersection, occluded);
				selectionTestable->testSelect(selector, _selectionTest);

				if (occluded) {
					_texturable = Texturable();

					Patch* patch = Node_getPatch(node);
					if (patch != NULL) {
						// Check for filtered patches
						if (GlobalFilterSystem().isVisible("texture", patch->getShader())) {
							_texturable.brush = NULL;
							_texturable.face = NULL;
							_texturable.patch = patch;
							_texturable.node = node;
						}
					}
				}
			}
		}
	}
	else {
		// Is an entity, don't traverse it, if it isn't a group node
		return node_is_group(node);
	}

	// Return TRUE, traverse this subgraph
	return true;
}

} // namespace algorithm
} // namespace selection
