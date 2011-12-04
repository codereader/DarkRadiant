#include "PatchBezier.h"

#include "math/pi.h"
#include "registry/registry.h"

	namespace {
		const std::string RKEY_PATCH_SUBDIVIDE_THRESHOLD = "user/ui/patch/subdivideThreshold";
	}

/* greebo: These are a lot of helper functions related to bezier curves
 */

void BezierInterpolate(BezierCurve *pCurve) {
	pCurve->left = pCurve->left.mid(pCurve->crd);
	pCurve->right = pCurve->crd.mid(pCurve->right);
	pCurve->crd = pCurve->left.mid(pCurve->right);
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

	static float subdivideThreshold = registry::getValue<float>(RKEY_PATCH_SUBDIVIDE_THRESHOLD);

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
	leftCurve->crd = left.mid(crd);		// ip_left
	rightCurve->crd = crd.mid(right);		// ip_right
	leftCurve->right = rightCurve->left = leftCurve->crd.mid(rightCurve->crd); // ip_crd
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
	GSList* leftList = NULL;
	GSList* rightList = NULL;

	bool listSplit = false;

	// Traverse the list and interpolate all curves which satisfy the "isCurved" condition
	for (GSList *l = pCurveList; l != NULL; l = l->next)
	{
		BezierCurve* curve = static_cast<BezierCurve*>(l->data);

		if (listSplit || curve->isCurved())
		{
			// Set the flag to TRUE to indicate that we already subdivided one part of this list
			// All other parts will be subdivided too
			listSplit = true;

			// Split this curve in two, by allocating two new curves
			BezierCurve* leftCurve = new BezierCurve;
			BezierCurve* rightCurve = new BezierCurve;

			// Let the current curve submit interpolation data to the newly allocated curves
			curve->interpolate(leftCurve, rightCurve);

			// Add the new curves to the allocated list
			leftList = g_slist_prepend(leftList, leftCurve);
			rightList = g_slist_prepend(rightList, rightCurve);
		}
	}

	// If we have a subdivision in this list, enter to the next level of recursion
	if (leftList != NULL && rightList != NULL && depth != PATCH_MAX_SUBDIVISION_DEPTH)
	{
		// Allocate two new tree nodes for the left and right part
		pTree->left = new BezierCurveTree;
		pTree->right = new BezierCurveTree;

		BezierCurveTree_FromCurveList(pTree->left, leftList, depth + 1);
		BezierCurveTree_FromCurveList(pTree->right, rightList, depth + 1);

		// Free the left and right lists from this level recursion
		for (GSList* l = leftList; l != 0; l = g_slist_next(l))
		{
			delete (BezierCurve*)l->data;
		}

		for (GSList* l = rightList; l != 0; l = g_slist_next(l))
		{
			delete (BezierCurve*)l->data;
		}

		g_slist_free(leftList);
		g_slist_free(rightList);
	}

	// If no subdivisions have been calculated, just leave this tree node, children are NULL by default
}
