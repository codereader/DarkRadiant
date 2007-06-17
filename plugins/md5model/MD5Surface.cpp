#include "MD5Surface.h"

namespace md5
{

inline VertexPointer vertexpointer_arbitrarymeshvertex(const ArbitraryMeshVertex* array)
{
  return VertexPointer(VertexPointer::pointer(&array->vertex), sizeof(ArbitraryMeshVertex));
}

// Refresh AABB
void MD5Surface::updateAABB() {
	m_aabb_local = AABB();
	for(vertices_t::iterator i = m_vertices.begin(); i != m_vertices.end(); ++i)
	  m_aabb_local.includePoint(reinterpret_cast<const Vector3&>(i->vertex));
	
	for(MD5Surface::indices_t::iterator i = m_indices.begin(); i != m_indices.end(); i += 3)
	{
			ArbitraryMeshVertex& a = m_vertices[*(i + 0)];
			ArbitraryMeshVertex& b = m_vertices[*(i + 1)];
			ArbitraryMeshVertex& c = m_vertices[*(i + 2)];
	
	  ArbitraryMeshTriangle_sumTangents(a, b, c);
	}
	
	for(MD5Surface::vertices_t::iterator i = m_vertices.begin(); i != m_vertices.end(); ++i)
	{
	  vector3_normalise(reinterpret_cast<Vector3&>((*i).tangent));
	  vector3_normalise(reinterpret_cast<Vector3&>((*i).bitangent));
	}
}

// Back-end render
void MD5Surface::render(RenderStateFlags state) const
{
  if((state & RENDER_BUMP) != 0)
  {
      glVertexAttribPointerARB(11, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_vertices.data()->normal);
      glVertexAttribPointerARB(8, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_vertices.data()->texcoord);
      glVertexAttribPointerARB(9, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_vertices.data()->tangent);
      glVertexAttribPointerARB(10, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_vertices.data()->bitangent);
  }
  else
  {
    glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &m_vertices.data()->normal);
    glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &m_vertices.data()->texcoord);
  }
  glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &m_vertices.data()->vertex);
  glDrawElements(GL_TRIANGLES, GLsizei(m_indices.size()), RenderIndexTypeID, m_indices.data());

}

// Selection test
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
