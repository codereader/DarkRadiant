# Test creating a new dialog
dialog = GlobalDialogManager.createDialog("Test")

# Add a label
dialog.addLabel("Testlabel")

# Add an entry box and remember the handle
entryHandle = dialog.addEntryBox("Entry")

# Add a spin button
dialog.addSpinButton("Spinner", 0, 10, 0.5)

# Add a combo box, the options must be passed in the form of a StringVector
options = StringVector()
options.append("Test1")
options.append("Test2")
dialog.addComboBox("Test", options)

# Add a simple checkbox
dialog.addCheckbox("TestCheckbox")

if dialog.run() == Dialog.OK:
	dialog = GlobalDialogManager.createMessageBox("Result", "User pressed OK, entry is: <b>" + dialog.getElementValue(entryHandle) + "</b>", Dialog.CONFIRM)
	dialog.setTitle("Result Popup Message")
	dialog.run()
