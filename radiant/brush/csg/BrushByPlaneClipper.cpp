#include "BrushByPlaneClipper.h"

#include "CSG.h"
#include "scenelib.h"
#include "brush/BrushNode.h"
#include "ui/texturebrowser/TextureBrowser.h"

namespace brush
{

namespace algorithm
{

BrushByPlaneClipper::BrushByPlaneClipper(
	const Vector3& p0, const Vector3& p1, const Vector3& p2,
	const TextureProjection& projection, EBrushSplit split) :
		_p0(p0),
		_p1(p1),
		_p2(p2),
		_projection(projection),
		_split(split),
		_useCaulk(GlobalClipper().useCaulkForNewFaces()),
		_caulkShader(GlobalClipper().getCaulkShader())
{}

void BrushByPlaneClipper::split(const BrushPtrVector& brushes)
{
	Plane3 plane(_p0, _p1, _p2);

	if (!plane.isValid())
	{
		return;
	}

	for (BrushPtrVector::const_iterator i = brushes.begin(); i != brushes.end(); ++i)
	{
		const BrushNodePtr& node = *i;

		// Don't clip invisible nodes
		if (!node->visible())
		{
			continue;
		}

		Brush& brush = node->getBrush();

		scene::INodePtr parent = node->getParent();

		if (!parent)
		{
			continue;
		}

		// greebo: Analyse the brush to find out which shader is the most used one
		getMostUsedTexturing(brush);

		BrushSplitType split = Brush_classifyPlane(brush, _split == eFront ? -plane : plane);

		if (split.counts[ePlaneBack] && split.counts[ePlaneFront])
		{
			// the plane intersects this brush
			if (_split == eFrontAndBack)
			{
				scene::INodePtr fragmentNode = GlobalBrushCreator().createBrush();

				assert(fragmentNode != NULL);

				// Put the fragment in the same layer as the brush it was clipped from
				// Do this before adding the fragment to the parent, since there is an
				// update algorithm setting the visibility of the fragment right there.
				fragmentNode->assignToLayers(node->getLayers());

				// greebo: For copying the texture scale the new node needs to be inserted in the scene
				// otherwise the shaders cannot be captured and the scale is off

				// Insert the child into the designated parent
				scene::addNodeToContainer(fragmentNode, parent);

				// Select the child
				Node_setSelected(fragmentNode, true);

				Brush* fragment = Node_getBrush(fragmentNode);
				assert(fragment != NULL);
				fragment->copy(brush);

				FacePtr newFace = fragment->addPlane(_p0, _p1, _p2, _mostUsedShader, _mostUsedProjection);

				if (newFace != NULL && _split != eFront)
				{
					newFace->flipWinding();
				}

				fragment->removeEmptyFaces();
				ASSERT_MESSAGE(!fragment->empty(), "brush left with no faces after split");
			}

			FacePtr newFace = brush.addPlane(_p0, _p1, _p2, _mostUsedShader, _mostUsedProjection);

			if (newFace != NULL && _split == eFront) {
				newFace->flipWinding();
			}

			brush.removeEmptyFaces();
			ASSERT_MESSAGE(!brush.empty(), "brush left with no faces after split");
		}
		// the plane does not intersect this brush
		else if (_split != eFrontAndBack && split.counts[ePlaneBack] != 0)
		{
			// the brush is "behind" the plane
			// Remove the node from the scene
			scene::removeNodeFromParent(node);
		}
	}
}

#if 0
void BrushByPlaneClipper::visit(const scene::INodePtr& node) const
{
	// Don't clip invisible nodes
	if (!node->visible())
	{
		return;
	}

	// Try to cast the instance onto a brush
	Brush* brush = Node_getBrush(node);

	// Return if not brush
	if (brush == NULL)
	{
		return;
	}

	Plane3 plane(_p0, _p1, _p2);

	if (!plane.isValid())
	{
		return;
	}

	// greebo: Analyse the brush to find out which shader is the most used one
	getMostUsedTexturing(*brush);

	BrushSplitType split = Brush_classifyPlane(*brush, _split == eFront ? -plane : plane);

	if (split.counts[ePlaneBack] && split.counts[ePlaneFront])
	{
		// the plane intersects this brush
		if (_split == eFrontAndBack)
		{
			scene::INodePtr fragmentNode = GlobalBrushCreator().createBrush();

			// greebo: For copying the texture scale the new node needs a valid rendersystem
			fragmentNode->setRenderSystem(node->getRenderSystem());

			assert(fragmentNode != NULL);

			Brush* fragment = Node_getBrush(fragmentNode);
			assert(fragment != NULL);
			fragment->copy(*brush);

			// Put the fragment in the same layer as the brush it was clipped from
			scene::assignNodeToLayers(fragmentNode, node->getLayers());

			FacePtr newFace = fragment->addPlane(_p0, _p1, _p2, _mostUsedShader, _mostUsedProjection);

			if (newFace != NULL && _split != eFront)
			{
				newFace->flipWinding();
			}

			fragment->removeEmptyFaces();
			ASSERT_MESSAGE(!fragment->empty(), "brush left with no faces after split");

			// Mark this brush for insertion
			_insertList.insert(InsertMap::value_type(fragmentNode, node->getParent()));
		}

		FacePtr newFace = brush->addPlane(_p0, _p1, _p2, _mostUsedShader, _mostUsedProjection);

		if (newFace != NULL && _split == eFront) {
			newFace->flipWinding();
		}

		brush->removeEmptyFaces();
		ASSERT_MESSAGE(!brush->empty(), "brush left with no faces after split");
	}
	// the plane does not intersect this brush
	else if (_split != eFrontAndBack && split.counts[ePlaneBack] != 0)
	{
		// the brush is "behind" the plane
		_deleteList.insert(node);
	}
}
#endif

void BrushByPlaneClipper::getMostUsedTexturing(const Brush& brush) const
{
	// Intercept this call to apply caulk to all faces when the registry key is set
	if (_useCaulk)
	{
		_mostUsedShader =  _caulkShader;
		return;
	}

	std::map<std::string, int> shaderCount;
	_mostUsedShader = "";
	int mostUsedShaderCount(0);
	_mostUsedProjection = TextureProjection();

	// greebo: Get the most used shader of this brush
	for (Brush::const_iterator i = brush.begin(); i != brush.end(); ++i)
	{
		// Get the shadername
		const std::string& shader = (*i)->getShader();

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
	if (_mostUsedShader.empty() || mostUsedShaderCount == 1)
	{
		_mostUsedShader = GlobalTextureBrowser().getSelectedShader();
		_mostUsedProjection = _projection;
	}
}

} // namespace algorithm
} // namespace brush
