#include "PatchInterface.h"

#include "ipatch.h"

namespace script {

class ScriptPatchNode :
	public ScriptSceneNode
{
public:
	ScriptPatchNode(const scene::INodePtr& node) :
		ScriptSceneNode((node != NULL && Node_isPatch(node)) ? node : scene::INodePtr())
	{}

	// Checks if the given SceneNode structure is a PatchNode
	static bool isPatch(const ScriptSceneNode& node) {
		return Node_isPatch(node);
	}

	// "Cast" service for Python, returns a ScriptPatchNode. 
	// The returned node is non-NULL if the cast succeeded
	static ScriptPatchNode getPatch(const ScriptSceneNode& node) {
		// Try to cast the node onto a patch
		IPatchNodePtr patchNode = boost::dynamic_pointer_cast<IPatchNode>(
			static_cast<scene::INodePtr>(node)
		);
		
		// Construct a patchNode (contained node may be NULL)
		return (patchNode != NULL) ? ScriptPatchNode(node) : ScriptPatchNode(scene::INodePtr());
	}

	//// Resizes the patch to the given dimensions
	//void setDims(std::size_t width, std::size_t height) 
	//{
	//	scene::INodePtr node = _node.lock();
	//	if (node == NULL) return;

	//	IPatch 
	//}
	//
	//// Get the patch dimensions
	//virtual std::size_t getWidth() const = 0;
	//virtual std::size_t getHeight() const = 0;

	//// Return a defined patch control vertex at <row>,<col>
	//virtual PatchControl& ctrlAt(std::size_t row, std::size_t col) = 0;
	//virtual const PatchControl& ctrlAt(std::size_t row, std::size_t col) const = 0;

	///** 
	// * greebo: Inserts two columns before and after the column with index <colIndex>.
 //	 * Throws an GenericPatchException if an error occurs.
 //	 */
 //	virtual void insertColumns(std::size_t colIndex) = 0;
 //	
 //	/** 
	// * greebo: Inserts two rows before and after the row with index <rowIndex>.
 //	 * Throws an GenericPatchException if an error occurs.
 //	 */
 //	virtual void insertRows(std::size_t rowIndex) = 0;

	///** 
	// * greebo: Removes columns or rows right before and after the col/row 
 //	 * with the given index, reducing the according dimension by 2.
 //	 */
 //	virtual void removePoints(bool columns, std::size_t index) = 0;
 //	
 //	/** 
	// * greebo: Appends two rows or columns at the beginning or the end.
 //	 */
 //	virtual void appendPoints(bool columns, bool beginning) = 0;

	//// Check if the patch has invalid control points or width/height are zero
	//virtual bool isValid() const = 0;

	//// Check whether all control vertices are in the same 3D spot (with minimal tolerance)
	//virtual bool isDegenerate() const = 0;

	//// Shader handling
	//virtual const std::string& getShader() const = 0;
	//virtual void setShader(const std::string& name) = 0;

	///** 
	// * greebo: Sets/gets whether this patch is a patchDef3 (fixed tesselation)
	// */
	//virtual bool subdivionsFixed() const = 0;
	//
	///** greebo: Returns the x,y subdivision values (for tesselation)
	// */
	//virtual Subdivisions getSubdivisions() const = 0;
	//
	///** greebo: Sets the subdivision of this patch
	// * 
	// * @isFixed: TRUE, if this patch should be a patchDef3 (fixed tesselation)
	// * @divisions: a two-component vector containing the desired subdivisions
	// */
	//virtual void setFixedSubdivisions(bool isFixed, const Subdivisions& divisions) = 0;
};

ScriptSceneNode PatchInterface::createPatchDef2() {
	// Create a new patch and return the script scene node
	return ScriptSceneNode(GlobalPatchCreator(DEF2).createPatch());
}

ScriptSceneNode PatchInterface::createPatchDef3() {
	// Create a new patch and return the script scene node
	return ScriptSceneNode(GlobalPatchCreator(DEF3).createPatch());
}

void PatchInterface::registerInterface(boost::python::object& nspace) {
	// Define a PatchNode interface
	nspace["PatchNode"] = boost::python::class_<ScriptPatchNode, 
		boost::python::bases<ScriptSceneNode> >("PatchNode", boost::python::init<const scene::INodePtr&>() )
		//.def("getNumFaces", &ScriptPatchNode::getNumFaces)
	;

	// Add the "isPatch" and "getPatch" method to all ScriptSceneNodes
	boost::python::object sceneNode = nspace["SceneNode"];

	boost::python::objects::add_to_namespace(sceneNode, 
		"isPatch", boost::python::make_function(&ScriptPatchNode::isPatch));

	boost::python::objects::add_to_namespace(sceneNode, 
		"getPatch", boost::python::make_function(&ScriptPatchNode::getPatch));
	
	// Define the GlobalPatchCreator interface
	nspace["GlobalPatchCreator"] = boost::python::class_<PatchInterface>("GlobalPatchCreator")
		.def("createPatchDef2", &PatchInterface::createPatchDef2)
		.def("createPatchDef3", &PatchInterface::createPatchDef3)
	;

	// Now point the Python variable "GlobalPatchCreator" to this instance
	nspace["GlobalPatchCreator"] = boost::python::ptr(this);
}

} // namespace script
