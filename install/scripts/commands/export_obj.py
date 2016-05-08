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
    version = "0.1"

    # Check if we have a valid selection

    selectionInfo = GlobalSelectionSystem.getSelectionInfo()

    # Don't allow empty selections or selected components only
    if selectionInfo.totalCount == 0 or selectionInfo.totalCount == selectionInfo.componentCount:
        errMsg = GlobalDialogManager.createMessageBox('No selection', 'Nothing selected, cannot run exporter.', Dialog.ERROR)
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
            geometry.faces.append([i for i in range(firstVertex+1, geomlist.vertexCount+1)])

        geomlist.objects.append(geometry)
        return

    def processPatch(patchnode):
        return # todo
        verts = []
        faces = []

        shader = patchnode.getShader()

        # Tels: skip if caulk and no caulk should be exported
        if shader == 'textures/common/caulk' and int(GlobalRegistry.get('user/scripts/objExport/exportcaulk')) == 0:
            return

        if not shader in shaderlist:
            shaderlist.append(shader)
        mesh = patchnode.getTesselatedPatchMesh()
        for x in mesh.vertices:
            verts.append([x.vertex.x(), x.vertex.y(), x.vertex.z(), x.texcoord.x(), x.texcoord.y() * -1, x.normal.x(), x.normal.y(), x.normal.z()])
        tris = skinmatrix([x for x in range(len(verts))], mesh.width, mesh.height)
        for x in tris:
            x.append(shaderlist.index(shader))
            faces.append(x)

        geomlist.append([verts, faces])
        return

    # Traversor class to visit child primitives of entities
    class nodeVisitor(SceneNodeVisitor):
        def pre(self, scenenode):
            # Brush?
            if scenenode.isBrush():
                processBrush(scenenode.getBrush())
            # Patch?
            elif scenenode.isPatch():
                processPatch(scenenode.getPatch())

            # Traverse all child nodes, regardless of type
            return 1

    class SelectionWalker(SelectionVisitor):
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

    if dialog.run() == Dialog.OK:
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
            prompt = GlobalDialogManager.createMessageBox('Warning', 'The file ' + fullpath + ' already exists. Do you wish to overwrite it?', Dialog.ASK)
            if prompt.run() == Dialog.YES:
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
                # TODO
                # center objects at 0,0,0
                xlist = []
                ylist = []
                zlist = []
                for item in geomlist:
                    for vert in item[0]:
                        xlist.append(vert[0])
                        ylist.append(vert[1])
                        zlist.append(vert[2])
                xcenter=(max(xlist)+min(xlist))/2
                ycenter=(max(ylist)+min(ylist))/2
                zcenter=(max(zlist)+min(zlist))/2
                for item in geomlist:
                    for vert in item[0]:
                        vert[0] = vert[0] - xcenter
                        vert[1] = vert[1] - ycenter
                        vert[2] = vert[2] - zcenter

            # This string will hold our export data
            data = str()

            for x in geomlist.objects:
                # Group name (g) 
                data = data + "g {0}\n\n".format(x.name)
                
                # Vertex list (v)
                for vert in x.vertices:
                    data = data + "v {0} {1} {2}\n".format(vert.x(), vert.y(), vert.z())
                
                data = data + "\n"
                    
                # Texture coord list (vt)
                for texcoord in x.texcoords:
                    data = data + "vt {0} {1}\n".format(texcoord.x(), texcoord.y())
                    
                data = data + "\n"
                    
                # Face list (f)
                for faceIndices in x.faces:
                    data = data + "f"
                    for faceIndex in faceIndices:
                        data = data + " {0}/{1}".format(faceIndex, faceIndex)
                    data = data + "\n"
                
                data = data + "\n"
                        
            # Write the compiled data to the output file
            file = open(fullpath, 'w')
            file.write(data)
            file.close()

# __executeCommand__ evaluates to true after DarkRadiant has successfully initialised
if __executeCommand__:
    execute()

