#include "PatchBezier.h"

#include "math/pi.h"
#include "iregistry.h"

	namespace {
		const std::string RKEY_PATCH_SUBDIVIDE_THRESHOLD = "user/ui/patch/subdivideThreshold";
	}

/* greebo: These are a lot of helper functions related to bezier curves 
 */

void BezierInterpolate(BezierCurve *pCurve) {
	pCurve->left = vector3_mid(pCurve->left, pCurve->crd);
	pCurve->right = vector3_mid(pCurve->crd, pCurve->right);
	pCurve->crd = vector3_mid(pCurve->left, pCurve->right);
}

bool BezierCurve::isCurved() const
{
	// Calculate the deltas
	Vector3 vTemp(right - left);
	Vector3 v1(crd - left);
	
	if (v1 == g_vector3_identity || vTemp == v1) // return 0 if 1->2 == 0 or 1->2 == 1->3
	{
		return false;
	}

	Vector3 v2(right - crd);

	v1.normalise();
	v2.normalise();

	if (v1 == v2)
	{
		// All points are on the same line
		return false;
	}

	Vector3 v3(vTemp);
	const double width = v3.getLength();
	
	v3 *= 1.0 / width;

	if (v1 == v3 && v2 == v3)
	{
		return false;
	}

	// The points are not on the same line, determine the angle
	const double angle = acos(v1.dot(v2)) / c_pi;

	const double index = width * angle;

	static float subdivideThreshold = GlobalRegistry().getFloat(RKEY_PATCH_SUBDIVIDE_THRESHOLD);

	if (index > subdivideThreshold)
	{
		return true;
	}

	return false;
}

/**
 * greebo: The vertex interpolation works like this: 
 *
 * The original segment is LEFT >> CRD >> RIGHT, which will be sudivided into two segments: "left" and "right".
 *
 * The "left" segment will be LEFT >> ip_left >> ip_crd
 * The "right" segment will be ip_crd >> ip_right >> RIGHT
 *
 * In the end, the two segments will still be using the LEFT and RIGHT vertices (which is important as these are the
 * "fixed" control points of the patch, but the CRD one will be disregarded.
 
   LEFT O 
        |   
        |
        | 
        |
        |
        |
ip_left X
        |\
        | \
        |  \
        |   X  ip_crd
        |    \
        |     \
    CRD O------X------O RIGHT
             ip_right
*/
void BezierCurve::interpolate(BezierCurve* leftCurve, BezierCurve* rightCurve) const
{
	// The left and right vertices are the anchors
	leftCurve->left = left;
	rightCurve->right = right;

	// The mid-point of the current curve 
	leftCurve->crd = vector3_mid(left, crd);		// ip_left
	rightCurve->crd = vector3_mid(crd, right);		// ip_right
	leftCurve->right = rightCurve->left = vector3_mid(leftCurve->crd, rightCurve->crd); // ip_crd
}

std::size_t BezierCurveTree::setup(std::size_t idx, std::size_t stride)
{
	if (left != NULL && right != NULL)
	{
		idx = left->setup(idx, stride);

		// Store new index
		index = idx*stride;

		idx++;
		idx = right->setup(idx, stride);
	}
	else
	{
		// Either left or right is NULL, assign leaf index
		index = BEZIERCURVETREE_MAX_INDEX;
	}

	// idx will be returned unchanged if no children have been setup
	return idx;
}

const std::size_t PATCH_MAX_SUBDIVISION_DEPTH = 16;

void BezierCurveTree_FromCurveList(BezierCurveTree *pTree, GSList *pCurveList, std::size_t depth)
{
	GSList *pLeftList = 0;
	GSList *pRightList = 0;
	
	BezierCurve *pCurve, *pLeftCurve, *pRightCurve;

	bool bSplit = false;

	// Traverse the list and interpolate all curves which satisfy the "isCurved" condition
	for (GSList *l = pCurveList; l; l = l->next)
	{
		pCurve = (BezierCurve *)(l->data);

		if (bSplit || pCurve->isCurved())
		{
			// Set the flag to TRUE to indicate that we already subdivided one part of this list
			// All other parts will be subdivided too
			bSplit = true;

			// Split this curve in two, by allocating two new curves
			pLeftCurve = new BezierCurve;
			pRightCurve = new BezierCurve;

			// Let the current curve submit interpolation data to the newly allocated curves
			pCurve->interpolate(pLeftCurve, pRightCurve);

			// Add the new curves to the allocated list
			pLeftList = g_slist_prepend(pLeftList, pLeftCurve);
			pRightList = g_slist_prepend(pRightList, pRightCurve);
		}
	}

  if(pLeftList != 0 && pRightList != 0 && depth != PATCH_MAX_SUBDIVISION_DEPTH)
  {
    pTree->left = new BezierCurveTree;
    pTree->right = new BezierCurveTree;
    BezierCurveTree_FromCurveList(pTree->left, pLeftList, depth + 1);
    BezierCurveTree_FromCurveList(pTree->right, pRightList, depth + 1);

    for(GSList* l = pLeftList; l != 0; l = g_slist_next(l))
    {
      delete (BezierCurve*)l->data;
    }

    for(GSList* l = pRightList; l != 0; l = g_slist_next(l))
    {
      delete (BezierCurve*)l->data;
    }
    
    g_slist_free(pLeftList);
    g_slist_free(pRightList);
  }
  else
  {
    pTree->left = 0;
    pTree->right = 0;
  }
}
