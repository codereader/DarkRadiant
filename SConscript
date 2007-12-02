import os, sys, commands, string
# OS Detection:
OS = commands.getoutput('uname')

Import('GLOBALS')
Import(GLOBALS)

def build_list(s_prefix, s_string):
    s_list = Split(s_string)
    for i in range(len(s_list)):
        s_list[i] = s_prefix + '/' + s_list[i]
    return s_list

# common code ------------------------------------------------------

xmlutilEnv = g_env.Copy()
xmlutilEnv.useXML2()
xmlutilSource = 'Document.cpp Node.cpp'
xmlutil = xmlutilEnv.StaticLibrary(target='libs/xmlutil', source=build_list('libs/xmlutil', xmlutilSource))

# libs/math library
mathEnv = g_env.Copy()
mathSrc = 'aabb.cpp matrix.cpp'
math = mathEnv.StaticLibrary(target='libs/math', 
							 source=build_list('libs/math', mathSrc))

md5lib_lib = g_env.StaticLibrary(target='libs/md5lib', source='libs/md5lib/md5lib.c')

ddslib_lib = g_env.StaticLibrary(target='libs/ddslib', source='libs/ddslib/ddslib.cpp')

jpeg_env = g_env.Copy()
jpeg_env.Prepend(CPPPATH = 'libs/jpeg6')
jpeg_src = 'jcomapi.cpp jdcoefct.cpp jdinput.cpp jdpostct.cpp jfdctflt.cpp jpgload.cpp jdapimin.cpp jdcolor.cpp jdmainct.cpp jdsample.cpp jidctflt.cpp jutils.cpp jdapistd.cpp jddctmgr.cpp jdmarker.cpp jdtrans.cpp jmemmgr.cpp jdatasrc.cpp jdhuff.cpp jdmaster.cpp jerror.cpp jmemnobs.cpp'
jpeg_lib = jpeg_env.StaticLibrary(target='libs/jpeg6', source=build_list('libs/jpeg6', jpeg_src))

picomodel_src = 'picointernal.c picomodel.c picomodules.c pm_3ds.c pm_ase.c pm_md3.c pm_obj.c\
  pm_ms3d.c pm_mdc.c pm_fm.c pm_md2.c pm_lwo.c pm_terrain.c lwo/clip.c lwo/envelope.c lwo/list.c lwo/lwio.c\
  lwo/lwo2.c lwo/lwob.c lwo/pntspols.c lwo/surface.c lwo/vecmath.c lwo/vmap.c'
picomodel_lib = g_env.StaticLibrary(target='libs/picomodel', source=build_list('libs/picomodel', picomodel_src))

#splines_env = g_env.Copy()
#splines_src = build_list('libs/splines', 'math_angles.cpp math_matrix.cpp math_quaternion.cpp math_vector.cpp q_parse.cpp q_shared.cpp splines.cpp util_str.cpp')
#splines_env['CPPPATH'].append('include')
#splines_lib = splines_env.StaticLibrary(target='libs/splines', source=splines_src)

gtkutil_env = g_env.Copy();
gtkutil_env['CPPPATH'].append('include')
gtkutil_env.useOpenGL()
gtkutil_env.useGlib2()
gtkutil_env.useGtk2()
gtkutil_env.useGtkGLExt()
gtkutil_env.useXML2()

gtkutil_src = '\
  button.cpp\
  clipboard.cpp\
  container.cpp\
  cursor.cpp\
  DeferredAdjustment.cpp\
  dialog.cpp\
  entry.cpp\
  frame.cpp\
  filechooser.cpp\
  glfont.cpp\
  glwidget.cpp\
  idledraw.cpp\
  messagebox.cpp\
  nonmodal.cpp\
  pointer.cpp\
  widget.cpp\
  window.cpp\
  xorrectangle.cpp\
  ModalProgressDialog.cpp\
  TreeModel.cpp\
  VFSTreePopulator.cpp\
  RegistryConnector.cpp\
  window/PersistentTransientWindow.cpp\
  PanedPosition.cpp\
  MenuItemAccelerator.cpp\
  Timer.cpp\
  WindowPosition.cpp\
'

gtkutil_lib = gtkutil_env.StaticLibrary(target='libs/gtkutil', source=build_list('libs/gtkutil', gtkutil_src))

# end static / common libraries ---------------------------------------------------

# radiant, modules and plugins ----------------------------------------------------

module_env = g_env.Copy()
module_env.Dir('libs/string')
module_env.Dir('libs/memory')
module_env['CPPPATH'].append('include')
if (module_env['PLATFORM'] == 'posix'):
    module_env['LINKFLAGS'] += '-ldl ' # do we need this library?
if (module_env['PLATFORM'] == 'win32'):
    module_env['LINKFLAGS'] += '-Wl,--kill-at ' # must not append @n to exported DLL symbols otherwise Radiant will not find them
module_env['LIBPREFIX'] = ''
module_env.Append(LIBPATH = ['libs'])
module_env['no_import_lib'] = 1
module_env.useXML2()
module_env.useOpenGL()

# Filters module
filterEnv = module_env.Copy()
filterSrc = build_list('plugins/filters', 
                       'filters.cpp XMLFilter.cpp BasicFilterSystem.cpp')
filterEnv.useBoostRegex()
filterEnv.Append(LIBS = 'xmlutil')
filterLib = filterEnv.SharedLibrary(target='filters', source=filterSrc, no_import_lib=1)
filterEnv.Install(INSTALL + '/modules', filterLib)

# XMLRegistry module
xmlRegistryEnv = module_env.Copy()
xmlRegistrySrc = build_list('plugins/xmlregistry', 'RegistryTree.cpp XMLRegistry.cpp')
xmlRegistryEnv.Append(LIBS = 'xmlutil')
xmlRegistryEnv.useBoostRegex()
xmlRegistryLib = xmlRegistryEnv.SharedLibrary(target='xmlregistry', source=xmlRegistrySrc)
xmlRegistryEnv.Install(INSTALL + '/modules', xmlRegistryLib)

# FileTypes module
fileTypesEnv = module_env.Copy()
fileTypesSrc = build_list('plugins/filetypes', 'FileTypeRegistry.cpp')
fileTypesLib = fileTypesEnv.SharedLibrary(target='filetypes', source=fileTypesSrc)
fileTypesEnv.Install(INSTALL + '/modules', fileTypesLib)

# Grid module
gridEnv = module_env.Copy()
gridSrc = build_list('plugins/grid', 'Grid.cpp')
gridLib = gridEnv.SharedLibrary(target='grid', source=gridSrc)
gridEnv.Install(INSTALL + '/modules', gridLib)

# UndoSystem module
undoEnv = module_env.Copy()
undoSrc = build_list('plugins/undo', 'UndoSystem.cpp')
undoLib = undoEnv.SharedLibrary(target='undo', source=undoSrc)
undoEnv.Install(INSTALL + '/modules', undoLib)

# Eclassmgr module
eclassSrc = build_list('plugins/eclassmgr', 'eclass_doom3.cpp Doom3EntityClass.cpp')
eclassEnv = module_env.Copy()
eclassEnv.Append(LIBS = 'math')
eclassEnv.useGlib2()
eclassEnv.useGtk2()
eclassLib = eclassEnv.SharedLibrary(target='eclassmgr', source=eclassSrc, no_import_lib=1)
eclassEnv.Depends(eclassLib, math)
eclassEnv.Install(INSTALL + '/modules', eclassLib)

# Sound manager module
sndSrc = build_list('plugins/sound', 'sound.cpp SoundManager.cpp SoundPlayer.cpp')
sndEnv = module_env.Copy()
sndEnv.Append(LIBS = ['gtkutil'])
sndEnv.useOpenAL()
sndEnv.useGlib2()
sndEnv.useGtk2()
sndEnv.useBoost()
sndLib = sndEnv.SharedLibrary(target='sound', source=sndSrc)
sndEnv.Depends(sndLib, gtkutil_lib)
sndEnv.Install(INSTALL + '/modules', sndLib)

vfspk3_env = module_env.Copy()
vfspk3_lst = build_list('plugins/vfspk3', 'vfspk3.cpp vfs.cpp archive.cpp')
vfspk3_env.useGlib2()
vfspk3_lib = vfspk3_env.SharedLibrary(target='vfspk3', source=vfspk3_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
vfspk3_env.Install(INSTALL + '/modules', vfspk3_lib)

archivezip_env = module_env.Copy()
archivezip_lst = build_list('plugins/archivezip', 'plugin.cpp archive.cpp pkzip.cpp zlibstream.cpp')
archivezip_env.useZLib()
archivezip_env.Append(LIBPATH = ['libs'])
archivezip_lib = archivezip_env.SharedLibrary(target='archivezip', source=archivezip_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
archivezip_env.Install(INSTALL + '/modules', archivezip_lib)

# Shaders module
shaders_env = module_env.Copy()
shaders_lst = build_list('plugins/shaders', 
						 'plugin.cpp \
						 CShader.cpp \
						 MapExpression.cpp \
						 ShaderTemplate.cpp \
						 ShaderFileLoader.cpp \
						 ShaderLibrary.cpp \
						 Doom3ShaderSystem.cpp \
						 textures/GLTextureManager.cpp \
						 textures/TextureManipulator.cpp \
						 textures/DefaultConstructor.cpp \
						 textures/FileLoader.cpp \
						 textures/ImageLoaderManager.cpp')
shaders_env.useGlib2()
shaders_env.Append(LIBS = ['xmlutil'])
shaders_env.Append(LIBPATH = ['libs'])
shaders_lib = shaders_env.SharedLibrary(target='shaders', source=shaders_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
shaders_env.Depends(shaders_lib, xmlutil)
shaders_env.Install(INSTALL + '/modules', shaders_lib)

# Skins module
skinsEnv = module_env.Copy()
skinsList = build_list('plugins/skins', 'skincache.cpp Doom3SkinCache.cpp')
skinsLib = skinsEnv.SharedLibrary(target='skins', source=skinsList, no_import_lib=1)
skinsEnv.Install(INSTALL + '/modules', skinsLib)

image_env = module_env.Copy()
image_lst = build_list('plugins/image', 'bmp.cpp jpeg.cpp image.cpp pcx.cpp tga.cpp dds.cpp ImageGDK.cpp')
image_env.useGtk2()
image_env.useGlib2()
image_env.Append(LIBS=['jpeg6', 'ddslib'])
image_env.Append(LIBPATH = ['libs'])
image_lib = image_env.SharedLibrary(target='image', source=image_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
image_env.Depends(image_lib, jpeg_lib)
image_env.Depends(image_lib, ddslib_lib)
image_env.Install(INSTALL + '/modules', image_lib)

# Particles manager
particlesEnv = module_env.Copy()
particlesSrc = ['particles.cpp', 'ParticlesManager.cpp']
particlesLib = particlesEnv.SharedLibrary(
                target='particles',
                source=['plugins/particles/' + f for f in particlesSrc])
particlesEnv.Install(INSTALL + '/modules', particlesLib)

# Map loading and saving module
mapdoom3_env = module_env.Copy()
mapdoom3_lst=build_list('plugins/mapdoom3', 'mapdoom3.cpp parse.cpp write.cpp')
mapdoom3_env.Append(LIBS = ['xmlutil', 'gtkutil'])
mapdoom3_env.useGtk2()
mapdoom3_env.useGlib2()
mapdoom3_lib = mapdoom3_env.SharedLibrary(target='mapdoom3', source=mapdoom3_lst)
mapdoom3_env.Depends(mapdoom3_lib, xmlutil)
mapdoom3_env.Depends(mapdoom3_lib, gtkutil_lib)
mapdoom3_env.Install(INSTALL + '/modules', mapdoom3_lib)

model_env = module_env.Copy()
model_lst = build_list('plugins/model', 
					   'plugin.cpp model.cpp \
					   RenderablePicoModel.cpp RenderablePicoSurface.cpp \
					   PicoModelInstance.cpp')
model_env.Append(LIBS = ['picomodel', 'math'])
model_lib = model_env.SharedLibrary(target='model', source=model_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
model_env.Depends(model_lib, picomodel_lib)
model_env.Depends(model_lib, math)
model_env.Install(INSTALL + '/modules', model_lib)

md5model_env = module_env.Copy()
md5model_lst=build_list('plugins/md5model', 
						'plugin.cpp \
						 md5.cpp \
						 MD5Model.cpp \
						 MD5ModelInstance.cpp \
						 MD5ModelNode.cpp \
						 MD5Parser.cpp \
						 MD5ModelLoader.cpp \
						 MD5Surface.cpp')
md5model_env.Append(LIBS = ['math'])
md5model_lib = md5model_env.SharedLibrary(target='md5model', source=md5model_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
md5model_env.Depends(md5model_lib, math)
md5model_env.Install(INSTALL + '/modules', md5model_lib)

eventmanager_env = module_env.Copy()
eventmanager_lst=build_list('plugins/eventmanager', 'EventManager.cpp Accelerator.cpp Command.cpp Toggle.cpp WidgetToggle.cpp Modifiers.cpp MouseEvents.cpp')
eventmanager_env.Append(LIBS = ['gtkutil', 'xmlutil'])
eventmanager_env.useGtk2()
eventmanager_env.useGlib2()
eventmanager_lib = eventmanager_env.SharedLibrary(target='eventmanager', source=eventmanager_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
eventmanager_env.Depends(eventmanager_lib, gtkutil_lib)
eventmanager_env.Install(INSTALL + '/modules', eventmanager_lib)

uiManagerEnv = module_env.Copy()
uiManagerLst=build_list('plugins/uimanager', 'UIManager.cpp MenuManager.cpp MenuItem.cpp ToolbarManager.cpp')
uiManagerEnv.Append(LIBS = ['gtkutil', 'xmlutil'])
uiManagerEnv.useGtk2()
uiManagerEnv.useGlib2()
uiManagerLib = uiManagerEnv.SharedLibrary(target='uimanager', source=uiManagerLst, no_import_lib=1, WIN32_INSERT_DEF=0)
uiManagerEnv.Depends(uiManagerLib, gtkutil_lib)
uiManagerEnv.Install(INSTALL + '/modules', uiManagerLib)

# Entity creator module
entity_env = module_env.Copy()
entity_src = [
	'plugin.cpp',
	'EntityCreator.cpp',
	'entity.cpp',
	'angle.cpp',
	'angles.cpp',
	'colour.cpp',
	'model.cpp',
	'namedentity.cpp',
	'origin.cpp',
	'scale.cpp',
	'targetable.cpp',
	'rotation.cpp',
	'Doom3Entity.cpp',
	'KeyValue.cpp',
	'curve/Curve.cpp',
	'curve/CurveNURBS.cpp',
	'curve/CurveCatmullRom.cpp',
	'curve/CurveEditInstance.cpp',
	'light/Light.cpp',
	'light/Renderables.cpp',
	'light/LightInstance.cpp',
	'light/LightNode.cpp',
	'light/LightSettings.cpp',
	'doom3group/Doom3Group.cpp',
	'doom3group/Doom3GroupInstance.cpp',
	'doom3group/Doom3GroupNode.cpp',
	'speaker/Speaker.cpp',
	'speaker/SpeakerRenderables.cpp',
	'speaker/SpeakerInstance.cpp',
	'speaker/SpeakerNode.cpp',
	'speaker/SpeakerSettings.cpp',
	'generic/GenericEntity.cpp',
	'generic/GenericEntityInstance.cpp',
	'generic/GenericEntityNode.cpp',
	'eclassmodel/EclassModel.cpp',
	'eclassmodel/EclassModelInstance.cpp',
	'eclassmodel/EclassModelNode.cpp'
]
entity_lst = build_list('plugins/entity', entity_src)
entity_env.Append(LIBS = ['math', 'xmlutil'])
entity_lib = entity_env.SharedLibrary(target='entity', 
									  source=entity_lst, 
									  no_import_lib=1)
entity_env.Depends(entity_lib, math)
entity_env.Depends(entity_lib, xmlutil)
entity_env.Install(INSTALL + '/modules', entity_lib)

# Optional plugins

objEnv = module_env.Copy()
objEnv.Append(LIBS = ['gtkutil'])
objEnv.useGtk2()
objEnv.useGlib2()
objEnv.useBoostRegex()
objList = build_list('plugins/dm.objectives', 
					 'objectives.cpp \
					 ObjectivesEditor.cpp \
					 ObjectiveEntity.cpp \
					 ObjectiveKeyExtractor.cpp \
					 ComponentsDialog.cpp')
objLib = objEnv.SharedLibrary(target='dm_objectives',
							  source=objList,
							  no_import_lib=1)
objEnv.Install(INSTALL + '/plugins', objLib)

srEnv = module_env.Copy()
srEnv.Append(LIBS = ['gtkutil', 'xmlutil'])
srEnv.useGtk2()
srEnv.useGlib2()
srEnv.useBoostRegex()
srList = build_list('plugins/dm.stimresponse', 
					'plugin.cpp \
					 StimResponseEditor.cpp \
					 StimTypes.cpp \
					 SREntity.cpp \
					 StimResponse.cpp \
					 SRPropertyLoader.cpp \
					 SRPropertyRemover.cpp \
					 SRPropertySaver.cpp \
					 ClassEditor.cpp \
					 StimEditor.cpp \
					 ResponseEditor.cpp \
					 EffectEditor.cpp \
					 EffectArgumentItem.cpp \
					 ResponseEffect.cpp \
					 ResponseEffectTypes.cpp \
					 CustomStimEditor.cpp')
srLib = srEnv.SharedLibrary(target='dm_stimresponse',
							source=srList,
							no_import_lib=1)
srEnv.Install(INSTALL + '/plugins', srLib)

d3hookEnv = module_env.Copy()
d3hookEnv.Append(LIBS = ['gtkutil', 'xmlutil'])
d3hookEnv.useGtk2()
d3hookEnv.useGlib2()
d3hookEnv.useBoost()
d3hookEnv.Append(CPPPATH = ['#/plugins/dm.d3hook'])
if (d3hookEnv['PLATFORM'] == 'win32'):
	d3hookEnv.Append(LIBS = ['ws2_32', 'Psapi'])
	d3hookEnv.Append(CXXFLAGS = ' -Wno-deprecated ')
if (d3hookEnv['PLATFORM'] == 'posix'):
	d3hookEnv.Append(CXXFLAGS = ' -DRCF_USE_BOOST_ASIO -Wno-deprecated -Wno-unused ')
d3hookList = build_list('plugins/dm.d3hook',
						'plugin.cpp \
						 DarkRadiantRCFServer.cpp \
						 D3ProcessChecker.cpp \
						 DarkModRCFClient.cpp \
						 DarkModCommands.cpp \
						 RCF/RCF.cpp')
d3hookLib = d3hookEnv.SharedLibrary(target='dm_d3hook',
							source=d3hookList,
							no_import_lib=1)
d3hookEnv.Install(INSTALL + '/plugins', d3hookLib)

# Entity Class Tree Plugin
ectEnv = module_env.Copy()
ectEnv.Append(LIBS = ['gtkutil', 'xmlutil'])
ectEnv.useGtk2()
ectEnv.useGlib2()
ectEnv.useBoostRegex()
ectList = build_list('plugins/eclasstree', 
					'plugin.cpp \
					 EClassTree.cpp \
					 EClassTreeBuilder.cpp')
ectLib = ectEnv.SharedLibrary(target='eclasstree',
							source=ectList,
							no_import_lib=1)
ectEnv.Install(INSTALL + '/plugins', ectLib)

# Main Radiant binary

radiant_env = g_env.Copy()
radiant_env['CPPPATH'].append('include')
if radiant_env['PLATFORM'] == 'posix':
    radiant_env['LINKFLAGS'] += '-ldl '
radiant_env['LIBPREFIX'] = ''
radiant_env.useGlib2()
radiant_env.useXML2()
radiant_env.useGtk2()
radiant_env.useGtkGLExt()
radiant_env.useOpenGL()
radiant_env.useBoostFilesystem()
radiant_env.buildIconResource()

radiant_src = \
	[('radiant/' + p) for p in \
		['brushmanip.cpp',
		 'console.cpp',
		 'csg.cpp',
		 'entity.cpp',
		 'error.cpp',
		 'gtkdlgs.cpp',
		 'gtkmisc.cpp',
		 'main.cpp',
         'mainframe.cpp',
         'map.cpp',
         'nullmodel.cpp',
         'patchmanip.cpp',
         'plugin.cpp',
         'qgl.cpp',
         'referencecache.cpp',
         'renderstate.cpp',
         'select.cpp',
         'timer.cpp',
         'treemodel.cpp',
         'view.cpp',
         'winding.cpp',
         'windowobservers.cpp',
         'ui/einspector/EntityInspector.cpp',
         'ui/einspector/AddPropertyDialog.cpp',
         'ui/einspector/PropertyEditorFactory.cpp',
         'ui/einspector/Vector3PropertyEditor.cpp',
         'ui/einspector/BooleanPropertyEditor.cpp',
         'ui/einspector/EntityPropertyEditor.cpp',
         'ui/einspector/SkinPropertyEditor.cpp',
         'ui/einspector/SkinChooser.cpp',
         'ui/einspector/ColourPropertyEditor.cpp',
         'ui/einspector/TexturePropertyEditor.cpp',
         'ui/einspector/SoundPropertyEditor.cpp',
         'ui/einspector/SoundChooser.cpp',
         'ui/einspector/LightTextureChooser.cpp',
         'ui/einspector/FloatPropertyEditor.cpp',
         'ui/einspector/ModelPropertyEditor.cpp',
         'ui/einspector/PropertyEditor.cpp',
         'ui/entitychooser/EntityClassChooser.cpp',
         'ui/entitychooser/EntityClassTreePopulator.cpp',
         'ui/lightinspector/LightInspector.cpp',
         'ui/modelselector/ModelSelector.cpp',
         'ui/ortho/OrthoContextMenu.cpp',
         'ui/overlay/OverlayDialog.cpp',
         'ui/overlay/Overlay.cpp',
         'ui/common/ModelPreview.cpp',
         'ui/common/RenderableAABB.cpp',
         'ui/common/TexturePreviewCombo.cpp',
         'ui/common/ShaderSelector.cpp',
         'ui/common/ShaderChooser.cpp',
         'ui/common/SoundShaderPreview.cpp',
         'ui/mediabrowser/MediaBrowser.cpp',
         'ui/menu/FiltersMenu.cpp',
         'ui/colourscheme/ColourScheme.cpp',
         'ui/colourscheme/ColourSchemeManager.cpp',
         'ui/colourscheme/ColourSchemeEditor.cpp',
         'ui/particles/ParticlesChooser.cpp',
         'ui/patch/PatchInspector.cpp',
         'ui/patch/PatchCreateDialog.cpp',
         'ui/patch/PatchThickenDialog.cpp',
         'ui/surfaceinspector/SurfaceInspector.cpp',
         'ui/findshader/FindShader.cpp',
         'ui/transform/TransformDialog.cpp',
         'ui/groupdialog/GroupDialog.cpp',
         'ui/prefdialog/PrefPage.cpp',
         'ui/prefdialog/PrefDialog.cpp',
         'ui/entitylist/EntityList.cpp',
         'ui/about/AboutDialog.cpp',
         'ui/texturebrowser/TextureBrowser.cpp',
         'ui/mapinfo/MapInfoDialog.cpp',
         'ui/splash/Splash.cpp',
         'textool/TexTool.cpp',
         'textool/TexToolItem.cpp',
         'textool/item/PatchItem.cpp',
         'textool/item/PatchVertexItem.cpp',
         'textool/item/BrushItem.cpp',
         'textool/item/FaceItem.cpp',
         'selection/Manipulatables.cpp',
         'selection/Manipulators.cpp',
         'selection/BestPoint.cpp',
         'selection/Intersection.cpp',
         'selection/TransformationVisitors.cpp',
         'selection/SelectionTest.cpp',
         'selection/SelectObserver.cpp',
         'selection/ManipulateObserver.cpp',
         'selection/Planes.cpp',
         'selection/RadiantWindowObserver.cpp',
         'selection/RadiantSelectionSystem.cpp',
         'selection/algorithm/Primitives.cpp',
         'selection/algorithm/Curves.cpp',
         'selection/algorithm/ModelFinder.cpp',
         'selection/algorithm/Shader.cpp',
         'selection/algorithm/Group.cpp',
         'selection/algorithm/GroupCycle.cpp',
         'selection/algorithm/Transformation.cpp',
         'selection/shaderclipboard/ShaderClipboard.cpp',
         'selection/shaderclipboard/Texturable.cpp',
         'patch/Patch.cpp',
         'patch/PatchBezier.cpp',
         'patch/PatchInstance.cpp',
         'patch/PatchModule.cpp',
         'patch/PatchNode.cpp',
         'brushexport/BrushExportOBJ.cpp',
         'brush/BrushNode.cpp',
         'brush/BrushPrimitTexDef.cpp',
         'brush/TexDef.cpp',
         'brush/TextureProjection.cpp',
         'brush/FaceShader.cpp',
         'brush/FaceTexDef.cpp',
         'brush/FacePlane.cpp',
         'brush/Face.cpp',
         'brush/Brush.cpp',
         'brush/FaceInstance.cpp',
         'brush/BrushInstance.cpp',
         'brush/BrushModule.cpp',
         'brush/export/CollisionModel.cpp',
         'camera/CameraSettings.cpp',
         'camera/Camera.cpp',
         'camera/CamWnd.cpp',
         'camera/GlobalCamera.cpp',
         'xyview/XYWnd.cpp',
         'xyview/GlobalXYWnd.cpp',
         'ui/mru/MRU.cpp',
         'ui/mru/MRUMenuItem.cpp',
         'ui/commandlist/CommandList.cpp',
         'ui/commandlist/ShortcutChooser.cpp',
         'map/AutoSaver.cpp',
         'map/Map.cpp',
         'map/MapFileManager.cpp',
         'map/MapPosition.cpp',
         'map/MapPositionManager.cpp',
         'map/RegionManager.cpp',
         'map/RootNode.cpp',
         'map/PointFile.cpp',
         'map/FindMapElements.cpp',
         'map/algorithm/Traverse.cpp',
         'namespace/Namespace.cpp',
         'namespace/NameObserver.cpp',
         'render/backend/OpenGLStateBucket.cpp',
         'render/backend/OpenGLShader.cpp',
         'render/backend/GLProgramFactory.cpp',
         'settings/GameFileLoader.cpp',
         'settings/GameManager.cpp',
         'settings/Game.cpp',
         'settings/Win32Registry.cpp',
         'settings/PreferenceSystem.cpp',
         'scenegraph/CompiledGraph.cpp',
         'scenegraph/SceneGraphModule.cpp',
         'clipper/Clipper.cpp',
         'clipper/ClipPoint.cpp',
         'modulesystem/ApplicationContextImpl.cpp',
         'modulesystem/ModuleLoader.cpp',
         'modulesystem/DynamicLibrary.cpp',
         'modulesystem/DynamicLibraryLoader.cpp',
         'modulesystem/ModuleRegistry.cpp'
         ]
    ]

radiant_env.Prepend(LIBS = ['math', 'gtkutil', 'xmlutil'])
radiant_env.Prepend(LIBPATH = ['libs'])

# Win32 libs

if radiant_env['PLATFORM'] == 'win32':
    radiant_env.Append(LIBS = ['ws2_32', 'comdlg32'])
    radiant_src.append('radiant/multimon.cpp')

# Add the icon on Windows
if radiant_env['PLATFORM'] == 'win32':
	radiant_src += ['radiant/darkradiant.o']

radiant_prog = radiant_env.Program(target='darkradiant', 
								   source=radiant_src)

radiant_env.Depends(radiant_prog, gtkutil_lib)
radiant_env.Depends(radiant_prog, xmlutil)
radiant_env.Depends(radiant_prog, math)
radiant_env.Install(INSTALL, radiant_prog)

# Radiant post-install

if radiant_env['PLATFORM'] == 'win32':
    radiant_env.Install(INSTALL, ['#libxml2.w32/lib/libxml2.dll', 
		'#w32/vorbis/lib/ogg.dll',
		'#w32/vorbis/lib/vorbis.dll',
		'#w32/vorbis/lib/vorbisfile.dll',
		'#w32/openal/lib/alut.dll',
		'#w32/openal/lib/OpenAL32.dll',
		'#w32/openal/lib/wrap_oal.dll'])

# end setup ---------------------------------------------------------------------------------------
