#ifndef OPENGLSTATELESS_H_
#define OPENGLSTATELESS_H_

#include "iglrender.h"

/**
 * Comparison class for OpenGLState objects.
 */
struct OpenGLStateLess
{
	bool operator() (const OpenGLState& self, const OpenGLState& other) const
    {
	  //! Sort by sort-order override.
	  if(self.m_sort != other.m_sort)
	  {
	    return self.m_sort < other.m_sort;
	  }
	  //! Sort by texture handle.
	  if(self.texture0 != other.texture0)
	  {
	    return self.texture0 < other.texture0;
	  }
	  if(self.texture1 != other.texture1)
	  {
	    return self.texture1 < other.texture1;
	  }
	  if(self.texture2 != other.texture2)
	  {
	    return self.texture2 < other.texture2;
	  }
	  //! Sort by state bit-vector.
	  if(self.renderFlags != other.renderFlags)
	  {
	    return self.renderFlags < other.renderFlags;
	  }
	  //! Comparing address makes sure states are never equal.
	  return &self < &other;
    }
};



#endif /*OPENGLSTATELESS_H_*/
