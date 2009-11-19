/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "select.h"

#include "debugging/debugging.h"

#include "ientity.h"
#include "iselection.h"
#include "iundo.h"

#include <vector>

#include "signal/isignal.h"
#include "shaderlib.h"
#include "scenelib.h"

#include "gtkutil/idledraw.h"
#include "gtkutil/dialog.h"
#include "gtkutil/widget.h"
#include "brushmanip.h"
#include "patchmanip.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "igrid.h"
#include "selection/SceneWalkers.h"
#include "xyview/GlobalXYWnd.h"
#include "selection/algorithm/General.h"

void Select_FlipAxis (int axis)
{
  Vector3 flip(1, 1, 1);
  flip[axis] = -1;
  GlobalSelectionSystem().scaleSelected(flip);
}


enum axis_t
{
  eAxisX = 0,
  eAxisY = 1,
  eAxisZ = 2,
};

enum sign_t
{
  eSignPositive = 1,
  eSignNegative = -1,
};

inline Matrix4 matrix4_rotation_for_axis90(axis_t axis, sign_t sign)
{
  switch(axis)
  {
  case eAxisX:
    if(sign == eSignPositive)
    {
      return matrix4_rotation_for_sincos_x(1, 0);
    }
    else
    {
      return matrix4_rotation_for_sincos_x(-1, 0);
    }
  case eAxisY:
    if(sign == eSignPositive)
    {
      return matrix4_rotation_for_sincos_y(1, 0);
    }
    else
    {
      return matrix4_rotation_for_sincos_y(-1, 0);
    }
  default://case eAxisZ:
    if(sign == eSignPositive)
    {
      return matrix4_rotation_for_sincos_z(1, 0);
    }
    else
    {
      return matrix4_rotation_for_sincos_z(-1, 0);
    }
  }
}

inline Quaternion quaternion_for_axis90(axis_t axis, sign_t sign)
{
#if 1
  switch(axis)
  {
  case eAxisX:
    if(sign == eSignPositive)
    {
      return Quaternion(c_half_sqrt2f, 0, 0, c_half_sqrt2f);
    }
    else
    {
      return Quaternion(-c_half_sqrt2f, 0, 0, -c_half_sqrt2f);
    }
  case eAxisY:
    if(sign == eSignPositive)
    {
      return Quaternion(0, c_half_sqrt2f, 0, c_half_sqrt2f);
    }
    else
    {
      return Quaternion(0, -c_half_sqrt2f, 0, -c_half_sqrt2f);
    }
  default://case eAxisZ:
    if(sign == eSignPositive)
    {
      return Quaternion(0, 0, c_half_sqrt2f, c_half_sqrt2f);
    }
    else
    {
      return Quaternion(0, 0, -c_half_sqrt2f, -c_half_sqrt2f);
    }
  }
#else
  quaternion_for_matrix4_rotation(matrix4_rotation_for_axis90((axis_t)axis, (deg > 0) ? eSignPositive : eSignNegative));
#endif
}

void Select_RotateAxis (int axis, float deg)
{
  if(fabs(deg) == 90.f)
  {
    GlobalSelectionSystem().rotateSelected(quaternion_for_axis90((axis_t)axis, (deg > 0) ? eSignPositive : eSignNegative));
  }
  else
  {
    switch(axis)
    {
    case 0:
      GlobalSelectionSystem().rotateSelected(quaternion_for_matrix4_rotation(matrix4_rotation_for_x_degrees(deg)));
      break;
    case 1:
      GlobalSelectionSystem().rotateSelected(quaternion_for_matrix4_rotation(matrix4_rotation_for_y_degrees(deg)));
      break;
    case 2:
      GlobalSelectionSystem().rotateSelected(quaternion_for_matrix4_rotation(matrix4_rotation_for_z_degrees(deg)));
      break;
    }
  }
}

void Selection_Flipx(const cmd::ArgumentList& args)
{
  UndoableCommand undo("mirrorSelected -axis x");
  Select_FlipAxis(0);
}

void Selection_Flipy(const cmd::ArgumentList& args)
{
  UndoableCommand undo("mirrorSelected -axis y");
  Select_FlipAxis(1);
}

void Selection_Flipz(const cmd::ArgumentList& args)
{
  UndoableCommand undo("mirrorSelected -axis z");
  Select_FlipAxis(2);
}

void Selection_Rotatex(const cmd::ArgumentList& args)
{
  UndoableCommand undo("rotateSelected -axis x -angle -90");
  Select_RotateAxis(0,-90);
}

void Selection_Rotatey(const cmd::ArgumentList& args)
{
  UndoableCommand undo("rotateSelected -axis y -angle 90");
  Select_RotateAxis(1, 90);
}

void Selection_Rotatez(const cmd::ArgumentList& args)
{
  UndoableCommand undo("rotateSelected -axis z -angle -90");
  Select_RotateAxis(2,-90);
}



void Nudge(int nDim, float fNudge)
{
  Vector3 translate(0, 0, 0);
  translate[nDim] = fNudge;
  
  GlobalSelectionSystem().translateSelected(translate);
}

void Selection_NudgeZ(float amount)
{
  std::ostringstream command;
  command << "nudgeSelected -axis z -amount " << amount;
  UndoableCommand undo(command.str());

  Nudge(2, amount);
}

void Selection_MoveDown(const cmd::ArgumentList& args)
{
  Selection_NudgeZ(-GlobalGrid().getGridSize());
}

void Selection_MoveUp(const cmd::ArgumentList& args)
{
  Selection_NudgeZ(GlobalGrid().getGridSize());
}
