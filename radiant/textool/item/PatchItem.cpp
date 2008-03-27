#include "PatchItem.h"

#include "PatchVertexItem.h"
#include "patch/Patch.h"

namespace textool {

PatchItem::PatchItem(Patch& sourcePatch) : 
	_sourcePatch(sourcePatch)
{
	// Add all the patch control vertices as children to this class
	for (PatchControlIter i = _sourcePatch.begin(); i != _sourcePatch.end(); i++) {
		// Allocate a new vertex children on the heap
		TexToolItemPtr patchVertexItem(
			new PatchVertexItem(*i)
		);
		
		// Add it to the children of this patch
		_children.push_back(patchVertexItem);
	}
}

void PatchItem::render() {
	glEnable(GL_BLEND);
	glBlendColor(0,0,0, 0.3f);
	glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);
	
	glColor3f(1, 1, 1);
	glBegin(GL_QUAD_STRIP);
	
	// Get the tesselation and the first
	PatchTesselation& tess = _sourcePatch.getTesselation();
	
	const RenderIndex* strip_indices = tess.m_indices.data();
	
	for (std::size_t i = 0; i<tess.m_numStrips; i++, strip_indices += tess.m_lenStrips)	{
		for (unsigned int offset = 0; offset < tess.m_lenStrips; offset++) {
			// Retrieve the mesh vertex from the line strip
			ArbitraryMeshVertex& meshVertex = tess.m_vertices[*(strip_indices + offset)];
			glVertex2f(meshVertex.texcoord[0], meshVertex.texcoord[1]);
		}
	}
	glEnd();
	glDisable(GL_BLEND);
	
	// Now invoke the default render method (calls render() on all children)
	TexToolItem::render();
}

void PatchItem::beginTransformation() {
	// Call the default routine
	TexToolItem::beginTransformation();
	// Save the patch undomemento
	_sourcePatch.undoSave();
}

void PatchItem::update() {
	// Call the default routine
	TexToolItem::update();
	// Notify the sourcepatch what's happened 
	_sourcePatch.controlPointsChanged();
}

void PatchItem::selectRelated() {
	// Call the default routine
	TexToolItem::selectRelated();
	
	// Now have a look at the patch vertices, select all if one is selected
	for (std::size_t i = 0; i < _children.size(); i++) {
		if (_children[i]->isSelected()) {
			// A selected child has been found, select them all
			for (unsigned int j = 0; j < _children.size(); j++) {
				_children[j]->setSelected(true);
			}
			// Stop the loop
			break;
		}
	}
}

} // namespace TexTool
