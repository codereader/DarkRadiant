#include "MD5Surface.h"

namespace md5
{

inline VertexPointer vertexpointer_arbitrarymeshvertex(const ArbitraryMeshVertex* array)
{
  return VertexPointer(VertexPointer::pointer(&array->vertex), sizeof(ArbitraryMeshVertex));
}

void MD5Surface::testSelect(Selector& selector, 
							SelectionTest& test, 
							const Matrix4& localToWorld)
{
	test.BeginMesh(localToWorld);
	
	SelectionIntersection best;
	test.TestTriangles(
	  vertexpointer_arbitrarymeshvertex(m_vertices.data()),
	  IndexPointer(m_indices.data(), IndexPointer::index_type(m_indices.size())),
	  best
	);

	if(best.valid()) {
		selector.addIntersection(best);
	}
}

} // namespace md5
