__commandName__ = 'SplitPatch'
__commandDisplayName__ = 'Split patch'

def execute():
    MAXGRIDPOWER =  8   # Grid 256
    MINGRIDPOWER = -1   # Grid 0.5

    class UserError(Exception): pass
    class TooManyVerts(Exception): pass
    class TooFewVerts(Exception): pass

    class Vert:
        """Holds coords for one patch control vertex."""
        def __init__(self, vertex, texcoord):
            from darkradiant import Vector3, Vector2
            self.vertex = Vector3(vertex)
            self.texcoord = Vector2(texcoord)

    class Verts:
        """A 2d matrix (row/col) implemented as a list where each element is a 
        list of Vert objects in one row of the patch."""
        def __init__(self, patch=None):
            self.verts = []
            if patch:
                for rownum in range(patch.getHeight()):
                    row = []
                    for colnum in range(patch.getWidth()):
                        v = Vert(patch.ctrlAt(rownum, colnum).vertex, patch.ctrlAt(rownum, colnum).texcoord)
                        row.append(v)
                    self.verts.append(row)

    class PatchData:
        """Store all of the information needed to reconstruct a patch exactly."""
        def __init__(self, patch):
            self.cols = patch.getWidth()                      # int
            self.rows = patch.getHeight()                     # int
            self.verts = Verts(patch)
            self.shader = patch.getShader()                   # string
            self.subdivisionsFixed = patch.subdivionsFixed()  # bool
            self.subdivs = patch.getSubdivisions()            # Subdivisions

        def replaceverts(self, verts):
            self.verts = verts
            self.rows = len(verts.verts)
            self.cols = len(verts.verts[0])

        def getRows(self, first=0, last=None):
            """Return a Verts object holding the subset of the patch 
            mesh verts in the specified row range."""
            if not last:
                last = self.rows
            result = Verts()
            result.verts = self.verts.verts[first:last]
            return result

        def getCols(self, first=0, last=None):
            """Return a Verts object holding the subset of the patch 
            mesh verts in the specified column range."""
            if not last:
                last = self.cols
            result = Verts()
            result.verts = [r[first:last] for r in self.verts.verts]
            return result

        def getVertex(self, row, col):
            return self.verts.verts[row][col].vertex

        def getTexcoord(self, row, col):
            return self.verts.verts[row][col].texcoord

    # Utility functions
    def getAndValidateSelection():
        sInfo = GlobalSelectionSystem.getSelectionInfo()
        if sInfo.patchCount != 1 or sInfo.brushCount > 0 or sInfo.entityCount > 0 or sInfo.componentCount < 2:
            raise UserError('Bad selection. Select one patch only, and ' \
                            '2 or more verts from the same (pink) row or column.')
        patch = GlobalSelectionSystem.ultimateSelected().getPatch()
        if not patch.isValid():
            raise UserError("This isn't a valid patch. It has some invalid " \
                            "vertices, or maybe all vertices are in the same spot.")        
        #if not patch.subdivionsFixed():
            #raise UserError('You need to fix tesselation on the patch in the ' \
                            #'Patch Inspector (Shift+S) before splitting it.')
        return patch

    def resetPatch(patch, patchdata):
        """Apply patchdata to the patch, reconstructing it."""
        p, pd = patch, patchdata
        p.setDims(pd.cols, pd.rows)
        p.setShader(pd.shader)
        p.setFixedSubdivisions(pd.subdivisionsFixed, pd.subdivs)
        for row in range(pd.rows):
            for col in range(pd.cols):
                p.ctrlAt(row, col).vertex = pd.getVertex(row, col)
                p.ctrlAt(row, col).texcoord = pd.getTexcoord(row, col)
        p.controlPointsChanged()

    def clonePatch(patch):
        """Clone the existing patch so the new one is automatically 
        part of the same func_static and layer as the original. Return 
        a reference to the new patch."""
        patch.setSelected(0) # Clears vertex editing mode
        patch.setSelected(1)
        GlobalCommandSystem.execute('CloneSelection')
        return GlobalSelectionSystem.ultimateSelected().getPatch()
        
    # Procedures for identifying the selected verts.
    # They can't be accessed directly, so move them about to find out at what
    # distance they cross the bounding box of the patch. Then infer the 
    # selected verts' bounding box, and which row or col they sit on.
    def distToEdge(patch, direction, axis, gridPower):
        """Return distance from selected verts' bounding box to the edge 
        of the patch's bounding box. Make sure orthoview is facing right way 
        before calling.
        SIDE EFFECTS: Leaves selected verts displaced by a small amount. Grid 
        size is changed."""
        GlobalGrid.setGridSize(gridPower)
        stepsize = 2 ** gridPower
        reverse =      'right' if direction == 'left' \
                  else 'left'  if direction == 'right' \
                  else 'down'  if direction == 'up' \
                  else 'up' 
        origin = lambda: getattr(patch.getWorldAABB().origin, axis)()
        extent = lambda: getattr(patch.getWorldAABB().extents, axis)()
        if direction in ('right', 'up'):
            boundary = lambda: origin() + extent()
            comparison = '<='
        else:
            boundary = lambda: origin() - extent()
            comparison = '>='
        starting_boundary = boundary()
        looplimit = (extent() * 2) / stepsize # Safety valve. Don't let verts move further than the 
                                              # extent of the patch. Stops an infinite loop crashing 
                                              # DR if something goes wrong.
        stepcount = 0    
        while eval('boundary() %s starting_boundary' % comparison) and stepcount < looplimit:
            GlobalCommandSystem.execute('SelectNudge' + direction)
            stepcount += 1
        for undostep in range(stepcount):
            GlobalCommandSystem.execute('SelectNudge' + reverse)
        return (stepcount - 1) * stepsize
    
    def attemptGetVertsLine(patch, tolerance):
        """Return ('row', 4) or ('col', 2) etc.
        Designed to be called multiple times with different tolerance if necessary.
        SIDE EFFECTS: Leaves selected verts displaced by a small amount. Grid 
        size is changed."""
        from math import log
        AABB = patch.getWorldAABB()
        gridPower = int(log(tolerance, 2))
        GlobalCommandSystem.execute('ViewSide')
        minX = AABB.origin.x() - AABB.extents.x() + distToEdge(patch, 'left',  'x', gridPower) - tolerance
        maxX = AABB.origin.x() + AABB.extents.x() - distToEdge(patch, 'right', 'x', gridPower) + tolerance
        minZ = AABB.origin.z() - AABB.extents.z() + distToEdge(patch, 'down',  'z', gridPower) - tolerance
        maxZ = AABB.origin.z() + AABB.extents.z() - distToEdge(patch, 'up',    'z', gridPower) + tolerance
        GlobalCommandSystem.execute('ViewFront') 
        minY = AABB.origin.y() - AABB.extents.y() + distToEdge(patch, 'left',  'y', gridPower) - tolerance
        maxY = AABB.origin.y() + AABB.extents.y() - distToEdge(patch, 'right', 'y', gridPower) + tolerance
        GlobalCommandSystem.execute('ViewTop')    
        # Find which verts lie in the selected box
        includedRows = set()
        includedCols = set()
        for row in range(patch.getHeight()):
            for col in range(patch.getWidth()):
                vert = patch.ctrlAt(row, col)
                if minX <= vert.vertex.x() <= maxX and \
                   minY <= vert.vertex.y() <= maxY and \
                   minZ <= vert.vertex.z() <= maxZ:
                    includedRows.add(row)
                    includedCols.add(col)      
        # Interpret result
        # Special case: if the user selects the existing seam of a polyhedron like a sphere or cone:
        if includedRows == set((0, patch.getHeight()-1)) or includedCols == set((0, patch.getWidth()-1)):
            raise UserError("You've selected the existing seam of a 3d patch. It's already cut " \
                            "there, so no action has been taken.")
        if len(includedRows) > 1 and len(includedCols) > 1:
            raise TooManyVerts()
        elif len(includedRows) < 2 and len(includedCols) < 2:
            raise TooFewVerts()
        elif len(includedRows) == 1:
            return 'row', includedRows.pop()
        else:
            return 'col', includedCols.pop()        

    def getSelectedVertsLine(patch, patchdata):
        """Return ('row', 4) or ('col', 2) etc.
        Makes multiple attempts if needed.
        Mutates patch, so uses patchdata to restore state."""
        userGridPower = GlobalGrid.getGridPower()
        # Start with tolerance = the user's grid size
        tolerance = 2 ** userGridPower
        mintolerance = 2 ** MINGRIDPOWER
        maxtolerance = 2 ** MAXGRIDPOWER
        lineType, lineNum = None, None
        while (not lineType) and mintolerance <= tolerance <= maxtolerance:
            try:
                print('Patch Splitter: Trying to identify selected verts with tolerance %0.2f' % tolerance)
                lineType, lineNum = attemptGetVertsLine(patch, tolerance) # has side effects that can't be fixed till search \
            except TooManyVerts:                                          # \ is finished, as the fix clears the user's selection
                tolerance /= 2.0
            except TooFewVerts:
                tolerance *= 2
        GlobalGrid.setGridSize(userGridPower) # \ fix the side-effects
        resetPatch(patch, patchdata)          # /
        if not lineType:
            raise UserError('Unable to determine selected verts. Try again with different ' \
                            'verts from the same row/column. Make sure all the verts are ' \
                            'on the same line. Selecting 2 _pink_ verts might help with ' \
                            'very twisty patches.')
        if lineNum == 0 \
           or (lineType == 'row' and lineNum == patch.getHeight()-1) \
           or (lineType == 'col' and lineNum == patch.getWidth()-1):
            raise UserError("You've selected the existing edge of the patch. No action has been taken.")
        if lineNum % 2:
            raise UserError("You've selected a green line. Patches can be cut only along lines with some pink verts.")
        return lineType, lineNum

    # Execution starts here
    # STEP 1: Validate selection
    patch = getAndValidateSelection()
    patchdata = PatchData(patch)
    
    # STEP 2: Work out what row or column is selected
    lineType, lineNum = getSelectedVertsLine(patch, patchdata)
    print('RESULT: ', lineType, lineNum)
    
    # STEP 3: Split the patch
    newpatch = clonePatch(patch)
    try:
        if lineType == 'row':
            newverts1 = patchdata.getRows(last=lineNum+1)
            newverts2 = patchdata.getRows(first=lineNum)
        elif lineType == 'col':
            newverts1 = patchdata.getCols(last=lineNum+1)  
            newverts2 = patchdata.getCols(first=lineNum)
        # Make PatchData objects for the 2 new patches. Initialize them with 
        # either of the existing patches then swap out the verts list.
        newdata1, newdata2 = PatchData(newpatch), PatchData(newpatch)
        newdata1.replaceverts(newverts1)
        newdata2.replaceverts(newverts2)
        # Adjust the patches
        resetPatch(newpatch, newdata2)
        resetPatch(patch, newdata1)
    except Exception as e:
        # Clean up before reporting error
        GlobalSelectionSystem.setSelectedAll(False)
        newpatch.setSelected(True)
        GlobalCommandSystem.execute('deleteSelected')
        resetPatch(patch, patchdata)
        raise
    
    # Step 4: Success! leave the new patches selected
    patch.setSelected(True)
    newpatch.setSelected(True)

import darkradiant as dr

if __executeCommand__:    
    try:
        execute()
    except Exception as e:
        GlobalDialogManager.createMessageBox('Patch Splitter', str(e), dr.Dialog.ERROR).run()
