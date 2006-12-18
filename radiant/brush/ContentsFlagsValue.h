#ifndef CONTENTSFLAGSVALUE_H_
#define CONTENTSFLAGSVALUE_H_

#include "generic/bitfield.h"

const unsigned int BRUSH_DETAIL_FLAG = 27;
const unsigned int BRUSH_DETAIL_MASK = (1 << BRUSH_DETAIL_FLAG);

class ContentsFlagsValue {
public:
	ContentsFlagsValue() {}
	
	ContentsFlagsValue(int surfaceFlags, int contentFlags, int value, bool specified) :
		m_surfaceFlags(surfaceFlags),
		m_contentFlags(contentFlags),
		m_value(value),
		m_specified(specified)
	{}
	
	void assignMasked(const ContentsFlagsValue& other) {
		
		bool detail = bitfield_enabled(m_contentFlags, BRUSH_DETAIL_MASK);
		
		*this = other;
		
		if (detail) {
		    m_contentFlags = bitfield_enable(m_contentFlags, BRUSH_DETAIL_MASK);
		}
		else {
			m_contentFlags = bitfield_disable(m_contentFlags, BRUSH_DETAIL_MASK);
		}
	}
	
	int m_surfaceFlags;
	int m_contentFlags;
	int m_value;
	bool m_specified;
}; // class ContentsFlagsValue

/* greebo: this is the old assignMasked() function, I moved it into the ContentsFlagsValue class,
 * but left the old code here, in case there is a problem with the assignment and one needs the original code
 * 
 * inline void ContentsFlagsValue_assignMasked(ContentsFlagsValue& flags, const ContentsFlagsValue& other)
{
  bool detail = bitfield_enabled(flags.m_contentFlags, BRUSH_DETAIL_MASK);
  flags = other;
  if(detail)
  {
    flags.m_contentFlags = bitfield_enable(flags.m_contentFlags, BRUSH_DETAIL_MASK);
  }
  else
  {
    flags.m_contentFlags = bitfield_disable(flags.m_contentFlags, BRUSH_DETAIL_MASK);
  }
}*/

#endif /*CONTENTSFLAGSVALUE_H_*/
