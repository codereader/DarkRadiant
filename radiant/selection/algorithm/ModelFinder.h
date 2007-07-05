#ifndef MODELFINDER_H_
#define MODELFINDER_H_

#include <vector>
#include "ipath.h"
#include "iselection.h"

// Forward declaration to avoid including the entire scenelib
namespace scene {
	class Instance;
} // namespace scene

namespace selection {
	namespace algorithm {
	
	// a Visitor that checks the current selection for models
	class ModelFinder : 
		public SelectionSystem::Visitor
	{
	public:
		typedef std::vector<scene::Path> ModelList;
		
	private:
		mutable ModelList _modelList;
		mutable bool _onlyModels;
	
	public:
		ModelFinder();
	
		/** greebo: Visits every selected instance and adds all
		 * 			models to the internal list
		 */
		void visit(scene::Instance& instance) const;
		
		/** greebo: Retrieves the result of the search
		 */
		ModelList& getList();
		
		/** greebo: Returns TRUE if no models were found.
		 */
		bool empty() const;
		
		/** greebo: Returns TRUE if ONLY models were found, no other
		 * 			objects like brushes, lights, etc.
		 */
		bool onlyModels() const;
	};
		
	} // namespace algorithm
} // namespace selection 

#endif /*MODELFINDER_H_*/
