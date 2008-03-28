#include "BrushByPlaneClipper.h"

#include "CSG.h"
#include "scenelib.h"
#include "brush/Brush.h"

namespace brush {
namespace algorithm {

BrushByPlaneClipper::BrushByPlaneClipper(
	const Vector3& p0, const Vector3& p1, const Vector3& p2, 
	const std::string& shader, 
	const TextureProjection& projection, EBrushSplit split) : 
		_p0(p0), 
		_p1(p1), 
		_p2(p2), 
		_shader(shader), 
		_projection(projection), 
		_split(split)
{}

BrushByPlaneClipper::~BrushByPlaneClipper() {
	for (std::set<scene::INodePtr>::iterator i = _deleteList.begin(); 
		 i != _deleteList.end(); i++)
	{
		// Remove the node from the scene
		scene::removeNodeFromParent(*i);
	}

	for (InsertMap::iterator i = _insertList.begin();
		 i != _insertList.end(); i++)
	{
		// Insert the child into the designated parent
		scene::addNodeToContainer(i->first, i->second);

		// Select the child
		Node_setSelected(i->first, true);
	}
}

void BrushByPlaneClipper::visit(const scene::INodePtr& node) const {
	// Don't clip invisible nodes
	if (!node->visible()) {
		return;
	}

	// Try to cast the instance onto a brush
	Brush* brush = Node_getBrush(node);

	// Return if not brush
	if (brush == NULL) {
		return;
	}

	Plane3 plane(_p0, _p1, _p2);
	if (!plane.isValid()) {
		return;
	}

	// greebo: Analyse the brush to find out which shader is the most used one
	getMostUsedTexturing(brush);

	BrushSplitType split = Brush_classifyPlane(*brush, _split == eFront ? -plane : plane);

	if (split.counts[ePlaneBack] && split.counts[ePlaneFront]) {
		// the plane intersects this brush
		if (_split == eFrontAndBack) {
			scene::INodePtr brushNode = GlobalBrushCreator().createBrush();

			assert(brushNode != NULL);

			Brush* fragment = Node_getBrush(brushNode);
			assert(fragment != NULL);
			fragment->copy(*brush);

			FacePtr newFace = fragment->addPlane(_p0, _p1, _p2, _mostUsedShader, _mostUsedProjection);

			if (newFace != NULL && _split != eFront) {
				newFace->flipWinding();
			}

			fragment->removeEmptyFaces();
			ASSERT_MESSAGE(!fragment->empty(), "brush left with no faces after split");

			// Mark this brush for insertion
			_insertList.insert(InsertMap::value_type(brushNode, node->getParent()));
		}

		FacePtr newFace = brush->addPlane(_p0, _p1, _p2, _mostUsedShader, _mostUsedProjection);

		if (newFace != NULL && _split == eFront) {
			newFace->flipWinding();
		}

		brush->removeEmptyFaces();
		ASSERT_MESSAGE(!brush->empty(), "brush left with no faces after split");
	}
	// the plane does not intersect this brush
	else if (_split != eFrontAndBack && split.counts[ePlaneBack] != 0) {
		// the brush is "behind" the plane
		_deleteList.insert(node);
	}
}

void BrushByPlaneClipper::getMostUsedTexturing(const Brush* brush) const {
	std::map<std::string, int> shaderCount;
	_mostUsedShader = "";
	int mostUsedShaderCount(0);
	_mostUsedProjection = TextureProjection();

	// greebo: Get the most used shader of this brush
	for (Brush::const_iterator i = brush->begin(); i != brush->end(); i++) {
		// Get the shadername
		const std::string& shader = (*i)->GetShader();

		// Insert counter, if necessary
		if (shaderCount.find(shader) == shaderCount.end()) {
			shaderCount[shader] = 0;
		}

		// Increase the counter
		shaderCount[shader]++;

		if (shaderCount[shader] > mostUsedShaderCount) {
			_mostUsedShader = shader;
			mostUsedShaderCount = shaderCount[shader];

			// Copy the TexDef from the face into the local member
			(*i)->GetTexdef(_mostUsedProjection);
		}
	}

	// Fall back to the default shader, if nothing found
	if (_mostUsedShader.empty() || mostUsedShaderCount == 1) {
		_mostUsedShader = _shader;
		_mostUsedProjection = _projection;
	}
}

} // namespace algorithm
} // namespace brush
