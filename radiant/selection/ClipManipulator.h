#pragma once

#include "debugging/debugging.h"
#include "Manipulator.h"

class ClipManipulator : public selection::Manipulator
{
public:
  ManipulatorComponent* getActiveComponent() {
    ERROR_MESSAGE("clipper is not manipulatable");
    return 0;
  }

  Type getType() const
  {
	  return Clip;
  }

  void setSelected(bool select) {}
  bool isSelected() const {
    return false;
  }
}; // class ClipManipulator

