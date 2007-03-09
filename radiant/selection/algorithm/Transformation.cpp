#include "Transformation.h"

#include <string>
#include "math/quaternion.h"
#include "iundo.h"
#include "iselection.h"
#include "gtkutil/dialog.h"
#include "mainframe.h"

namespace selection {
	namespace algorithm {
		
// greebo: see header for documentation
void rotateSelected(const Vector3& eulerXYZ) {
	std::string command("rotateSelectedEulerXYZ: ");
	command += eulerXYZ;
	UndoableCommand undo(command.c_str());

	GlobalSelectionSystem().rotateSelected(quaternion_for_euler_xyz_degrees(eulerXYZ));
}

// greebo: see header for documentation
void scaleSelected(const Vector3& scaleXYZ) {
	
	if (fabs(scaleXYZ[0]) > 0.0001f && 
		fabs(scaleXYZ[1]) > 0.0001f && 
		fabs(scaleXYZ[2]) > 0.0001f) 
	{
		std::string command("scaleSelected: ");
		command += scaleXYZ;
		UndoableCommand undo(command.c_str());
	
		GlobalSelectionSystem().scaleSelected(scaleXYZ);
	}
	else {
		gtkutil::errorDialog("Cannot scale by zero value.", MainFrame_getWindow());
	}
}
		
	} // namespace algorithm
} // namespace selection
