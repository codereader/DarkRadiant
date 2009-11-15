#ifndef BEZIER_H_
#define BEZIER_H_

#include "math/Vector3.h"
#include <glib/gslist.h>
#include <limits>

struct BezierCurve
{
	Vector3 crd;
	Vector3 left;
	Vector3 right;

	BezierCurve()
	{}

	BezierCurve(const Vector3& crd_, const Vector3& left_, const Vector3& right_) :
		crd(crd_),
		left(left_),
		right(right_)
	{}
};

const std::size_t BEZIERCURVETREE_MAX_INDEX = std::numeric_limits<std::size_t>::max();

/** 
 * greebo: A BezierCurveTree represents a node in the binary subdivision tree.
 * If left and right are both NULL, this node is a leaf and no further subdivisions
 * are available for this part of the patch. The index variable holds the depth of this node.
 * A leaf carries BEZIERCURVETREE_MAX_INDEX as index.
 */
class BezierCurveTree
{
public:
	std::size_t index;
	BezierCurveTree* left;
	BezierCurveTree* right;

	// Returns TRUE when no more subdivisions are available beyond this depth
	bool isLeaf() const
	{
		return left == NULL && right == NULL;
	}
};

std::size_t BezierCurveTree_Setup(BezierCurveTree *pCurve, std::size_t index, std::size_t stride);
void BezierCurveTree_Delete(BezierCurveTree *pCurve);
void BezierCurveTree_FromCurveList(BezierCurveTree *pTree, GSList *pCurveList, std::size_t depth = 0);

void BezierInterpolate(BezierCurve *pCurve);
bool BezierCurve_IsCurved(BezierCurve *pCurve);

inline void BezierCurveTreeArray_deleteAll(std::vector<BezierCurveTree*>& curveTrees) {
  for(std::vector<BezierCurveTree*>::iterator i = curveTrees.begin(); i != curveTrees.end(); ++i) {
    BezierCurveTree_Delete(*i);
  }
}

#endif /*BEZIER_H_*/
