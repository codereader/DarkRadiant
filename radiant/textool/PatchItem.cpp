#include "PatchItem.h"

#include "patch/Patch.h"
#include "PatchVertexItem.h"

namespace selection {
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

AABB PatchItem::getExtents() {
	AABB returnValue;
	
	// Cycle through all the children and include their AABB
	for (unsigned int i = 0; i < _children.size(); i++) {
		returnValue.includeAABB(_children[i]->getExtents());
	}
	
	return returnValue;
}

void PatchItem::render() {
	glEnable(GL_BLEND);
	glBlendColor(0,0,0, 0.2f);
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
	
	// Cycle through all the children and ask them to render themselves
	for (unsigned int i = 0; i < _children.size(); i++) {
		_children[i]->render();
	}
}

void PatchItem::transform(const Matrix4& matrix) {
	// Cycle through all the children and ask them to render themselves
	for (unsigned int i = 0; i < _children.size(); i++) {
		_children[i]->transform(matrix);
	}
}

void PatchItem::transformSelected(const Matrix4& matrix) {
	// If this object is selected, transform the whole PatchItem and all children
	if (_selected) {
		transform(matrix);
	}
	else {
		// PatchItem is not selected, propagate the call
		for (unsigned int i = 0; i < _children.size(); i++) {
			_children[i]->transformSelected(matrix);
		}
	}
	
	_sourcePatch.controlPointsChanged();
}

bool PatchItem::testSelect(const Rectangle& rectangle) {
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

TexToolItemVec PatchItem::getSelectables(const Rectangle& rectangle) {
	TexToolItemVec returnVector;
	
	for (unsigned int i = 0; i < _children.size(); i++) {
		// Return true on the first selected child
		if (_children[i]->testSelect(rectangle)) {
			returnVector.push_back(_children[i]);
		}
	}
	
	return returnVector;
}

void PatchItem::beginTransformation() {
	_sourcePatch.undoSave();
}

	} // namespace TexTool
} // namespace selection
