#pragma once

#include "math/Vector3.h"
#include <glib.h>
#include <limits>
#include <vector>

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

	// Returns true if the given set of coordinates satisfies the "curved" condition
	// This is determined by comparing the directions of the various deltas
	bool isCurved() const;

	// Interpolates the curve values and writes them to <leftCurve> and <rightCurve>
	// which will form two new (connected) segments interpolating the current one.
	void interpolate(BezierCurve* leftCurve, BezierCurve* rightCurve) const;
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

	BezierCurveTree() :
		left(NULL),
		right(NULL)
	{}

	~BezierCurveTree()
	{
		delete left;	// it's safe to pass NULL to delete
		delete right;
	}

	// Returns TRUE when no more subdivisions are available beyond this depth
	bool isLeaf() const
	{
		return left == NULL && right == NULL;
	}

	// Sets up the indices of this part of the curve tree
	// Recursively calls setup() on the children if available
	// The index is returned unchanged if no children are set up.
	std::size_t setup(std::size_t idx, std::size_t stride);
};

void BezierCurveTree_FromCurveList(BezierCurveTree *pTree, GSList *pCurveList, std::size_t depth = 0);

void BezierInterpolate(BezierCurve *pCurve);

inline void BezierCurveTreeArray_deleteAll(std::vector<BezierCurveTree*>& curveTrees) {
  for(std::vector<BezierCurveTree*>::iterator i = curveTrees.begin(); i != curveTrees.end(); ++i) {
     delete *i;
  }
}
