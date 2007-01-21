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
	  if(self.m_texture != other.m_texture)
	  {
	    return self.m_texture < other.m_texture;
	  }
	  if(self.m_texture1 != other.m_texture1)
	  {
	    return self.m_texture1 < other.m_texture1;
	  }
	  if(self.m_texture2 != other.m_texture2)
	  {
	    return self.m_texture2 < other.m_texture2;
	  }
	  if(self.m_texture3 != other.m_texture3)
	  {
	    return self.m_texture3 < other.m_texture3;
	  }
	  if(self.m_texture4 != other.m_texture4)
	  {
	    return self.m_texture4 < other.m_texture4;
	  }
	  if(self.m_texture5 != other.m_texture5)
	  {
	    return self.m_texture5 < other.m_texture5;
	  }
	  if(self.m_texture6 != other.m_texture6)
	  {
	    return self.m_texture6 < other.m_texture6;
	  }
	  if(self.m_texture7 != other.m_texture7)
	  {
	    return self.m_texture7 < other.m_texture7;
	  }
	  //! Sort by state bit-vector.
	  if(self.m_state != other.m_state)
	  {
	    return self.m_state < other.m_state;
	  }
	  //! Comparing address makes sure states are never equal.
	  return &self < &other;
	  }
};



#endif /*OPENGLSTATELESS_H_*/
