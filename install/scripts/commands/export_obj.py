# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# ***** END GPL LICENCE BLOCK *****

# Set the command name so that DarkRadiant recognises this file
__commandName__ = 'objExport'
__commandDisplayName__ = 'Export OBJ...'

# The actual algorithm called by DarkRadiant is contained in the execute() function
def execute():
    script = "DarkRadiant Wavefront OBJ Export (*.obj)"
    author = "Python port by greebo, based on original exporter C++ code in DarkRadiant and the ASE exporter scripts"
    version = "0.2"

    import darkradiant as dr

    # Check if we have a valid selection

    selectionInfo = GlobalSelectionSystem.getSelectionInfo()

    # Don't allow empty selections or selected components only
    if selectionInfo.totalCount == 0 or selectionInfo.totalCount == selectionInfo.componentCount:
        errMsg = GlobalDialogManager.createMessageBox('No selection', 'Nothing selected, cannot run exporter.', dr.Dialog.ERROR)
        errMsg.run()
        return

    # An exportable object found in the map
    class Geometry(object):
        name = ''           # Name of the object to be exported
        vertices = []       # Vertices
        texcoords = []      # Texture coordinates
        faces = []          # Each face in turn is an array of indices referencing the vertices and texcoords
                            # For simplicity, we assume that the referenced vertices and texcoords always
                            # have the same global index number.
        def __init__(self, name):
            self.name = name
            self.vertices = []
            self.texcoords = []
            self.faces = []

    # An exportable object found in the map
    class Geometries(object):
        vertexCount = 0     # Global Vertex Index (every vertex in an OBJ file has a unique number)
        objects = []        # List of Geometry objects

    # We put all of the objects we collect in the map into this array
    # Before we write it to the output stream the vertices can be processed (e.g. centered)
    geomlist = Geometries()
    
    def processBrush(brushnode):
        # Create a new exportable object 
        geometry = Geometry('Brush{0}'.format(len(geomlist.objects)))
        
        numfaces = brushnode.getNumFaces()
        for index in range(numfaces):
            facenode = brushnode.getFace(index)
            shader = facenode.getShader()

            # Tels: skip if caulk and no caulk should be exported
            if (shader == 'textures/common/caulk') and (int(GlobalRegistry.get('user/scripts/objExport/exportcaulk'))) == 0:
                continue

            winding = facenode.getWinding()
            
            # Remember the index of the first vertex
            firstVertex = geomlist.vertexCount

            for point in winding:
                # Write coordinates into the export buffers
                geometry.vertices.append(point.vertex)
                geometry.texcoords.append(point.texcoord)
                
                # Keep track of the exported vertices
                geomlist.vertexCount += 1
                
            # Append the face to the list, referencing vertices from firstVertex to the current count
            # Face indices are 1-based, so increase by 1 each time
			# Reverse the list to produce the correct face normal direction
            geometry.faces.append([i for i in reversed(range(firstVertex+1, geomlist.vertexCount+1))])

        print('Processed brush geometry: {0} verts and {1} faces'.format(len(geometry.vertices), len(geometry.faces)))
        geomlist.objects.append(geometry)
            
        return

    def processPatch(patchnode):
        shader = patchnode.getShader()

        # Tels: skip if caulk and no caulk should be exported
        if shader == 'textures/common/caulk' and int(GlobalRegistry.get('user/scripts/objExport/exportcaulk')) == 0:
            return

        # Create a new exportable object 
        geometry = Geometry('Patch{0}'.format(len(geomlist.objects)))

        mesh = patchnode.getTesselatedPatchMesh()
        
        # Remember the index of the first vertex
        firstVertex = geomlist.vertexCount
        
        for h in range(0, mesh.height):
            for w in range(0, mesh.width):
                point = mesh.vertices[mesh.width*h + w]

                # Write coordinates into the lists
                geometry.vertices.append(point.vertex)
                geometry.texcoords.append(point.texcoord)
                
                # Keep track of the exported vertices
                geomlist.vertexCount += 1

                # Starting from the second row, we're gathering the faces
                if h > 0 and w > 0:
                    # Gather the indices forming a quad
                    v1 = 1 + firstVertex + h*mesh.width + w;
                    v2 = 1 + firstVertex + (h-1)*mesh.width + w;
                    v3 = 1 + firstVertex + (h-1)*mesh.width + (w-1);
                    v4 = 1 + firstVertex + h*mesh.width + (w-1);

                    # Construct the quad
                    geometry.faces.append([v1, v4, v3, v2])
        
        print('Processed patch geometry: {0} verts and {1} faces'.format(len(geometry.vertices), len(geometry.faces)))
        geomlist.objects.append(geometry)
        
        return

    # Traversor class to visit child primitives of entities
    class nodeVisitor(dr.SceneNodeVisitor):
        def pre(self, scenenode):
            if scenenode.isBrush():
                processBrush(scenenode.getBrush())
            elif scenenode.isPatch():
                processPatch(scenenode.getPatch())

            # Traverse all child nodes, regardless of type
            return 1

    class SelectionWalker(dr.SelectionVisitor):
        def visit(self, scenenode):
            if scenenode.isBrush():
                processBrush(scenenode.getBrush())
            elif scenenode.isPatch():
                processPatch(scenenode.getPatch())
            elif scenenode.isEntity():
                # greebo: Found an entity, this could be a func_static or similar
                # Traverse children of this entity using a new walker
                nodewalker = nodeVisitor()
                scenenode.traverse(nodewalker)
            else:
                print('WARNING: unsupported node type selected. Skipping: ' + scenenode.getNodeType())

    # Dialog
    dialog = GlobalDialogManager.createDialog(script + ' v' + version)

    # Add an entry box and remember the handle
    fileHandle = dialog.addEntryBox("Filename:")
    dialog.setElementValue(fileHandle, GlobalRegistry.get('user/scripts/objExport/recentFilename'))

    # Add an entry box and remember the handle
    pathHandle = dialog.addPathEntry("Save path:", True)
    dialog.setElementValue(pathHandle, GlobalRegistry.get('user/scripts/objExport/recentPath'))

    # Add a checkbox
    centerObjectsHandle = dialog.addCheckbox("Center objects at 0,0,0 origin")
    dialog.setElementValue(centerObjectsHandle, GlobalRegistry.get('user/scripts/objExport/centerObjects'))

    # Add another checkbox
    exportCaulkHandle = dialog.addCheckbox("Export caulked faces")
    dialog.setElementValue(exportCaulkHandle, GlobalRegistry.get('user/scripts/objExport/exportcaulk'))

    if dialog.run() == dr.Dialog.OK:
        fullpath = dialog.getElementValue(pathHandle) + '/' + dialog.getElementValue(fileHandle)
        if not fullpath.endswith('.obj'):
            fullpath = fullpath + '.obj'

        # Save the path for later use
        GlobalRegistry.set('user/scripts/objExport/recentFilename', dialog.getElementValue(fileHandle))
        GlobalRegistry.set('user/scripts/objExport/recentPath', dialog.getElementValue(pathHandle))
        GlobalRegistry.set('user/scripts/objExport/centerObjects', dialog.getElementValue(centerObjectsHandle))
        GlobalRegistry.set('user/scripts/objExport/exportcaulk', dialog.getElementValue(exportCaulkHandle))

        try:
            file = open(fullpath, 'r')
            file.close()
            prompt = GlobalDialogManager.createMessageBox('Warning', 'The file ' + fullpath + ' already exists. Do you wish to overwrite it?', dr.Dialog.ASK)
            if prompt.run() == dr.Dialog.YES:
                overwrite = True
            else:
                overwrite = False
        except IOError:
            overwrite = True

        if overwrite:
            walker = SelectionWalker()
            GlobalSelectionSystem.foreachSelected(walker)

            # greebo: Check if we should center objects at the 0,0,0 origin
            if int(dialog.getElementValue(centerObjectsHandle)) == 1:
                # center objects at 0,0,0
                xlist = []
                ylist = []
                zlist = []
                for item in geomlist.objects:
                    for vert in item.vertices:
                        xlist.append(vert.x())
                        ylist.append(vert.y())
                        zlist.append(vert.z())
                xcenter=(max(xlist)+min(xlist))/2
                ycenter=(max(ylist)+min(ylist))/2
                zcenter=(max(zlist)+min(zlist))/2
                for item in geomlist.objects:
                    for vert in item.vertices:
                        vert -= dr.Vector3(xcenter, ycenter, zcenter)

            # This string will hold our export data
            data = str()

            for x in geomlist.objects:
                objstr = str()
            
                # Group name (g) 
                objstr = objstr + "g {0}\n\n".format(x.name)
                
                # Vertex list (v)
                for vert in x.vertices:
                    objstr = objstr + "v {0} {1} {2}\n".format(vert.x(), vert.y(), vert.z())
                
                objstr = objstr + "\n"
                    
                # Texture coord list (vt)
                for texcoord in x.texcoords:
                    objstr = objstr + "vt {0} {1}\n".format(texcoord.x(), 1-texcoord.y()) # reverse V coord
                    
                objstr = objstr + "\n"
                    
                # Face list (f)
                for faceIndices in x.faces:
                    objstr = objstr + "f"
                    for faceIndex in faceIndices:
                        objstr = objstr + " {0}/{1}".format(faceIndex, faceIndex)
                    objstr = objstr + "\n"
                
                objstr = objstr + "\n"
                data = data + objstr
                        
            # Write the compiled data to the output file
            file = open(fullpath, 'w')
            file.write(data)
            file.close()
            
            print('Done writing OBJ data to {0}'.format(fullpath))

# __executeCommand__ evaluates to true after DarkRadiant has successfully initialised
if __executeCommand__:
    execute()

