#ifndef PATCHCONTROL_H_
#define PATCHCONTROL_H_

#include "ipatch.h"
#include "container/array.h"

// greebo: The types to cycle through a patchcontrol array/list/matrix/whatever
typedef PatchControl* PatchControlIter;
typedef const PatchControl* PatchControlConstIter;

// greebo: An array containing patchcontrols (doh!) used to store the control vertices in the Patch class
typedef Array<PatchControl> PatchControlArray;

// Copy all the controls from another ControlArray (from <begin> to <end>) to the ControlArray <ctrl>
inline void copy_ctrl(PatchControlIter ctrl, PatchControlConstIter begin, PatchControlConstIter end) {
  std::copy(begin, end, ctrl);
}

inline void PatchControlArray_invert(Array<PatchControl>& ctrl, std::size_t width, std::size_t height) {
  Array<PatchControl> tmp(width);

  PatchControlIter from = ctrl.data() + (width * (height - 1));
  PatchControlIter to = ctrl.data();
  for(std::size_t h = 0; h != ((height - 1) >> 1); ++h, to += width, from -= width)
  {
    copy_ctrl(tmp.data(), to, to + width);
    copy_ctrl(to, from, from + width);
    copy_ctrl(from, tmp.data(), tmp.data() + width);
  }
}

#endif /*PATCHCONTROL_H_*/
