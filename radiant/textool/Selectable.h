#ifndef TEXTOOL_SELECTABLE_H_
#define TEXTOOL_SELECTABLE_H_

namespace selection {
	namespace TexTool {

class Selectable 
{
public:
	/** greebo: Tests if this can be selected at the given s/t coordinates.
	 * 
	 * @returns: TRUE if the selectable corresponds to 
	 * 			 the given coords, FALSE otherwise. 
	 */
	virtual bool testSelect(const float s, const float& t) = 0;
};

	} // namespace TexTool
} // namespace selection

#endif /*TEXTOOL_SELECTABLE_H_*/
