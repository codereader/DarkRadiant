#ifndef VERTEXINSTANCE_H_
#define VERTEXINSTANCE_H_

#include "FaceInstance.h"

namespace brush {

class VertexInstance :
	public ISelectable
{
	FaceInstances& m_faceInstances;
	SelectableVertex* m_vertex;

	void select_vertex(bool select) {
		FaceVertexId faceVertex = m_vertex->m_faceVertex;
		do {
			m_faceInstances[faceVertex.getFace()].select_vertex(faceVertex.getVertex(), select);
			faceVertex = next_vertex(m_vertex->m_faces, faceVertex);
		}
		while (faceVertex.getFace() != m_vertex->m_faceVertex.getFace());
	}

	bool selected_vertex() const {
		FaceVertexId faceVertex = m_vertex->m_faceVertex;
		do {
			if (!m_faceInstances[faceVertex.getFace()].selected_vertex(faceVertex.getVertex())) {
				return false;
			}
			faceVertex = next_vertex(m_vertex->m_faces, faceVertex);
		}
		while (faceVertex.getFace() != m_vertex->m_faceVertex.getFace());
		return true;
	}

public:
	VertexInstance(FaceInstances& faceInstances, SelectableVertex& vertex)
		: m_faceInstances(faceInstances), m_vertex(&vertex) {}

	VertexInstance& operator=(const VertexInstance& other) {
		m_vertex = other.m_vertex;
		return *this;
	}

	void setSelected(bool select) {
		select_vertex(select);
	}

	bool isSelected() const {
		return selected_vertex();
	}

	void invertSelected() {
		setSelected(!isSelected());
	}

	void testSelect(Selector& selector, SelectionTest& test) {
		SelectionIntersection best;
		m_vertex->testSelect(test, best);
		if (best.isValid()) {
			selector.addWithIntersection(*this, best);
		}
	}
};

} // namespace brush

#endif /*VERTEXINSTANCE_H_*/
