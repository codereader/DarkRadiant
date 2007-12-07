#ifndef BEZIER_H_
#define BEZIER_H_

#include "math/Vector3.h"
#include <glib/gslist.h>
#include "container/array.h"
#include <limits>

struct BezierCurve {
  Vector3 crd;
  Vector3 left;
  Vector3 right;
};

const std::size_t BEZIERCURVETREE_MAX_INDEX = std::numeric_limits<std::size_t>::max();

struct BezierCurveTree {
  std::size_t index;
  BezierCurveTree* left;
  BezierCurveTree* right;
};

inline bool BezierCurveTree_isLeaf(const BezierCurveTree* node) {
  return node->left == 0 && node->right == 0;
}

std::size_t BezierCurveTree_Setup(BezierCurveTree *pCurve, std::size_t index, std::size_t stride);
void BezierCurveTree_Delete(BezierCurveTree *pCurve);
void BezierCurveTree_FromCurveList(BezierCurveTree *pTree, GSList *pCurveList, std::size_t depth = 0);

void BezierInterpolate(BezierCurve *pCurve);
bool BezierCurve_IsCurved(BezierCurve *pCurve);

inline void BezierCurveTreeArray_deleteAll(Array<BezierCurveTree*>& curveTrees) {
  for(Array<BezierCurveTree*>::iterator i = curveTrees.begin(); i != curveTrees.end(); ++i) {
    BezierCurveTree_Delete(*i);
  }
}

#endif /*BEZIER_H_*/
