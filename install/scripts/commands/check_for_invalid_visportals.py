"""Find any visportal brushes in the map with more than exactly one
   face assigned to the textures/editor/visportal shader.   

Tracker item: #4397 (http://bugs.thedarkmod.com/view.php?id=4397)
Author: greebo
Version: 1.0
"""

__commandName__ = 'check_invalid_visportals' # should not contain spaces
__commandDisplayName__ = 'Test for invalid Visportals '

def execute():

    # Returns 1 if the brush is a valid visportal, 0 otherwise
    def testVisportalBrush(brush, visportalShader):
        numVisportalFaces = 0
        
        for index in range(brush.getNumFaces()):
            if brush.getFace(index).getShader() == visportalShader:
                numVisportalFaces += 1
        
        if numVisportalFaces != 1:
            return 0 # brush is not OK
            
        return 1 # brush is ok

    class VisportalResults(object):
        numVisportals = 0
        invalidPortals = []
    
    results = VisportalResults()
    visportalShader = 'textures/editor/visportal'
    
    class VisportalChecker(SceneNodeVisitor):
        def pre(self, node):
            if node.isBrush():
                brush = node.getBrush()
                if brush.hasShader(visportalShader):
                    results.numVisportals += 1
                    
                    if not testVisportalBrush(brush, visportalShader):
                        results.invalidPortals.append(brush)
                return 0 # don't traverse brushes
            return 1
            
    # Instantiate a new walker object and check brushes
    walker = VisportalChecker()
    GlobalSceneGraph.root().traverse(walker)
    
    # Notify the user about the results
    msg = '%d visportals checked, %d have errors' % (results.numVisportals, len(results.invalidPortals)) + '\n\n'
    
    if results.numVisportals == 0:
        msg = 'There are no visportals in this map.'
    
    if len(results.invalidPortals) > 0:
        # Unselect everything in the scene before highlighting the problematic portals
        GlobalSelectionSystem.setSelectedAll(0)
        
        # Highlight the brushes one by one
        for brush in results.invalidPortals:
            brush.setSelected(1)
        
        msg += 'The problematic visportals have been highlighted.'
    
    GlobalDialogManager.createMessageBox('Visportal Test Results', msg, Dialog.CONFIRM).run()

if __executeCommand__:
    execute()
