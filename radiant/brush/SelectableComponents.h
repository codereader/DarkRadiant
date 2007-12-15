#ifndef SELECTABLECOMPONENTS_H_
#define SELECTABLECOMPONENTS_H_

#include "Winding.h"
#include "Face.h"

class FaceVertexId {
	std::size_t m_face;
	std::size_t m_vertex;

public:
	FaceVertexId(std::size_t face, std::size_t vertex)
		: m_face(face), m_vertex(vertex)
	{}

	std::size_t getFace() const {
		return m_face;
	}
	
	std::size_t getVertex() const {
		return m_vertex;
	}
};

class SelectableEdge
{
  Vector3 getEdge() const
  {
    const Winding& winding = getFace().getWinding();
    return vector3_mid(winding[m_faceVertex.getVertex()].vertex, winding[winding.next(m_faceVertex.getVertex())].vertex);
  }

public:
  Faces& m_faces;
  FaceVertexId m_faceVertex;

  SelectableEdge(Faces& faces, FaceVertexId faceVertex)
    : m_faces(faces), m_faceVertex(faceVertex)
  {
  }
  SelectableEdge& operator=(const SelectableEdge& other)
  {
    m_faceVertex = other.m_faceVertex;
    return *this;
  }

  Face& getFace() const
  {
    return *m_faces[m_faceVertex.getFace()];
  }

  void testSelect(SelectionTest& test, SelectionIntersection& best)
  {
    test.TestPoint(getEdge(), best);
  }
};

class SelectableVertex
{
  Vector3 getVertex() const
  {
    return getFace().getWinding()[m_faceVertex.getVertex()].vertex;
  }

public:
  Faces& m_faces;
  FaceVertexId m_faceVertex;

  SelectableVertex(Faces& faces, FaceVertexId faceVertex)
    : m_faces(faces), m_faceVertex(faceVertex)
  {
  }
  SelectableVertex& operator=(const SelectableVertex& other)
  {
    m_faceVertex = other.m_faceVertex;
    return *this;
  }

  Face& getFace() const
  {
    return *m_faces[m_faceVertex.getFace()];
  }

  void testSelect(SelectionTest& test, SelectionIntersection& best)
  {
    test.TestPoint(getVertex(), best);
  }
};

#endif /*SELECTABLECOMPONENTS_H_*/
