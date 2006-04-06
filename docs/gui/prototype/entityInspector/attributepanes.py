import gtk
import pygtk

# CONSTANTS

PROPERTY_INACTIVE_TEXT = "Inactive properties are not applied to the selected entity. Their values are determined by in-game defaults."

# Simple label with custom text

class LabelAttributePane (gtk.VBox):
   
    def __init__(self, text):
        gtk.VBox.__init__(self)
        textLabel = gtk.Label('\n' + text)
        textLabel.set_line_wrap(True)
        self.pack_start(textLabel, False)

# Allows editing of a 3-component vector (point or size)
        
class Vector3AttributePane (gtk.VBox):
    
    def __init__(self, name):
        gtk.VBox.__init__(self)
        self.set_border_width(6)

        self.pack_start(gtk.CheckButton("Apply property"), False, False, 6)
        self.pack_start(self.createVectorBox(), False)
        self.pack_end(self.createButtons(), False)

    def createButtons(self):
        hbx = gtk.HBox(False, 6)
        hbx.pack_end(gtk.Button(' Apply '), False)
        hbx.pack_end(gtk.Button(' Reset '), False)
        return hbx
        
    def createVectorBox(self):
        hbx = gtk.VBox()
        #hbx.set_border_width(12)
        hbx.pack_start(self.createLabelledEntry('X '), False, False, 6)
        hbx.pack_start(self.createLabelledEntry('Y '), False, False, 6)
        hbx.pack_end(self.createLabelledEntry('Z '), False, False, 6)
        return hbx

    def createLabelledEntry(self, text):
        hbx = gtk.HBox()
        hbx.pack_start(gtk.Label(text), False, False, 12)
        hbx.pack_end(gtk.Entry(), True)
        return hbx