#ifndef PATCHSAVEDSTATE_H_
#define PATCHSAVEDSTATE_H_

#include "PatchControl.h"

/* greebo: This is a structure that is allocated on the heap and contains all the state
 * information of a patch. This information is used by the UndoSystem to save the current
 * patch state and to revert it on request.
 */
class SavedState : public UndoMemento {
	public:
	
	// The members to store the state information
	std::size_t m_width, m_height;
	std::string m_shader;
	PatchControlArray m_ctrl;
	bool m_patchDef3;
	std::size_t m_subdivisions_x;
	std::size_t m_subdivisions_y;
	
	// Constructor
	SavedState(
		std::size_t width,
		std::size_t height,
		const PatchControlArray& ctrl,
		const std::string& shader,
		bool patchDef3,
		std::size_t subdivisions_x,
		std::size_t subdivisions_y
	) :
		m_width(width),
		m_height(height),
		m_shader(shader),
		m_ctrl(ctrl),
		m_patchDef3(patchDef3),
		m_subdivisions_x(subdivisions_x),
		m_subdivisions_y(subdivisions_y)
    {
    }

	// Delete this memento from the heap
    void release() {
		delete this;
    }
};

#endif /*PATCHSAVEDSTATE_H_*/
