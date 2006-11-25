#ifndef PATCHTESSELATION_H_
#define PATCHTESSELATION_H_

#include "render.h"
#include "PatchBezier.h"

/* greebo: This is the structure that represents the tesselation of a patch. Haven't looked too deep into 
 * the whole Bezier stuff so I can't say more, I'm afraid.
 */

class PatchTesselation {
public:
  PatchTesselation()
    : m_numStrips(0), m_lenStrips(0), m_nArrayWidth(0), m_nArrayHeight(0)
  {
  }
  Array<ArbitraryMeshVertex> m_vertices;
  Array<RenderIndex> m_indices;
  std::size_t m_numStrips;
  std::size_t m_lenStrips;

  Array<std::size_t> m_arrayWidth;
  std::size_t m_nArrayWidth;
  Array<std::size_t> m_arrayHeight;
  std::size_t m_nArrayHeight;

  Array<BezierCurveTree*> m_curveTreeU;
  Array<BezierCurveTree*> m_curveTreeV;
};

#endif /*PATCHTESSELATION_H_*/
