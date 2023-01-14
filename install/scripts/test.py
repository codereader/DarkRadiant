# Some interface tests
import darkradiant as dr

# Test the Registry interface
value = GlobalRegistry.get('user/paths/appPath')
print(value)

worldspawn = Radiant.findEntityByClassname("worldspawn")
worldspawn.setKeyValue('test', 'success')
print('Worldspawn edited')

worldspawn = Radiant.findEntityByName("world")
worldspawn.setKeyValue('test', 'another success')
print('Worldspawn edited')

# Test the DeclarationManager interface
class TestDeclarationVisitor(dr.DeclarationVisitor) :
    def visit(self, decl):
        print(str(decl.getDeclType()) + ": " + decl.getDeclName())

visitor = TestDeclarationVisitor()

# Visit all skins
GlobalDeclarationManager.foreachDeclaration(Declaration.Type.Skin, visitor)

caulk = GlobalDeclarationManager.findDeclaration(Declaration.Type.Material, "textures/common/caulk")
print("Name: " + caulk.getDeclName())
print("Type: " + str(caulk.getDeclType()))
print("Defined in: " + str(caulk.getDeclFilePath()))
print("Definition: " + caulk.getBlockSyntax().contents)

GlobalDeclarationManager.foreachDeclaration(Declaration.Type.ModelDef, visitor)

# FX interface
fx = GlobalFxManager.findFx("fx/sparks")

if not fx.isNull():
    # An FX declaration inherits all the Declaration methods and properties
    print("Name: " + fx.getDeclName())
    print("Type: " + str(fx.getDeclType()))
    print("Defined in: " + str(fx.getDeclFilePath()))
    print("Definition: " + fx.getBlockSyntax().contents)

    print("Number of actions: " + str(fx.getNumActions()))
    
    action = fx.getAction(1)
    print("Action with Index 1 (this is the second):")
    print("Action #1 has Type: " + str(action.getActionType()))
    print("Action #1 has Name: " + str(action.getName()))
    print("Action #1 has Delay: " + str(action.getDelayInSeconds()))
    print("Action #1 has Duration: " + str(action.getDurationInSeconds()))
    print("Action #1 has IgnoreMaster: " + str(action.getIgnoreMaster()))
    print("Action #1 has ShakeTime: " + str(action.getShakeTimeInSeconds()))
    print("Action #1 has ShakeAmplitude: " + str(action.getShakeAmplitude()))
    print("Action #1 has ShakeDistance: " + str(action.getShakeDistance()))
    print("Action #1 has ShakeFalloff: " + str(action.getShakeFalloff()))
    print("Action #1 has ShakeImpulse: " + str(action.getShakeImpulse()))
    print("Action #1 has NoShadows: " + str(action.getNoShadows()))
    print("Action #1 has FireSiblingAction: " + str(action.getFireSiblingAction()))
    print("Action #1 has RandomDelay: " + str(action.getRandomDelay()))
    print("Action #1 has Rotate: " + str(action.getRotate()))
    print("Action #1 has TrackOrigin: " + str(action.getTrackOrigin()))
    print("Action #1 has Restart: " + str(action.getRestart()))
    print("Action #1 has FadeInTimeInSeconds: " + str(action.getFadeInTimeInSeconds()))
    print("Action #1 has FadeOutTimeInSeconds: " + str(action.getFadeOutTimeInSeconds()))
    print("Action #1 has DecalSize: " + str(action.getDecalSize()))
    print("Action #1 has Offset: " + str(action.getOffset()))
    print("Action #1 has Axis: " + str(action.getAxis()))
    print("Action #1 has Angle: " + str(action.getAngle()))
    print("Action #1 has UseLight: " + str(action.getUseLight()))
    print("Action #1 has UseModel: " + str(action.getUseModel()))
    print("Action #1 has AttachLight: " + str(action.getAttachLight()))
    print("Action #1 has AttachEntity: " + str(action.getAttachEntity()))
    print("Action #1 has LaunchProjectileDef: " + str(action.getLaunchProjectileDef()))
    print("Action #1 has LightMaterialName: " + str(action.getLightMaterialName()))
    print("Action #1 has LightRgbColour: " + str(action.getLightRgbColour()))
    print("Action #1 has LightRadius: " + str(action.getLightRadius()))
    print("Action #1 has ModelName: " + str(action.getModelName()))
    print("Action #1 has DecalMaterialName: " + str(action.getDecalMaterialName()))
    print("Action #1 has ParticleTrackVelocity: " + str(action.getParticleTrackVelocity()))
    print("Action #1 has SoundShaderName: " + str(action.getSoundShaderName()))
    print("Action #1 has ShockwaveDefName: " + str(action.getShockwaveDefName()))

# Create a new material
myOwnMaterial = GlobalDeclarationManager.findOrCreateDeclaration(Declaration.Type.Material, "textures/myown_material")

syntax = myOwnMaterial.getBlockSyntax()
syntax.contents = "diffusemap _white"
myOwnMaterial.setBlockSyntax(syntax)

# Save the material to a new file
myOwnMaterial.setDeclFilePath("materials/", "script_test.mtr")
GlobalDeclarationManager.saveDeclaration(myOwnMaterial)

# Test the EClassManager interface
eclass = GlobalEntityClassManager.findClass('atdm:func_shooter')
print(eclass.getAttribute('editor_usage').getValue())

# Try creating a func_shooter entity
if not eclass.isNull():
	shooter = GlobalEntityCreator.createEntity(eclass)

modelDef = GlobalEntityClassManager.findModel('tdm_ai_citywatch')
print('ModelDef mesh for tdm_ai_citywatch = ' + modelDef.mesh)

# Test iterating over C++ std::map
for anim in modelDef.anims:
	print(anim + " = " + modelDef.anims[anim])

# Test implementing a eclass visitor interface
#class TestVisitor(EntityClassVisitor) :
#	def visit(self, eclass):
#		print(eclass.getAttribute('editor_usage').getValue())

#eclassVisitor = TestVisitor()
#GlobalEntityClassManager.forEachEntityClass(eclassVisitor)

# Test implementing a model def visitor interface
#class TestModelDefVisitor(ModelDefVisitor) :
#	def visit(self, modelDef):
#		print(modelDef.mesh)
#
#modelDefVisitor = TestModelDefVisitor()
#GlobalEntityClassManager.forEachModelDef(modelDefVisitor)

# Test traversing the scenegraph
class SceneWalker(dr.SceneNodeVisitor) :
	def pre(self, node):
		print(node.getNodeType())

		# Try to get a model from this node
		model = node.getModel()

		if not model.isNull():
			print('Node is a model')
		else:
			print('Node is not a model')

		return 1

walker = SceneWalker()
GlobalSceneGraph.root().traverse(walker)

# Test traversing the current selection
class Walker(dr.SelectionVisitor) :
	def visit(self, node):
		# Try to "cast" the node to a brush
		brush = node.getBrush()

		# If the Brush is not NULL the cast succeeded
		if not brush.isNull():
			print(brush.getNumFaces())
		else:
			print('Node is not a brush')

		# Try to cast the node to a patch
		patch = node.getPatch()

		# If the Patch is not NULL the cast succeeded
		if not patch.isNull():
			print('Node is a patch')
		else:
			print('Node is not a patch')

		# Try to get a model from this node
		model = node.getModel()

		if not model.isNull():
			print('Node is a model')
		else:
			print('Node is not a model')


visitor = Walker()
GlobalSelectionSystem.foreachSelected(visitor)

# Visit every selected face
class FaceVisitor(dr.SelectedFaceVisitor) :
	def visitFace(self, face):
		print(face.getShader())

visitor = FaceVisitor()
GlobalSelectionSystem.foreachSelectedFace(visitor)

print('Map name is ' + GlobalMap.getMapName())

print(GlobalMap.getEditMode())

# Switching Map Modes
GlobalMap.setEditMode(MapEditMode.Merge)
GlobalMap.setEditMode(MapEditMode.Normal)

# Point File Management is visible
print(GlobalMap.showPointFile("test.lin"))
print(GlobalMap.isPointTraceVisible())

# Enumerate the point files available for the current map
print("Point files found: " + str(len(GlobalMap.getPointFileList())))

for path in GlobalMap.getPointFileList:
	print("Pointfile: " + path)

# Try to find the map's worldspawn
worldspawn = GlobalMap.getWorldSpawn()

if not worldspawn.isNull():
	# Cast the node onto an entity
	worldspawnent = worldspawn.getEntity()
	if not worldspawnent.isNull():
		print('Spawnclass of worldspawn: ' + worldspawnent.getKeyValue('spawnclass'))
else:
	print('There is no worldspawn in this map yet')

# Test the entity visitor interface
class TestEntityVisitor(dr.EntityVisitor) :
	def visit(self, key, value):
		print('Worldspawn has spawnarg: ' + key + ' = ' + value)

if not worldspawn.isNull():
	tev = TestEntityVisitor()

	# Cast the node onto an entity
	worldspawnent = worldspawn.getEntity()

	worldspawnent.forEachKeyValue(tev)

	# Try to retrieve all keyvalues starting with "n"
	keyvalues = worldspawnent.getKeyValuePairs('t')

	for kv in keyvalues:
		print('Keyvalue ' + kv[0] + ' = ' + kv[1])

# Test the commandsystem
GlobalCommandSystem.execute('texscale "0 0.1"')

# Test the GameManager interface
print('Mod path = ' + GlobalGameManager.getModPath())

game = GlobalGameManager.currentGame()
print('Current game type: ' + game.getKeyValue('type'))

print('VFS Search paths:')
vfsPaths = GlobalGameManager.getVFSSearchPaths()

for path in vfsPaths:
	print(path)

# Test FileSystem (VFS)
class TestFileVisitor(dr.FileVisitor) :
	def visit(self, filename):
		print('Found file: ' + filename)

filevisitor = TestFileVisitor()
GlobalFileSystem.forEachFile('skins/', 'skin', filevisitor, 99)

filecontents = GlobalFileSystem.readTextFile('skins/tdm_ai_guard_citywatch.skin');
print(filecontents)

# Test the Grid Interface
print('Current grid size = ' + str(GlobalGrid.getGridSize()))

# Test the ShaderSystem interface
class TestMaterialVisitor(dr.MaterialVisitor) :
	def visit(self, shader):
		if not shader.isNull():
			print('Found shader: ' + shader.getName() + ' defined in ' + shader.getShaderFileName())

# Disabled code, takes very long in TDM
# materialVisitor = TestMaterialVisitor()
# GlobalMaterialManager.foreachMaterial(materialVisitor)

material = GlobalMaterialManager.getMaterialForName('bc_rat')

if not material.isNull():
	print('Material ' + material.getName() + ' is defined in ' + material.getShaderFileName())

# Test finding a model
class ModelFinder(dr.SceneNodeVisitor) :
	def pre(self, node):
		# Try to get a model from this node
		model = node.getModel()

		if not model.isNull():
			print('Model information:')
			print('Filename: ' + model.getFilename())
			print('Model path: ' + model.getModelPath())
			print('Surface count: ' + str(model.getSurfaceCount()))
			print('Vertex count: ' + str(model.getVertexCount()))
			print('Poly count: ' + str(model.getPolyCount()))

			materials = model.getActiveMaterials()

			print('Active Materials:')
			for material in materials:
				print(material)

			for i in range(0, model.getSurfaceCount()):
				surface = model.getSurface(i)
				print('Surface: ' + str(i))
				print('  Default Shader: ' + surface.getDefaultMaterial())
				print('  Active Shader: ' + surface.getActiveMaterial())
				print('  PolyCount: ' + str(surface.getNumTriangles()))
				print('  Vertex Count: ' + str(surface.getNumVertices()))

				s = Vector3(0,0,0)
				numverts = surface.getNumVertices()
				for v in range(0, numverts):
					meshvertex = surface.getVertex(v)
					s += meshvertex.vertex

				print('  Sum of all vertices: ' + str(s.x()) + ',' + str(s.y()) + ',' + str(s.z()))

		return 1

walker = ModelFinder()
GlobalSceneGraph.root().traverse(walker)

# Test the ModelSkinCache interface
#allSkins = GlobalModelSkinCache.getAllSkins()
#
#for skin in allSkins:
#	modelskin = GlobalModelSkinCache.capture(skin)
#	print('Skin found: ' + modelskin.getName())

v = dr.Vector3(6,6,6)
v += dr.Vector3(10,10,10)
print(v)

# Test patch manipulation
class PatchManipulator(dr.SceneNodeVisitor) :
	def pre(self, node):
		# Try to get a patch from this node
		patch = node.getPatch()

		if not patch.isNull():
			print('Patch information:')
			print('Dimensions: ' + str(patch.getWidth()) + 'x' + str(patch.getHeight()))

			w = 0
			while w < patch.getWidth():
				h = 0
				while h < patch.getHeight():
					# Push the vertex around a bit
					ctrl = patch.ctrlAt(h,w)
					ctrl.vertex += dr.Vector3(0,0,10*(h-w))
					h += 1
				w += 1

			patch.controlPointsChanged()

		return 1

walker = PatchManipulator()
GlobalSceneGraph.root().traverse(walker)

# Test the SelectionSetManager interface
class SelectionSetWalker(dr.SelectionSetVisitor) :
	def visit(self, selectionset):
		print(selectionset.getName())

walker = SelectionSetWalker()
GlobalSelectionSetManager.foreachSelectionSet(walker)

selSet = GlobalSelectionSetManager.createSelectionSet("TestSelectionSet")
selSet.assignFromCurrentScene()

selSet.deselect()
selSet.select()

selSet.clear()

if selSet.empty():
	GlobalSelectionSetManager.deleteSelectionSet("TestSelectionSet")

print('')

soundshader = GlobalSoundManager.getSoundShader('tdm_ai_lady_alertdown_to_idle')

if not soundshader.isNull():
	print('Name of this sound shader: ' + soundshader.getName())

	radii = soundshader.getRadii()

	print('Minimum radius in meters: ' + str(radii.getMin(1)))
	print('Maximum radius in meters: ' + str(radii.getMax(1)))

	fileList = soundshader.getSoundFileList()
	for i in range(0, len(fileList)):
		print(' Sound file used by this shader: ' + fileList[i])

	if (len(fileList) > 0):
		GlobalSoundManager.playSound(fileList[0])

# Test SelectionGroup interface
group = GlobalSelectionGroupManager.createSelectionGroup()

print('Created group with ID: ' + str(group.getId()))

# Test traversing the current selection
class GroupAdder(dr.SelectionVisitor) :
	def visit(self, node):
		group.addNode(node)

visitor = Walker()
GlobalSelectionSystem.foreachSelected(visitor)

print('The group contains now ' + str(group.size()) + ' items')

# Deselect the group
GlobalSelectionGroupManager.setGroupSelected(group.getId(), 0)

# List nodes in this group
class SelectionGroupWalker(dr.SelectionGroupVisitor) :
	def visit(self, node):
		print('Group Member: ' + node.getNodeType())

gropWalker = SelectionGroupWalker();
group.foreachNode(gropWalker)

camview = GlobalCameraManager.getActiveView()
print(camview.getCameraOrigin())
camview.setCameraOrigin(dr.Vector3(50,0,50))

# Layer Functionality
def printLayers():
    class LayerPrinter(dr.LayerVisitor):
        def visit(self, layerID, layerName):
            print(layerID, layerName)
    layerPrinter = LayerPrinter()
    GlobalLayerManager.foreachLayer(layerPrinter)
    print("=================")

print("Layers:")
printLayers()

print("Test create")
print(GlobalLayerManager.createLayer("One"))	
print(GlobalLayerManager.createLayer("Two"))
print(GlobalLayerManager.createLayer("Forty-two", 42))
print(GlobalLayerManager.createLayer("TwoAgain", 2))
printLayers()

print("Test delete")
print(GlobalLayerManager.deleteLayer("NotALayer"))
print(GlobalLayerManager.deleteLayer("TwoAgain"))
printLayers()

print("Test get")
print(GlobalLayerManager.getLayerID("Forty-two"))
print(GlobalLayerManager.getLayerName(42))

print("Test exists")
print(GlobalLayerManager.layerExists(123))
print(GlobalLayerManager.layerExists(42))

print("Test rename")
print(GlobalLayerManager.renameLayer(42, "Forty-two"))
print(GlobalLayerManager.renameLayer(42, "Two"))
print(GlobalLayerManager.renameLayer(42, "HHGTTG"))

print("Test active")
GlobalLayerManager.setActiveLayer(3)
print(GlobalLayerManager.getActiveLayer())
GlobalLayerManager.setActiveLayer(2)
print(GlobalLayerManager.getActiveLayer())

print("Test visible")
GlobalLayerManager.setLayerVisibility("One", False)
print(GlobalLayerManager.layerIsVisible("One"))
GlobalLayerManager.setLayerVisibility("One", True)
print(GlobalLayerManager.layerIsVisible("One"))

GlobalLayerManager.setLayerVisibility(1, False)
print(GlobalLayerManager.layerIsVisible(1))
GlobalLayerManager.setLayerVisibility(1, True)
print(GlobalLayerManager.layerIsVisible(1))

GlobalLayerManager.setSelected(0, True)
GlobalLayerManager.moveSelectionToLayer(1)
