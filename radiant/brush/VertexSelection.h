#ifndef VERTEXSELECTION_H_
#define VERTEXSELECTION_H_

#include <list>

typedef std::list<std::size_t> VertexSelection;

inline VertexSelection::iterator VertexSelection_find(VertexSelection& self, std::size_t value) {
	return std::find(self.begin(), self.end(), value);
}

inline VertexSelection::const_iterator VertexSelection_find(const VertexSelection& self, std::size_t value) {
	return std::find(self.begin(), self.end(), value);
}

inline VertexSelection::iterator VertexSelection_insert(VertexSelection& self, std::size_t value) {
	VertexSelection::iterator i = VertexSelection_find(self, value);
	
	if (i == self.end()) {
		self.push_back(value);
		return --self.end();
	}
	
	return i;
}

inline void VertexSelection_erase(VertexSelection& self, std::size_t value) {
	VertexSelection::iterator i = VertexSelection_find(self, value);
	
	if (i != self.end()) {
		self.erase(i);
	}
}

#endif /*VERTEXSELECTION_H_*/
