#pragma once

#include "OpenGLStateLess.h"

#include <map>

class OpenGLState;

namespace render
{

class OpenGLShaderPass;
typedef boost::shared_ptr<OpenGLShaderPass> OpenGLShaderPassPtr;

/**
 * A sorted map of OpenGL states, maintained by the OpenGLStateManager. The sort
 * function intelligently sorts OpenGL states in an order which will allow them
 * to rendered as efficiently as possible, reducing the number of unnecessary
 * context switches.
 */
typedef std::map<OpenGLState*,
				 OpenGLShaderPassPtr,
				 OpenGLStateLess> OpenGLStates;

/**
 * \brief
 * Interface for an object which can manage sorted GL states. This is
 * implemented by the OpenGLRenderSystem.
 */
class OpenGLStateManager
{
public:

    /**
     * \brief
     * Insert a new OpenGL state into the map.
     */
    virtual void insertSortedState(const OpenGLStates::value_type& val) = 0;

    /**
     * \brief
     * Remove a given OpenGL state from the map.
     */
    virtual void eraseSortedState(const OpenGLStates::key_type& key) = 0;

};


}

