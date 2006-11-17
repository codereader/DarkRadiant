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
xmlutilSource = 'Document.cpp Node.cpp XMLRegistry.cpp'
xmlutil = xmlutilEnv.StaticLibrary(target='libs/xmlutil', source=build_list('libs/xmlutil', xmlutilSource))

cmdlib_lib = g_env.StaticLibrary(target='libs/cmdlib', source='libs/cmdlib/cmdlib.cpp')

mathlib_src = 'mathlib.c bbox.c line.c m4x4.c ray.c'
mathlib_lib = g_env.StaticLibrary(target='libs/mathlib', source=build_list('libs/mathlib', mathlib_src))

mathEnv = g_env.Copy()
mathSrc = 'aabb.cpp'
math = mathEnv.StaticLibrary(target='libs/math', source=build_list('libs/math', mathSrc))

md5lib_lib = g_env.StaticLibrary(target='libs/md5lib', source='libs/md5lib/md5lib.c')

ddslib_lib = g_env.StaticLibrary(target='libs/ddslib', source='libs/ddslib/ddslib.c')

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

profile_env = g_env.Copy();
profile_env['CPPPATH'].append('include')
profile_src = 'profile.cpp file.cpp'
profile_lib = profile_env.StaticLibrary(target='libs/profile', source=build_list('libs/profile', profile_src))

gtkutil_env = g_env.Copy();
gtkutil_env['CPPPATH'].append('include')
gtkutil_env.useOpenGL()
gtkutil_env.useGlib2()
gtkutil_env.useGtk2()
gtkutil_env.useGtkGLExt()
gtkutil_env.useXML2()

gtkutil_src = '\
  accelerator.cpp\
  button.cpp\
  clipboard.cpp\
  closure.cpp\
  container.cpp\
  cursor.cpp\
  dialog.cpp\
  entry.cpp\
  frame.cpp\
  filechooser.cpp\
  glfont.cpp\
  glwidget.cpp\
  image.cpp\
  idledraw.cpp\
  menu.cpp\
  messagebox.cpp\
  nonmodal.cpp\
  paned.cpp\
  pointer.cpp\
  widget.cpp\
  window.cpp\
  xorrectangle.cpp\
'

gtkutil_lib = gtkutil_env.StaticLibrary(target='libs/gtkutil', source=build_list('libs/gtkutil', gtkutil_src))

# end static / common libraries ---------------------------------------------------

# radiant, modules and plugins ----------------------------------------------------

module_env = g_env.Copy()
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
filterSrc = build_list('plugins/filters', 'filters.cpp XMLFilter.cpp')
filterEnv.useBoostRegex()
filterEnv.Append(LIBS = 'xmlutil')
filterLib = filterEnv.SharedLibrary(target='filters', source=filterSrc, no_import_lib=1)
filterEnv.Install(INSTALL + '/modules', filterLib)

vfspk3_env = module_env.Copy()
vfspk3_lst = build_list('plugins/vfspk3', 'vfspk3.cpp vfs.cpp archive.cpp')
vfspk3_env.useGlib2()
vfspk3_lib = vfspk3_env.SharedLibrary(target='vfspk3', source=vfspk3_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
vfspk3_env.Install(INSTALL + '/modules', vfspk3_lib)

archivepak_env = module_env.Copy()
archivepak_lst = build_list('plugins/archivepak', 'plugin.cpp archive.cpp pak.cpp')
archivepak_lib = archivepak_env.SharedLibrary(target='archivepak', source=archivepak_lst, LIBS='cmdlib', LIBPATH='libs', no_import_lib=1, WIN32_INSERT_DEF=0)
archivepak_env.Depends(archivepak_lib, cmdlib_lib)
archivepak_env.Install(INSTALL + '/modules', archivepak_lib)

archivewad_env = module_env.Copy()
archivewad_lst = build_list('plugins/archivewad', 'plugin.cpp archive.cpp wad.cpp')
archivewad_lib = archivewad_env.SharedLibrary(target='archivewad', source=archivewad_lst, LIBS='cmdlib', LIBPATH='libs', no_import_lib=1, WIN32_INSERT_DEF=0)
archivewad_env.Depends(archivewad_lib, cmdlib_lib)
archivewad_env.Install(INSTALL + '/modules', archivewad_lib)

archivezip_env = module_env.Copy()
archivezip_lst = build_list('plugins/archivezip', 'plugin.cpp archive.cpp pkzip.cpp zlibstream.cpp')
archivezip_env.useZLib()
archivezip_env.Append(LIBPATH = ['libs'])
archivezip_env.Append(LIBS = ['cmdlib'])
archivezip_lib = archivezip_env.SharedLibrary(target='archivezip', source=archivezip_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
archivezip_env.Depends(archivezip_lib, cmdlib_lib)
archivezip_env.Install(INSTALL + '/modules', archivezip_lib)

shaders_env = module_env.Copy()
shaders_lst = build_list('plugins/shaders', 'shaders.cpp plugin.cpp ShaderTemplate.cpp')
shaders_env.useGlib2()
shaders_env.Append(LIBS = ['cmdlib', 'xmlutil'])
shaders_env.Append(LIBPATH = ['libs'])
shaders_lib = shaders_env.SharedLibrary(target='shaders', source=shaders_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
shaders_env.Depends(shaders_lib, cmdlib_lib)
shaders_env.Depends(shaders_lib, xmlutil)
shaders_env.Install(INSTALL + '/modules', shaders_lib)

image_env = module_env.Copy()
image_lst = build_list('plugins/image', 'bmp.cpp jpeg.cpp image.cpp pcx.cpp tga.cpp dds.cpp')
image_lib = image_env.SharedLibrary(target='image', source=image_lst, LIBS=['jpeg6', 'ddslib'], LIBPATH='libs', no_import_lib=1, WIN32_INSERT_DEF=0)
image_env.Depends(image_lib, jpeg_lib)
image_env.Depends(image_lib, ddslib_lib)
image_env.Install(INSTALL + '/modules', image_lib)

mapdoom3_env = module_env.Copy()
mapdoom3_lst=build_list('plugins/mapdoom3', 'mapdoom3.cpp parse.cpp write.cpp')
mapdoom3_env.Append(LIBS = ['cmdlib', 'xmlutil'])
mapdoom3_lib = mapdoom3_env.SharedLibrary(target='mapdoom3', source=mapdoom3_lst)
mapdoom3_env.Depends(mapdoom3_lib, cmdlib_lib)
mapdoom3_env.Depends(mapdoom3_lib, xmlutil)
mapdoom3_env.Install(INSTALL + '/modules', mapdoom3_lib)

imagepng_env = module_env.Copy()
imagepng_lst = build_list('plugins/imagepng', 'plugin.cpp')
imagepng_env.usePNG()
imagepng_lib = imagepng_env.SharedLibrary(target='imagepng', source=imagepng_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
imagepng_env.Install(INSTALL + '/modules', imagepng_lib)

model_env = module_env.Copy()
model_lst = build_list('plugins/model', 'plugin.cpp model.cpp RenderablePicoModel.cpp RenderablePicoSurface.cpp')
model_env.Append(LIBS = ['mathlib', 'picomodel', 'math'])
model_lib = model_env.SharedLibrary(target='model', source=model_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
model_env.Depends(model_lib, mathlib_lib)
model_env.Depends(model_lib, picomodel_lib)
model_env.Depends(model_lib, math)
model_env.Install(INSTALL + '/modules', model_lib)

md3model_env = module_env.Copy()
md3model_lst=build_list('plugins/md3model', 'plugin.cpp md5.cpp')
md3model_env.Append(LIBS = ['math'])
md3model_lib = md3model_env.SharedLibrary(target='md3model', source=md3model_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
md3model_env.Depends(md3model_lib, math)
md3model_env.Install(INSTALL + '/modules', md3model_lib)

entity_env = module_env.Copy()
entity_lst = build_list('plugins/entity', 'plugin.cpp entity.cpp eclassmodel.cpp generic.cpp group.cpp light.cpp miscmodel.cpp doom3group.cpp skincache.cpp angle.cpp angles.cpp colour.cpp model.cpp namedentity.cpp origin.cpp scale.cpp targetable.cpp rotation.cpp modelskinkey.cpp')
entity_env.Append(LIBS = ['math', 'xmlutil'])
entity_lib = entity_env.SharedLibrary(target='entity', source=entity_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
entity_env.Depends(entity_lib, math)
entity_env.Install(INSTALL + '/modules', entity_lib)

brushexport_env = module_env.Copy()
brushexport_lst = build_list('contrib/brushexport', 'plugin.cpp')
brushexport_env.useGlib2()
brushexport_env.useGtk2()
brushexport_lib = brushexport_env.SharedLibrary(target='brushexport', source=brushexport_lst)
brushexport_env.Install(INSTALL + '/plugins', brushexport_lib)

radiant_env = g_env.Copy()
radiant_env['CPPPATH'].append('include')
if radiant_env['PLATFORM'] == 'posix':
    radiant_env['LINKFLAGS'] += '-ldl '
if ( OS == 'Darwin' ):
  radiant_env['CXXFLAGS'] += '-fno-common '
  radiant_env['CCFLAGS'] += '-fno-common '
  radiant_env['LINKFLAGS'] += '-lX11 -lGL -lGLU '
radiant_env['LIBPREFIX'] = ''
radiant_env.useGlib2()
radiant_env.useXML2()
radiant_env.useGtk2()
radiant_env.useGtkGLExt()
radiant_env.useOpenGL()

radiant_src = [
'autosave.cpp',
'brush.cpp',
'brushmanip.cpp',
'brushmodule.cpp',
'brushnode.cpp',
'brush_primit.cpp',
'camwindow.cpp',
'clippertool.cpp',
'commands.cpp',
'console.cpp',
'csg.cpp',
'dialog.cpp',
'eclass_doom3.cpp',
'entity.cpp',
'entitylist.cpp',
'environment.cpp',
'error.cpp',
'feedback.cpp',
'filetypes.cpp',
'findtexturedialog.cpp',
'glwidget.cpp',
'grid.cpp',
'groupdialog.cpp',
'gtkdlgs.cpp',
'gtkmisc.cpp',
'image.cpp',
'main.cpp',
'mainframe.cpp',
'map.cpp',
'mru.cpp',
'nullmodel.cpp',
'parse.cpp',
'patch.cpp',
'patchdialog.cpp',
'patchmanip.cpp',
'patchmodule.cpp',
'plugin.cpp',
'pluginapi.cpp',
'pluginmanager.cpp',
'pluginmenu.cpp',
'points.cpp',
'preferencedictionary.cpp',
'preferences.cpp',
'qe3.cpp',
'qgl.cpp',
'referencecache.cpp',
'renderer.cpp',
'renderstate.cpp',
'scenegraph.cpp',
'select.cpp',
'selection.cpp',
'server.cpp',
'shaders.cpp',
'surfacedialog.cpp',
'texmanip.cpp',
'textures.cpp',
'texwindow.cpp',
'timer.cpp',
'treemodel.cpp',
'undo.cpp',
'view.cpp',
'winding.cpp',
'windowobservers.cpp',
'xmlstuff.cpp',
'xywindow.cpp',
'ui/einspector/EntityInspector.cpp',
'ui/einspector/AddPropertyDialog.cpp',
'ui/einspector/PropertyEditor.cpp',
'ui/einspector/PropertyEditorFactory.cpp',
'ui/einspector/Vector3PropertyEditor.cpp',
'ui/einspector/BooleanPropertyEditor.cpp',
'ui/einspector/EntityPropertyEditor.cpp',
#'ui/einspector/ClassnamePropertyEditor.cpp',
'ui/einspector/ColourPropertyEditor.cpp',
'ui/einspector/TexturePropertyEditor.cpp',
'ui/einspector/TextureChooser.cpp',
'ui/ortho/OrthoContextMenu.cpp',
'ui/ortho/EntityClassChooser.cpp',
'ui/modelselector/ModelSelector.cpp',
'ui/common/ModelPreview.cpp',
'ui/common/RenderableAABB.cpp',
'ui/common/ToolbarCreator.cpp',
'ui/common/TexturePreviewCombo.cpp',
'ui/mediabrowser/MediaBrowser.cpp',
'ui/menu/FiltersMenu.cpp',
'ui/colourscheme/ColourScheme.cpp',
'ui/colourscheme/ColourSchemeManager.cpp',
'ui/colourscheme/ColourSchemeEditor.cpp',
'selection/Manipulatables.cpp',
'selection/Manipulators.cpp',
'selection/BestPoint.cpp',
'selection/Intersection.cpp'
]

for i in range(len(radiant_src)):
  radiant_src[i] = 'radiant/' + radiant_src[i]

radiant_env.Prepend(LIBS = ['mathlib', 'math', 'cmdlib', 'profile', 'gtkutil', 'xmlutil'])
radiant_env.Prepend(LIBPATH = ['libs'])

# Win32 libs

if radiant_env['PLATFORM'] == 'win32':
    radiant_env.Append(LIBS = ['ws2_32', 'comdlg32'])
    radiant_src.append('radiant/multimon.cpp')

radiant_prog = radiant_env.Program(target='darkradiant', source=radiant_src)
radiant_env.Depends(radiant_prog, mathlib_lib)
radiant_env.Depends(radiant_prog, cmdlib_lib)
radiant_env.Depends(radiant_prog, profile_lib)
radiant_env.Depends(radiant_prog, gtkutil_lib)
radiant_env.Depends(radiant_prog, xmlutil)
radiant_env.Depends(radiant_prog, math)
radiant_env.Install(INSTALL, radiant_prog)

# Radiant post-install

if radiant_env['PLATFORM'] == 'win32':
    radiant_env.Install(INSTALL, '#libxml2.w32/lib/libxml2.dll')


# end setup ---------------------------------------------------------------------------------------
