#include "ClosestTexturableFinder.h"

#include "ifilter.h"
#include "ientity.h"
#include "brush/Brush.h"
#include "patch/Patch.h"

#include "Texturable.h"

#include "scenelib.h"
#include "selection/OccludeSelector.h"

namespace selection
{

namespace algorithm 
{

ClosestTexturableFinder::ClosestTexturableFinder(SelectionTest& test, Texturable& texturable) :
	_texturable(texturable),
	_selectionTest(test)
{}

bool ClosestTexturableFinder::pre(const scene::INodePtr& node)
{
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
				if (!GlobalFilterSystem().isVisible(FilterRule::TYPE_TEXTURE, (*i)->getShader()))
				{
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
						if (GlobalFilterSystem().isVisible(FilterRule::TYPE_TEXTURE, patch->getShader()))
						{
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
		return scene::isGroupNode(node);
	}

	// Return TRUE, traverse this subgraph
	return true;
}

} // namespace

} // namespace
