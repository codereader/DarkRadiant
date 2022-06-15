#pragma once

#include "OpenGLState.h"

namespace render
{

// Compare two Colour4 values
inline bool Colour4_less(const Colour4& a, const Colour4& b)
{
    if (a.x() != b.x()) return a.x() < b.x();
    if (a.y() != b.y()) return a.y() < b.y();
    if (a.z() != b.z()) return a.z() < b.z();
    if (a.w() != b.w()) return a.w() < b.w();

    return false;
}

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
      // Sort brighter colours on top if all of the above are equivalent
      if (self->getColour() != other->getColour())
      {
          return Colour4_less(self->getColour(), other->getColour());
      }
	  //! Comparing address makes sure states are never equal.
	  return self < other;
    }
};

}
