#pragma once

#include "OpenGLState.h"

namespace render
{

/**
 * Comparison class for OpenGLState objects.
 */
struct OpenGLStateLess
{
	bool operator() (const OpenGLState* self, const OpenGLState* other) const
    {
	  //! Sort by sort-order override.
	  if(self->getSortPosition() != other->getSortPosition())
	  {
	    return self->getSortPosition() < other->getSortPosition();
	  }
	  //! Sort by texture handle.
	  if(self->texture0 != other->texture0)
	  {
	    return self->texture0 < other->texture0;
	  }
	  if(self->texture1 != other->texture1)
	  {
	    return self->texture1 < other->texture1;
	  }
	  if(self->texture2 != other->texture2)
	  {
	    return self->texture2 < other->texture2;
	  }
	  //! Sort by state bit-vector.
	  if(self->getRenderFlags() != other->getRenderFlags())
	  {
	    return self->getRenderFlags() < other->getRenderFlags();
	  }
	  //! Comparing address makes sure states are never equal.
	  return self < other;
    }
};

}
