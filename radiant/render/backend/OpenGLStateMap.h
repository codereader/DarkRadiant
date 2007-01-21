#ifndef OPENGLSTATEMAP_H_
#define OPENGLSTATEMAP_H_

#include "renderstate.h"
#include "iglrender.h"

#include <map>

class OpenGLStateMap 
: public OpenGLStateLibrary
{
	// Map of named states
	typedef std::map<std::string, OpenGLState> States;
	States _states;

public:

  typedef States::iterator iterator;
  iterator begin()
  {
    return _states.begin();
  }
  iterator end()
  {
    return _states.end();
  }

	/**
	 * Construct a default OpenGLState in the provided reference.
	 */
	void getDefaultState(OpenGLState& state) const {
		state = OpenGLState();
	}

	void insert(const std::string& name, const OpenGLState& state) {
    	_states.insert(States::value_type(name, state)).second;
	}

	void erase(const std::string& name) {
		_states.erase(name);
	}

	virtual const OpenGLState& find(const std::string& name) {
		States::iterator i = _states.find(name);
		if (i != _states.end())
			return i->second;
		else
			throw std::runtime_error("OpenGLStateMap: " + name + " not found.");
	}
};

#endif /*OPENGLSTATEMAP_H_*/
