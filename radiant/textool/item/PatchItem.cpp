#include "PatchItem.h"

#include "igl.h"
#include "PatchVertexItem.h"

namespace textool
{

PatchItem::PatchItem(IPatch& sourcePatch) :
	_sourcePatch(sourcePatch)
{
	// Add all the patch control vertices as children to this class
	for (std::size_t c = 0; c < _sourcePatch.getWidth(); c++)
	{
		for (std::size_t r = 0; r < _sourcePatch.getHeight(); r++)
		{
			_children.emplace_back(new PatchVertexItem(_sourcePatch.ctrlAt(r, c)));
		}
	}
}

void PatchItem::render()
{
	glEnable(GL_BLEND);
	glBlendColor(0,0,0, 0.3f);
	glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

	glColor3f(1, 1, 1);

	// Get the tesselation and the first
	auto tess = _sourcePatch.getTesselatedPatchMesh();

	auto renderInfo = _sourcePatch.getRenderIndices();
	auto* strip_indices = &renderInfo.indices.front();

	for (std::size_t i = 0; i < renderInfo.numStrips; i++, strip_indices += renderInfo.lenStrips)
	{
		glBegin(GL_QUAD_STRIP);

		for (std::size_t offset = 0; offset < renderInfo.lenStrips; offset++)
		{
			// Retrieve the mesh vertex from the line strip
			auto& meshVertex = tess.vertices[*(strip_indices + offset)];
			glVertex2d(meshVertex.texcoord[0], meshVertex.texcoord[1]);
		}

		glEnd();
	}

	glDisable(GL_BLEND);

	// Now invoke the default render method (calls render() on all children)
	TexToolItem::render();
}

void PatchItem::beginTransformation() 
{
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
