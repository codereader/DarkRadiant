#ifndef PATCHCONTROL_H_
#define PATCHCONTROL_H_

#include "ipatch.h"

// greebo: An array containing patchcontrols (doh!) used to store the control vertices in the Patch class
typedef std::vector<PatchControl> PatchControlArray;

// greebo: The types to cycle through a patchcontrol array/list/matrix/whatever
typedef PatchControlArray::iterator PatchControlIter;
typedef PatchControlArray::const_iterator PatchControlConstIter;

// Copy all the controls from another ControlArray (from <begin> to <end>) to the ControlArray <ctrl>
inline void copy_ctrl(PatchControlIter ctrl, PatchControlConstIter begin, PatchControlConstIter end)
{
	std::copy(begin, end, ctrl);
}

inline void PatchControlArray_invert(PatchControlArray& ctrl, std::size_t width, std::size_t height)
{
	PatchControlArray tmp(width);

	PatchControlIter from = ctrl.begin() + (width * (height - 1));
	PatchControlIter to = ctrl.begin();

  for(std::size_t h = 0; h != ((height - 1) >> 1); ++h, to += width, from -= width)
  {
    copy_ctrl(tmp.begin(), to, to + width);
    copy_ctrl(to, from, from + width);
    copy_ctrl(from, tmp.begin(), tmp.begin() + width);
  }
}

#endif /*PATCHCONTROL_H_*/
