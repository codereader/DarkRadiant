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

#TODO:

# Set the command name so that DarkRadiant recognises this file
__commandName__ = 'aseExport'
__commandDisplayName__ = 'Export ASE...'

# The actual algorithm called by DarkRadiant is contained in the execute() function
def execute():
    script = "Dark Radiant ASCII Scene Export (*.ase)"
    author = "Richard Bartlett, some additions by greebo and tels"
    version = "0.7"

    # Check if we have a valid selection

    selectionInfo = GlobalSelectionSystem.getSelectionInfo()

    # Don't allow empty selections or selected components only
    if selectionInfo.totalCount == 0 or selectionInfo.totalCount == selectionInfo.componentCount:
        errMsg = GlobalDialogManager.createMessageBox('No selection', 'Nothing selected, cannot run exporter.', Dialog.ERROR)
        errMsg.run()
        return

    shaderlist = []
    geomlist = []

    # simple linear triangulation of n-sided poly

    def triangulate(pointset):
        tris = []
        for count in range(1, len(pointset) - 1):
            tris.append([pointset[0], pointset[count], pointset[count + 1]])
        return tris

    # skin patch matrix with tris

    def skinmatrix(pointset, width, height):
        tris = []
        for h in range(height-1):
            for w in range(width-1):
                tris.append([pointset[w+(h*width)], pointset[w+1+(h*width)], pointset[w+width+(h*width)]])
                tris.append([pointset[w+1+(h*width)], pointset[w+1+width+(h*width)], pointset[w+width+(h*width)]])
        return tris

    def processBrush(brushnode):
        verts = []
        faces = []

        numfaces = brushnode.getNumFaces()
        for index in range(numfaces):
            facenode = brushnode.getFace(index)
            shader = facenode.getShader()

            # Tels: skip if caulk and no caulk should be exported
            if (shader == 'textures/common/caulk') and (int(GlobalRegistry.get('user/scripts/aseExport/exportcaulk'))) == 0:
                continue

            if not shader in shaderlist:
                shaderlist.append(shader)
            winding = facenode.getWinding()
            tris = triangulate([x+len(verts) for x in range(len(winding))])
            for x in tris:
                x.append(shaderlist.index(shader))
                faces.append(x)
            for x in reversed(winding):
                verts.append([x.vertex.x(), x.vertex.y(), x.vertex.z(), x.texcoord.x(), x.texcoord.y() * -1, x.normal.x(), x.normal.y(), x.normal.z()])

        geomlist.append([verts, faces])
        return

    def processPatch(patchnode):
        verts = []
        faces = []

        shader = patchnode.getShader()

        # Tels: skip if caulk and no caulk should be exported
        if shader == 'textures/common/caulk' and int(GlobalRegistry.get('user/scripts/aseExport/exportcaulk')) == 0:
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

    # Branch on primitive nodes
    def processPrimitive(scenenode):
        if scenenode.isBrush():
            processBrush(scenenode.getBrush())
        elif scenenode.isPatch():
            processPatch(scenenode.getPatch())
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

    class dataCollector(SelectionVisitor):
        def visit(self, scenenode):

            if scenenode.getNodeType() == 'primitive':
                processPrimitive(scenenode)
            elif scenenode.isEntity():
                # greebo: Found an entity, this could be a func_static or similar
                # Traverse children of this entity using a new walker
                nodewalker = nodeVisitor()
                scenenode.traverse(nodewalker)
            else:
                print('WARNING: unsupported node type selected. Skipping: ' + scenenode.getNodeType())

    # Dialog
    dialog = GlobalDialogManager.createDialog(script + 'v' + version)

    # Add an entry box and remember the handle
    fileHandle = dialog.addEntryBox("Filename:")
    dialog.setElementValue(fileHandle, GlobalRegistry.get('user/scripts/aseExport/recentFilename'))

    # Add an entry box and remember the handle
    pathHandle = dialog.addPathEntry("Save path:", True)
    dialog.setElementValue(pathHandle, GlobalRegistry.get('user/scripts/aseExport/recentPath'))

    # Add a checkbox
    centerObjectsHandle = dialog.addCheckbox("Center objects at 0,0,0 origin")
    dialog.setElementValue(centerObjectsHandle, GlobalRegistry.get('user/scripts/aseExport/centerObjects'))

    # Add another checkbox
    exportCaulkHandle = dialog.addCheckbox("Export caulked faces")
    dialog.setElementValue(exportCaulkHandle, GlobalRegistry.get('user/scripts/aseExport/exportcaulk'))

    if dialog.run() == Dialog.OK:
        fullpath = dialog.getElementValue(pathHandle) + '/' + dialog.getElementValue(fileHandle)
        if not fullpath.endswith('.ase'):
            fullpath = fullpath + '.ase'

        # Save the path for later use
        GlobalRegistry.set('user/scripts/aseExport/recentFilename', dialog.getElementValue(fileHandle))
        GlobalRegistry.set('user/scripts/aseExport/recentPath', dialog.getElementValue(pathHandle))
        GlobalRegistry.set('user/scripts/aseExport/centerObjects', dialog.getElementValue(centerObjectsHandle))
        GlobalRegistry.set('user/scripts/aseExport/exportcaulk', dialog.getElementValue(exportCaulkHandle))

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

            # Tels: Only collect the data if we are going to export it
            walker = dataCollector()
            GlobalSelectionSystem.foreachSelected(walker)

            # greebo: Check if we should center objects at the 0,0,0 origin
            if int(dialog.getElementValue(centerObjectsHandle)) == 1:
                #center objects at 0,0,0
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

            # split objects that do not share the same texture on all faces
            for x in geomlist:
                texlist = []
                for data in x[1]:
                    texlist.append(data[3])
                if len(set(texlist)) > 1:
                    temp = []
                    for texture in set(texlist):
                        vertlist = []
                        facelist = []
                        for data in x[1]:
                            if data[3] == texture:
                                facelist.append(data)
                        usedverts = []
                        for face in facelist:
                            usedverts.extend(face[0:3])
                        usedverts = list(set(usedverts))
                        for index in usedverts:
                            vertlist.append(x[0][index])
                        newfacelist = []
                        for face in facelist:
                            newfacelist.append([vertlist.index(x[0][face[0]]),vertlist.index(x[0][face[1]]),vertlist.index(x[0][face[2]]),face[3]])
                        temp.append([vertlist, newfacelist])
                    del geomlist[geomlist.index(x)]
                    geomlist.extend(temp)

            scene = '''\t*SCENE_FILENAME "{0}"
\t*SCENE_FIRSTFRAME 0
\t*SCENE_LASTFRAME 100
\t*SCENE_FRAMESPEED 30
\t*SCENE_TICKSPERFRAME 160
\t*SCENE_BACKGROUND_STATIC 0.0000\t0.0000\t0.0000
\t*SCENE_AMBIENT_STATIC 0.0000\t0.0000\t0.0000'''.format(GlobalMap.getMapName())

            materials = str()
            for x in shaderlist:
                materials = materials + '''\t*MATERIAL {0} {{
\t\t*MATERIAL_NAME "{1}"
\t\t*MATERIAL_CLASS "Standard"
\t\t*MATERIAL_AMBIENT 0.5882\t0.5882\t0.5882
\t\t*MATERIAL_DIFFUSE 0.5882\t0.5882\t0.5882
\t\t*MATERIAL_SPECULAR 0.9000\t0.9000\t0.9000
\t\t*MATERIAL_SHINE 0.1000
\t\t*MATERIAL_SHINESTRENGTH 0.0000
\t\t*MATERIAL_TRANSPARENCY 0.0000
\t\t*MATERIAL_WIRESIZE 1.0000
\t\t*MATERIAL_SHADING Blinn
\t\t*MATERIAL_XP_FALLOFF 0.0000
\t\t*MATERIAL_SELFILLUM 0.0000
\t\t*MATERIAL_FALLOFF In
\t\t*MATERIAL_XP_TYPE Filter
\t\t*MAP_DIFFUSE {{
\t\t\t*MAP_NAME "{2}"
\t\t\t*MAP_CLASS "Bitmap"
\t\t\t*MAP_SUBNO 1
\t\t\t*MAP_AMOUNT 1.0000
\t\t\t*BITMAP "\\\\purgatory\\purgatory\\doom\\base\{2}"
\t\t\t*MAP_TYPE Screen
\t\t\t*UVW_U_OFFSET 0.0000
\t\t\t*UVW_V_OFFSET 0.0000
\t\t\t*UVW_U_TILING 1.0000
\t\t\t*UVW_V_TILING 1.0000
\t\t\t*UVW_ANGLE 0.0000
\t\t\t*UVW_BLUR 1.0000
\t\t\t*UVW_BLUR_OFFSET 0.0000
\t\t\t*UVW_NOUSE_AMT 1.0000
\t\t\t*UVW_NOISE_SIZE 1.0000
\t\t\t*UVW_NOISE_LEVEL 1
\t\t\t*UVW_NOISE_PHASE 0.0000
\t\t\t*BITMAP_FILTER Pyramidal
\t\t}}
\t}}
'''.format(shaderlist.index(x), x, x.replace('/','\\'))

            geomobjects = str()

            for x in geomlist:
                # x[0] = vertices
                # vert[0] - vert[2] = vertex coords
                # vert[3] - vert[4] = texture coords
                # vert[5] - vert[7] = normal
                # x[1] = faces
                vertlist = str()
                for count, data in enumerate(x[0]):
                    vertlist = vertlist + '''\t\t\t*MESH_VERTEX {0}\t{1: 10.4f}\t{2: 10.4f}\t{3: 10.4f}\n'''.format(count, data[0], data[1], data[2])
                facelist = str()
                for count, data in enumerate(x[1]):
                    facelist = facelist + '''\t\t\t*MESH_FACE     {0}:  A:   {1} B:   {2} C:     {3} AB:       0 BC:    0 CA:    0\t *MESH_SMOOTHING 1 \t*MESH_MTLID {4}\n'''.format(count, data[0], data[1], data[2], data[3])
                tvertlist = str()
                for count, data in enumerate(x[0]):
                    tvertlist = tvertlist + '''\t\t\t*MESH_TVERT {0}\t{1: 10.4f}\t{2: 10.4f}\t0.0000\n'''.format(count, data[3], data[4])
                tfacelist = str()
                for count, data in enumerate(x[1]):
                    tfacelist = tfacelist + '''\t\t\t*MESH_TFACE {0}\t{1}\t{2}\t{3}\n'''.format(count, data[0], data[1], data[2])
                cfacelist = str()
                for count, data in enumerate(x[1]):
                    cfacelist = cfacelist + '''\t\t\t*MESH_CFACE {0}\t0\t0\t0\n'''.format(count)

                normals = str()
                for count, data in enumerate(x[1]):
                    normals += '''\t\t\t*MESH_FACENORMAL {0}\t{1: 10.4f}\t{2: 10.4f}\t{3: 10.4f}\n'''.format(count, x[0][data[0]][5], x[0][data[0]][6], x[0][data[0]][7]) # greebo: use first vertex normal as face normal
                    normals += '''\t\t\t\t*MESH_VERTEXNORMAL {0}\t{1: 10.4f}\t{2: 10.4f}\t{3: 10.4f}\n'''.format(data[0], x[0][data[0]][5], x[0][data[0]][6], x[0][data[0]][7])
                    normals += '''\t\t\t\t*MESH_VERTEXNORMAL {0}\t{1: 10.4f}\t{2: 10.4f}\t{3: 10.4f}\n'''.format(data[1], x[0][data[1]][5], x[0][data[1]][6], x[0][data[1]][7])
                    normals += '''\t\t\t\t*MESH_VERTEXNORMAL {0}\t{1: 10.4f}\t{2: 10.4f}\t{3: 10.4f}\n'''.format(data[2], x[0][data[2]][5], x[0][data[2]][6], x[0][data[2]][7])

                if len(x[1]) == 0:
                    continue

                geomobjects = geomobjects + '''*GEOMOBJECT {{
\t*NODE_NAME "{0}"
\t*NODE_TM {{
\t\t*NODE_NAME "{0}"
\t\t*INHERIT_POS 0 0 0
\t\t*INHERIT_ROT 0 0 0
\t\t*INHERIT_SCL 0 0 0
\t\t*TM_ROW0 1.0000\t0.0000\t0.0000
\t\t*TM_ROW1 0.0000\t1.0000\t0.0000
\t\t*TM_ROW2 0.0000\t0.0000\t1.0000
\t\t*TM_ROW3 0.0000\t0.0000\t0.0000
\t\t*TM_POS 0.0000\t0.0000\t0.0000
\t\t*TM_ROTAXIS 0.0000\t0.0000\t0.0000
\t\t*TM_ROTANGLE 0.0000
\t\t*TM_SCALE 1.0000\t1.0000\t1.0000
\t\t*TM_SCALEAXIS 0.0000\t0.0000\t0.0000
\t\t*TM_SCALEAXISANG 0.0000
\t}}
\t*MESH {{
\t\t*TIMEVALUE 0
\t\t*MESH_NUMVERTEX {1}
\t\t*MESH_NUMFACES {2}
\t\t*MESH_VERTEX_LIST {{
{3}\t\t}}
\t\t*MESH_FACE_LIST {{
{4}\t\t}}
\t\t*MESH_NUMTVERTEX {5}
\t\t*MESH_TVERTLIST {{
{6}\t\t}}
\t\t*MESH_NUMTVFACES {7}
\t\t*MESH_TFACELIST {{
{8}\t\t}}
\t\t*MESH_NUMCVERTEX 1
\t\t*MESH_CVERTLIST {{
\t\t\t*MESH_VERTCOL 0\t1.0000\t1.0000\t1.0000
\t\t}}
\t\t*MESH_NUMCVFACES {9}
\t\t*MESH_CFACELIST {{
{10}\t\t}}
\t\t*MESH_NORMALS {{
{11}\t\t}}
\t}}
\t*PROP_MOTIONBLUR 0
\t*PROP_CASTSHADOW 1
\t*PROP_RECVSHADOW 1
\t*MATERIAL_REF {12}
}}\n'''.format('mesh' + str(geomlist.index(x)), \
        len(x[0]), \
        len(x[1]), \
        vertlist, \
        facelist, \
        len(x[0]), \
        tvertlist, \
        len(x[1]), \
        tfacelist, \
        len(x[1]), \
        cfacelist, \
        normals, \
        x[1][0][3]) # material reference from first face

                data = '''*3DSMAX_ASCIIEXPORT\t200
*COMMENT "{0} v{1}"
*SCENE {{
{2}
}}
*MATERIAL_LIST {{
\t*MATERIAL_COUNT {3}
{4}}}
{5}'''.format(script, version, scene, len(shaderlist), materials, geomobjects)

                # Write the compiled data to the output file
                file = open(fullpath, 'w')
                file.write(data)
                file.close()

# __executeCommand__ evaluates to true after DarkRadiant has successfully initialised
if __executeCommand__:
    execute()
