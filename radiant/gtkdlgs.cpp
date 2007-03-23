/*
Copyright (c) 2001, Loki software, inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list 
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

Neither the name of Loki software nor the names of its contributors may be used 
to endorse or promote products derived from this software without specific prior 
written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY 
DIRECT,INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

//
// Some small dialogs that don't need much
//
// Leonardo Zide (leo@lokigames.com)
//

#include "gtkdlgs.h"

#include "debugging/debugging.h"
#include "version.h"

#include "igl.h"
#include "iscenegraph.h"
#include "iselection.h"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtktable.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktextview.h>
#include <gtk/gtktextbuffer.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkliststore.h>

#include "os/path.h"
#include "math/aabb.h"
#include "container/array.h"
#include "generic/static.h"
#include "stream/stringstream.h"
#include "convert.h"
#include "gtkutil/messagebox.h"
#include "gtkutil/image.h"

#include "gtkmisc.h"
#include "brushmanip.h"
#include "qe3.h"
#include "texwindow.h"
#include "mainframe.h"
#include "preferences.h"
#include "cmdlib.h"

// =============================================================================
// Arbitrary Sides dialog

void DoSides (int type, int axis)
{
  ModalDialog dialog;
  GtkEntry* sides_entry;

  GtkWindow* window = create_dialog_window(MainFrame_getWindow(), "Arbitrary sides", G_CALLBACK(dialog_delete_callback), &dialog);

  GtkAccelGroup* accel = gtk_accel_group_new();
  gtk_window_add_accel_group(window, accel);

  {
    GtkHBox* hbox = create_dialog_hbox(4, 4);
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(hbox));
    {
      GtkLabel* label = GTK_LABEL(gtk_label_new("Sides:"));
      gtk_widget_show(GTK_WIDGET(label));
      gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(label), FALSE, FALSE, 0);
    }
    {
      GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
      gtk_widget_show(GTK_WIDGET(entry));
      gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(entry), FALSE, FALSE, 0);
      sides_entry = entry;
      gtk_widget_grab_focus(GTK_WIDGET(entry));
    }
    {
      GtkVBox* vbox = create_dialog_vbox(4);
      gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(vbox), TRUE, TRUE, 0);
      {
        GtkButton* button = create_dialog_button("OK", G_CALLBACK(dialog_button_ok), &dialog);
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(button), FALSE, FALSE, 0);
        widget_make_default(GTK_WIDGET(button));
        gtk_widget_add_accelerator(GTK_WIDGET(button), "clicked", accel, GDK_Return, (GdkModifierType)0, (GtkAccelFlags)0);
      }
      {
        GtkButton* button = create_dialog_button("Cancel", G_CALLBACK(dialog_button_cancel), &dialog);
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(button), FALSE, FALSE, 0);
        gtk_widget_add_accelerator(GTK_WIDGET(button), "clicked", accel, GDK_Escape, (GdkModifierType)0, (GtkAccelFlags)0);
      }
    }
  }

  if(modal_dialog_show(window, dialog) == eIDOK)
  {
    const char *str = gtk_entry_get_text(sides_entry);

    Scene_BrushConstructPrefab(GlobalSceneGraph(), (EBrushPrefab)type, atoi(str), TextureBrowser_GetSelectedShader(GlobalTextureBrowser()));
  }

  gtk_widget_destroy(GTK_WIDGET(window));
}

// =============================================================================
// About dialog (no program is complete without one)

void DoAbout()
{
  ModalDialog dialog;
  ModalDialogButton ok_button(dialog, eIDOK);

  GtkWindow* window = create_modal_dialog_window(MainFrame_getWindow(), "About DarkRadiant", dialog);

  {
    GtkVBox* vbox = create_dialog_vbox(4, 4);
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(vbox));

    {
      GtkHBox* hbox = create_dialog_hbox(4);
      gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(hbox), FALSE, TRUE, 0);

      {
        GtkVBox* vbox2 = create_dialog_vbox(4);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(vbox2), TRUE, FALSE, 0);
        {
          GtkFrame* frame = create_dialog_frame(0, GTK_SHADOW_IN);
          gtk_box_pack_start(GTK_BOX (vbox2), GTK_WIDGET(frame), FALSE, FALSE, 0);
          {
            GtkImage* image = new_local_image("logo.bmp");
            gtk_widget_show(GTK_WIDGET(image));
            gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(image));
          }
        }
      }

      {
		// RADIANT_VERSION set from makeversion.py during build
        GtkLabel* label = GTK_LABEL(gtk_label_new("DarkRadiant " RADIANT_VERSION "\n"
          __DATE__ "\n\n"
          "The Dark Mod (www.thedarkmod.com)\n\n"
          "This product contains software technology\n"
          "from id Software, Inc. ('id Technology').\n"
          "id Technology 2000 id Software,Inc.\n\n"
          "DarkRadiant is based on the GPL version\n"
          "of GtkRadiant (www.qeradiant.com)\n"
        ));
                       
        gtk_widget_show(GTK_WIDGET(label));
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(label), FALSE, FALSE, 0);
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_justify(label, GTK_JUSTIFY_LEFT);
      }

      {
        GtkVBox* vbox2 = create_dialog_vbox(4);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(vbox2), FALSE, TRUE, 0);
        {
          GtkButton* button = create_modal_dialog_button("OK", ok_button);
          gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET(button), FALSE, FALSE, 0);
        }
      }
    }
    {
      GtkFrame* frame = create_dialog_frame("OpenGL Properties");
      gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(frame), FALSE, FALSE, 0);
      {
        GtkTable* table = create_dialog_table(3, 2, 4, 4, 4);
        gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(table));
        {
          GtkLabel* label = GTK_LABEL(gtk_label_new("Vendor:"));
          gtk_widget_show(GTK_WIDGET(label));
          gtk_table_attach(table, GTK_WIDGET(label), 0, 1, 0, 1,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
          gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        }
        {
          GtkLabel* label = GTK_LABEL(gtk_label_new("Version:"));
          gtk_widget_show(GTK_WIDGET(label));
          gtk_table_attach(table, GTK_WIDGET(label), 0, 1, 1, 2,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
          gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        }
        {
          GtkLabel* label = GTK_LABEL(gtk_label_new("Renderer:"));
          gtk_widget_show(GTK_WIDGET(label));
          gtk_table_attach(table, GTK_WIDGET(label), 0, 1, 2, 3,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
          gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        }
        {
          GtkLabel* label = GTK_LABEL(gtk_label_new(reinterpret_cast<const char*>(glGetString(GL_VENDOR))));
          gtk_widget_show(GTK_WIDGET(label));
          gtk_table_attach(table, GTK_WIDGET(label), 1, 2, 0, 1,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
          gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        }
        {
          GtkLabel* label = GTK_LABEL(gtk_label_new(reinterpret_cast<const char*>(glGetString(GL_VERSION))));
          gtk_widget_show(GTK_WIDGET(label));
          gtk_table_attach(table, GTK_WIDGET(label), 1, 2, 1, 2,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
          gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        }
        {
          GtkLabel* label = GTK_LABEL(gtk_label_new(reinterpret_cast<const char*>(glGetString(GL_RENDERER))));
          gtk_widget_show(GTK_WIDGET(label));
          gtk_table_attach(table, GTK_WIDGET(label), 1, 2, 2, 3,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
          gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        }
      }
      {
        GtkFrame* frame = create_dialog_frame("OpenGL Extensions");
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(frame), TRUE, TRUE, 0);
        {
          GtkScrolledWindow* sc_extensions = create_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS, 4);
          gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET(sc_extensions));
          {
            GtkWidget* text_extensions = gtk_text_view_new();
            gtk_text_view_set_editable(GTK_TEXT_VIEW(text_extensions), FALSE);
            gtk_container_add (GTK_CONTAINER (sc_extensions), text_extensions);
            GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_extensions));
            gtk_text_buffer_set_text(buffer, reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)), -1);
            gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_extensions), GTK_WRAP_WORD);
            gtk_widget_show(text_extensions);
          }
        }
      }
    }
  }

  modal_dialog_show(window, dialog);

  gtk_widget_destroy(GTK_WIDGET(window));
}

// =============================================================================
// Text Editor dialog 

// master window widget
static GtkWidget *text_editor = 0;
static GtkWidget *text_widget; // slave, text widget from the gtk editor

static gint editor_delete (GtkWidget *widget, gpointer data)
{
  if (gtk_MessageBox (widget, "Close the shader editor ?", "Radiant", eMB_YESNO, eMB_ICONQUESTION) == eIDNO)
    return TRUE;

  gtk_widget_hide (text_editor);

  return TRUE;
}

static void editor_save (GtkWidget *widget, gpointer data)
{
  FILE *f = fopen ((char*)g_object_get_data (G_OBJECT (data), "filename"), "w");
  gpointer text = g_object_get_data (G_OBJECT (data), "text");

  if (f == 0)
  {
    gtk_MessageBox (GTK_WIDGET(data), "Error saving file !");
    return;
  }

  char *str = gtk_editable_get_chars (GTK_EDITABLE (text), 0, -1);
  fwrite (str, 1, strlen (str), f);
  fclose (f);
}

static void editor_close (GtkWidget *widget, gpointer data)
{
  if (gtk_MessageBox (text_editor, "Close the shader editor ?", "Radiant", eMB_YESNO, eMB_ICONQUESTION) == eIDNO)
    return;

  gtk_widget_hide (text_editor);
}

static void CreateGtkTextEditor()
{
  GtkWidget *dlg;
  GtkWidget *vbox, *hbox, *button, *scr, *text;

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  g_signal_connect(G_OBJECT(dlg), "delete_event",
                      G_CALLBACK(editor_delete), 0);
  gtk_window_set_default_size (GTK_WINDOW (dlg), 600, 300);

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_widget_show (vbox);
  gtk_container_add(GTK_CONTAINER(dlg), GTK_WIDGET(vbox));
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  scr = gtk_scrolled_window_new (0, 0);
  gtk_widget_show (scr);
  gtk_box_pack_start(GTK_BOX(vbox), scr, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scr), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scr), GTK_SHADOW_IN);

  text = gtk_text_view_new();
  gtk_container_add (GTK_CONTAINER (scr), text);
  gtk_widget_show (text);
  g_object_set_data (G_OBJECT (dlg), "text", text);
  gtk_text_view_set_editable (GTK_TEXT_VIEW(text), TRUE);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_widget_show (hbox);
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(hbox), FALSE, TRUE, 0);

  button = gtk_button_new_with_label ("Close");
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(button), "clicked",
		      G_CALLBACK(editor_close), dlg);
  gtk_widget_set_usize (button, 60, -2);

  button = gtk_button_new_with_label ("Save");
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(button), "clicked",
		      G_CALLBACK(editor_save), dlg);
  gtk_widget_set_usize (button, 60, -2);

  text_editor = dlg;
  text_widget = text;
}

static void DoGtkTextEditor (const char* filename, guint cursorpos)
{
  if (!text_editor)
    CreateGtkTextEditor(); // build it the first time we need it

  // Load file
  FILE *f = fopen (filename, "r");

  if (f == 0)
  {
    globalOutputStream() << "Unable to load file " << filename << " in shader editor.\n";
    gtk_widget_hide (text_editor);
  }
  else
  {
    fseek (f, 0, SEEK_END);
    int len = ftell (f);
    void *buf = malloc (len);
    void *old_filename;

    rewind (f);
    fread (buf, 1, len, f);

    gtk_window_set_title (GTK_WINDOW (text_editor), filename);

    GtkTextBuffer* text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_widget));
    gtk_text_buffer_set_text(text_buffer, (char*)buf, len);

    old_filename = g_object_get_data (G_OBJECT (text_editor), "filename");
    if (old_filename)
      free(old_filename);
    g_object_set_data (G_OBJECT (text_editor), "filename", strdup (filename));

    // trying to show later
    gtk_widget_show (text_editor);

#ifdef WIN32
    process_gui();
#endif

    // only move the cursor if it's not exceeding the size..
    // NOTE: this is erroneous, cursorpos is the offset in bytes, not in characters
    // len is the max size in bytes, not in characters either, but the character count is below that limit..
    // thinking .. the difference between character count and byte count would be only because of CR/LF?
    {
      GtkTextIter text_iter;
      // character offset, not byte offset
      gtk_text_buffer_get_iter_at_offset(text_buffer, &text_iter, cursorpos);
      gtk_text_buffer_place_cursor(text_buffer, &text_iter);
    }

#ifdef WIN32
    gtk_widget_queue_draw(text_widget);
#endif

    free (buf);
    fclose (f);
  }
}

// =============================================================================
// Light Intensity dialog 

EMessageBoxReturn DoLightIntensityDlg (int *intensity)
{
  ModalDialog dialog;
  GtkEntry* intensity_entry;
  ModalDialogButton ok_button(dialog, eIDOK);
  ModalDialogButton cancel_button(dialog, eIDCANCEL);

  GtkWindow* window = create_modal_dialog_window(MainFrame_getWindow(), "Light intensity", dialog, -1, -1);

  GtkAccelGroup *accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(window, accel_group);

  {
    GtkHBox* hbox = create_dialog_hbox(4, 4);
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(hbox));
    {
      GtkVBox* vbox = create_dialog_vbox(4);
      gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(vbox), TRUE, TRUE, 0);
      {
        GtkLabel* label = GTK_LABEL(gtk_label_new("ESC for default, ENTER to validate"));
        gtk_widget_show(GTK_WIDGET(label));
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(label), FALSE, FALSE, 0);
      }
      {
        GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
        gtk_widget_show(GTK_WIDGET(entry));
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(entry), TRUE, TRUE, 0);

        gtk_widget_grab_focus(GTK_WIDGET(entry));

        intensity_entry = entry;
      }
    }
    {
      GtkVBox* vbox = create_dialog_vbox(4);
      gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(vbox), FALSE, FALSE, 0);

      {
        GtkButton* button = create_modal_dialog_button("OK", ok_button);
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(button), FALSE, FALSE, 0);
        widget_make_default(GTK_WIDGET(button));
        gtk_widget_add_accelerator(GTK_WIDGET(button), "clicked", accel_group, GDK_Return, (GdkModifierType)0, GTK_ACCEL_VISIBLE);
      }
      {
        GtkButton* button = create_modal_dialog_button("Cancel", cancel_button);
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(button), FALSE, FALSE, 0);
        gtk_widget_add_accelerator(GTK_WIDGET(button), "clicked", accel_group, GDK_Escape, (GdkModifierType)0, GTK_ACCEL_VISIBLE);
      }
    }
  }

  char buf[16];
  sprintf (buf, "%d", *intensity);
  gtk_entry_set_text(intensity_entry, buf);

  EMessageBoxReturn ret = modal_dialog_show(window, dialog);
  if(ret == eIDOK)
    *intensity = atoi (gtk_entry_get_text(intensity_entry));

  gtk_widget_destroy(GTK_WIDGET(window));

  return ret;
}


#ifdef WIN32
#include <gdk/gdkwin32.h>
#endif

#ifdef WIN32
  // use the file associations to open files instead of builtin Gtk editor
bool g_TextEditor_useWin32Editor = true;
#else
  // custom shader editor
bool g_TextEditor_useCustomEditor = false;
CopiedString g_TextEditor_editorCommand("");
#endif

void DoTextEditor (const char* filename, int cursorpos)
{
#ifdef WIN32
  if (g_TextEditor_useWin32Editor)
  {
    globalOutputStream() << "opening file '" << filename << "' (line " << cursorpos << " info ignored)\n";
    ShellExecute((HWND)GDK_WINDOW_HWND (GTK_WIDGET(MainFrame_getWindow())->window), "open", filename, 0, 0, SW_SHOW );
    return;
  }
#else
  // check if a custom editor is set
  if(g_TextEditor_useCustomEditor && !g_TextEditor_editorCommand.empty())
  {
	StringOutputStream strEditCommand(256);
    strEditCommand << g_TextEditor_editorCommand.c_str() << " \"" << filename << "\"";
    
    globalOutputStream() << "Launching: " << strEditCommand.c_str() << "\n";
    // note: linux does not return false if the command failed so it will assume success
    if (Q_Exec(0, const_cast<char*>(strEditCommand.c_str()), 0, true) == false)
    {
      globalOutputStream() << "Failed to execute " << strEditCommand.c_str() << ", using default\n";
    }
    else
    {
      // the command (appeared) to run successfully, no need to do anything more
      return;
    }
  }
#endif
  
  DoGtkTextEditor (filename, cursorpos);
}
