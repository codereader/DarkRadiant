import os, glob
import xml.etree.ElementTree as ElementTree
import shutil

pot_file = "darkradiant.pot"

if os.path.exists(pot_file):
	print('Removing existing pot file: ' + pot_file)
	os.remove(pot_file)

# xgettext needs to be in your path
xgettext_cmd = "xgettext";
xgettext_options = " --c++ --keyword=_ --keyword=N_ -o " + pot_file + " --msgid-bugs-address=greebo@angua.at ";

def invokeXGettext(targetfile):
	norm_filename = os.path.normpath(targetfile);

	if os.path.exists(pot_file):
		cmdline = xgettext_cmd + xgettext_options + " -j " + norm_filename
	else:
		cmdline = xgettext_cmd + xgettext_options + norm_filename

	#print(cmdline)
	os.system(cmdline)

def scanCodeFiles(path, exclude_directories):
	#print("Inspecting path: " + os.path.join(path, '*'))
	for current_file in glob.glob( os.path.join(path, '*') ):
		# Dive into directories
		if os.path.isdir(current_file):
			dir_normalised = current_file.replace('\\', '/')
			if not dir_normalised in exclude_directories:
				#print 'got a directory: ' + dir_normalised
				scanCodeFiles(dir_normalised, exclude_directories)
			else:
				print('skipping directory: ' + dir_normalised)
		else:
			# Analyse files
			ext = os.path.splitext(current_file)[1]

			if ext in [".cpp", ".h"]:
				print("Processing C++ file: " + current_file)
				invokeXGettext(current_file)

def processXmlFile(path, nodeXPath, elementNames, attributes):
	print("Processing XML file: " + path)

	doc = ElementTree.parse(path)

	elements = doc.findall(nodeXPath)

	found_values = []

	for element in elements:
		if len(attributes) > 0:
			for attr in attributes:
				if element.attrib.get(attr):
					found_values.append(element.attrib.get(attr))
		else:
			if len(elementNames) == 0:
				# Get the actual value of the element
				print(element.text)
				found_values.append(element.text)
			else:
				for elementName in elementNames:
					if element.tag == elementName and not element.text is None:
						#print('Found ' + element.tag)
						found_values.append(element.text)	

	# Now generate a dummy C++ file for xgettext to parse

	if len(found_values) > 0:

		dummy_file = "xml_file_content.cpp"

		file = open(dummy_file, "w")

		for value in found_values:
			value = value.replace("\n", "\\n")
			value = value.replace("\"", "\\\"")
			file.write('_("' + value + '")' + "\n")

		file.close()

		invokeXGettext(dummy_file)

		os.remove(dummy_file)

print("------- Scanning C++ Files ----------------");

build_root = '../../'

code_directories = ['radiant', 'plugins', 'libs']
exclude_directories = ['../../libs/pybind']

for directory in code_directories:
	scanCodeFiles(os.path.join(build_root, directory), exclude_directories)

print("------- Scanning XML Files ----------------");

xml_directory = "install"
empty = []

# Add menu captions
processXmlFile(os.path.join(build_root, xml_directory, "menu.xml"), ".//*", empty, ["caption"])

# Add colour scheme names
processXmlFile(os.path.join(build_root, xml_directory, "colours.xml"), ".//descriptions/*", empty, ["value"])

# Add toolbar tooltips
processXmlFile(os.path.join(build_root, xml_directory, "user.xml"), ".//toolbutton", empty, ["tooltip"])
processXmlFile(os.path.join(build_root, xml_directory, "user.xml"), ".//toggletoolbutton", empty, ["tooltip"])

# Check UI files for translatable strings
for current_file in glob.glob( os.path.join(build_root, xml_directory, "ui", '*.xrc') ):
    processXmlFile(current_file, ".//*", ['{http://www.wxwindows.org/wxxrc}label'], empty)

# Copy the POT file to the install folder
shutil.copy(pot_file, os.path.join(build_root, "install", "i18n"))