import gtk
import pygtk
import attributepanes

# CONSTANTS

NO_SELECTION_PANE_TEXT = "Select a property to edit in this category"

# Main EntityInspector class

class EntityInspector:
    
    # Main GTK window
    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    
    def __init__(self):
        
        splitPane = gtk.VBox()
        splitPane.pack_start(self.createSelectionTreeView())
        splitPane.pack_end(self.createDialogPane())
        splitPane.show()

        self.window.add(splitPane)
        self.window.show_all()
        self.window.connect("delete_event", gtk.main_quit)

    def switchPaneContent(self, text, widget):
        currentEditPane = self.editorFrame.get_child()
        if currentEditPane != None:
            self.editorFrame.remove(currentEditPane)
        
        self.editorFrame.set_label(text)
        self.editorFrame.add(widget)
        self.editorFrame.show_all()
        
    def treeSelectionChanged(self, data):
        selection = self.treeView.get_selection()
        (model, iter) = selection.get_selected()
        val = self.treeStore.get_value(iter, 0)
        if len(self.treeStore.get_path(iter)) == 1:
            self.switchPaneContent(val, attributepanes.LabelAttributePane(NO_SELECTION_PANE_TEXT))
        else:
            self.switchPaneContent(val, attributepanes.Vector3AttributePane(val))
        
    def createSelectionTreeView(self):
        vbx = gtk.VBox()
        self.treeStore = gtk.TreeStore(str)
        
        iBasic = self.treeStore.append(None, ['Basic'])
        self.treeStore.append(iBasic, ['Origin'])
        self.treeStore.append(iBasic, ['Colour'])
        
        iLight = self.treeStore.append(None, ['Light'])
        self.treeStore.append(iLight, ['Radius'])
        self.treeStore.append(iLight, ['Center'])
        self.treeStore.append(iLight, ['Cast shadows'])
        self.treeStore.append(iLight, ['Shader'])

        self.treeView = gtk.TreeView(self.treeStore)

        column = gtk.TreeViewColumn('Standard properties')
        cellR = gtk.CellRendererText()
        column.pack_start(cellR, True)
        column.add_attribute(cellR, 'text', 0)
        self.treeView.append_column(column)

        self.treeView.connect("cursor-changed", self.treeSelectionChanged)
        
        scrollWin = gtk.ScrolledWindow()
        scrollWin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        scrollWin.add_with_viewport(self.treeView)
        scrollWin.set_size_request(400, 300)

        vbx.pack_start(scrollWin, True, True, 0)
        vbx.pack_end(self.createAllPropertiesRow(), False)
        return vbx
    
    def createAllPropertiesRow(self):
        hbx = gtk.HBox()
        hbx.set_border_width(6)
        hbx.pack_start(gtk.Label("2 unrecognised properties on entity"), False)
        hbx.pack_end(gtk.Button("All properties..."), False)
        return hbx

    def createRightAlignedButton(self, text):
        hbx = gtk.HBox()
        hbx.pack_end(gtk.Button(text), False)
        return hbx
    
    def createLeftAlignedLabel(self, text):
        hbx = gtk.HBox()
        hbx.pack_start(gtk.Label(text), False)
        return hbx
        
    def createDialogPane(self):
        vbx = gtk.VBox()
        self.editorFrame = gtk.Frame("Edit property")
        self.editorFrame.set_border_width(6)
        #editorFrame.set_shadow_type(gtk.SHADOW_IN)
        vbx.pack_start(self.editorFrame, True, True)
        vbx.set_size_request(0, 250)
        
        return vbx
        
    def main(self):
        gtk.main()
        
if __name__ == "__main__":
    entityInspector = EntityInspector()
    entityInspector.main()