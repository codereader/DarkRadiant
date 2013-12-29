#pragma once

#include "debugging/debugging.h"

class ClipManipulator : public Manipulator
{
public:
  ManipulatorComponent* getActiveComponent() {
    ERROR_MESSAGE("clipper is not manipulatable");
    return 0;
  }

  void setSelected(bool select) {}
  bool isSelected() const {
    return false;
  }
}; // class ClipManipulator

