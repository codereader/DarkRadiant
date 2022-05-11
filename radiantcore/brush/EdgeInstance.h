#ifndef EDGEINSTANCE_H_
#define EDGEINSTANCE_H_

#include "FaceInstance.h"

class EdgeInstance : public ISelectable {
	FaceInstances& m_faceInstances;
	SelectableEdge* m_edge;

	void select_edge(bool select) {
		FaceVertexId faceVertex = m_edge->m_faceVertex;
		m_faceInstances[faceVertex.getFace()].select_edge(faceVertex.getVertex(), select);
		faceVertex = next_edge(m_edge->m_faces, faceVertex);
		m_faceInstances[faceVertex.getFace()].select_edge(faceVertex.getVertex(), select);
	}

	bool selected_edge() const {
		FaceVertexId faceVertex = m_edge->m_faceVertex;
		if (!m_faceInstances[faceVertex.getFace()].selected_edge(faceVertex.getVertex())) {
			return false;
		}
		faceVertex = next_edge(m_edge->m_faces, faceVertex);
		if (!m_faceInstances[faceVertex.getFace()].selected_edge(faceVertex.getVertex())) {
			return false;
		}

		return true;
	}

public:
	EdgeInstance(FaceInstances& faceInstances, SelectableEdge& edge)
			: m_faceInstances(faceInstances), m_edge(&edge) {}

	EdgeInstance& operator=(const EdgeInstance& other) {
		m_edge = other.m_edge;
		return *this;
	}

	virtual ~EdgeInstance() {}

	void setSelected(bool select) {
		select_edge(select);
	}

	bool isSelected() const {
		return selected_edge();
	}

	void invertSelected() {
		setSelected(!isSelected());
	}

	void testSelect(Selector& selector, SelectionTest& test) {
		SelectionIntersection best;
		m_edge->testSelect(test, best);
		if (best.isValid()) {
			selector.addWithIntersection(*this, best);
		}
	}
};

#endif /*EDGEINSTANCE_H_*/
