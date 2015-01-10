#pragma once

#include "PatchControl.h"

/* greebo: This is a structure that is allocated on the heap and contains all the state
 * information of a patch. This information is used by the UndoSystem to save the current
 * patch state and to revert it on request.
 */
class SavedState : 
	public IUndoMemento
{
public:
	// The members to store the state information
	std::size_t m_width, m_height;
	PatchControlArray m_ctrl;
	bool m_patchDef3;
	std::size_t m_subdivisions_x;
	std::size_t m_subdivisions_y;
    std::string _materialName;

	// Constructor
	SavedState(
		std::size_t width,
		std::size_t height,
		const PatchControlArray& ctrl,
		bool patchDef3,
		std::size_t subdivisions_x,
		std::size_t subdivisions_y,
        const std::string& materialName
	) :
		m_width(width),
		m_height(height),
		m_ctrl(ctrl),
		m_patchDef3(patchDef3),
		m_subdivisions_x(subdivisions_x),
		m_subdivisions_y(subdivisions_y),
        _materialName(materialName)
    {}
};
