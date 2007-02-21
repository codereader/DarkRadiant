#ifndef PRIMITIVES_H_
#define PRIMITIVES_H_

#include <string>
#include <stdexcept>
#include <vector>

class Face;
class Patch;
typedef std::vector<Patch*> PatchPtrVector; 

namespace selection {
	
	/** greebo: This is thrown if some of the routines
	 * below fail to retrieve the requested selection.
	 */
	class InvalidSelectionException :
		public std::runtime_error
	{
	public:
		// Constructor
		InvalidSelectionException(const std::string& what):
			std::runtime_error(what) 
		{}     
	};
	
	namespace algorithm {

	/** greebo: Returns the number of the selected face instances.
	 */
	int selectedFaceCount();

	/** greebo: Retrieves the reference to the last selected face.
	 * 
	 * Throws an selection::InvalidSelectionException on failure.
	 * 
	 * @returns: the Face& reference of the last element.
	 */
	Face& getLastSelectedFace();
	
	/** greebo: Retrieves the first selected patch.
	 * 
	 * Throws an selection::InvalidSelectionException on failure.
	 * 
	 * @returns: a reference to the patch.
	 */
	Patch& getLastSelectedPatch();
	
	/** greebo: Retrieves a list of selected Patches, populated
	 * by a scene walker.
	 *
	 * @returns: the vector with the selected patch pointers.
	 */
	PatchPtrVector getSelectedPatches();

	} // namespace algorithm
} // namespace selection

#endif /*PRIMITIVES_H_*/
