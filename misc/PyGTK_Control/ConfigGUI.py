#!/usr/bin/env python

# MyServer
# Copyright (C) 2002, 2003, 2004, 2007, 2008 The MyServer Team
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


import sys
import gobject

try:
    import pygtk
    pygtk.require("2.0")
except:
    pass
try:
    import gtk
    import gtk.glade
except:
    sys.exit(1)

class ConfigGUIGTK:
    """@package ConfigGUIGTK
    UI for configurtion tool"""

    def __init__(self):

        # Connect the Glade file
        gladefile = "XMLGui.glade"
        self.wTree = gtk.glade.XML(gladefile)

        # Bind main window
        self.window = self.wTree.get_widget("ConfigUI")
        # Bind "destroy" event
        if (self.window):
            self.window.connect("destroy", gtk.main_quit)

        # Here we call the methods creating widgets with functionality
        self.create_treeview_for_default_filenames()
        self.create_treeview_for_host_names()
        self.create_treeview_for_IP_addresses()
        self.create_a_combobox_for_connection_types()
        self.create_treeview_with_MIME_types()
        self.bind_signals()

    def create_treeview_for_default_filenames(self):
        """ Create treeview widget structure (one column) for default filenames """
        self.treeview=self.wTree.get_widget("treeview1")
        self.treemodel=gtk.TreeStore(gobject.TYPE_STRING)
        self.treeview.set_model(self.treemodel)
        # Add treeview header
        self.treeview.set_headers_visible(True)
        renderer=gtk.CellRendererText()
        column=gtk.TreeViewColumn("Default file names:",renderer, text=0)
        column.set_resizable(True)
        self.treeview.append_column(column)
        renderer=gtk.CellRendererText()

        # Add list items to tree view
        self.insert_row(self.treemodel,None,'default.html')
        self.insert_row(self.treemodel,None,'default.htm')
        self.insert_row(self.treemodel,None,'default.php')
        self.insert_row(self.treemodel,None,'index.html')
        self.insert_row(self.treemodel,None,'index.htm')
        self.insert_row(self.treemodel,None,'index.php')

        self.treeview.show()

    def create_treeview_for_host_names(self):
        """ Create treeview widget structure (one column) for host names """
        self.treeviewHost=self.wTree.get_widget("treeviewHost")
        self.treemodelHost=gtk.TreeStore(gobject.TYPE_STRING)
        self.treeviewHost.set_model(self.treemodelHost)

        renderer=gtk.CellRendererText()
        column=gtk.TreeViewColumn("Host:",renderer, text=0)
        column.set_resizable(True)
        self.treeviewHost.append_column(column)
        renderer=gtk.CellRendererText()
        self.treeviewHost.show()

    def create_treeview_for_IP_addresses(self):
        """ Create treeview widget structure (one column) for IP addresses """
        self.treeviewIP=self.wTree.get_widget("treeviewIP")
        self.treemodelIP=gtk.TreeStore(gobject.TYPE_STRING)
        self.treeviewIP.set_model(self.treemodelIP)

        renderer=gtk.CellRendererText()
        column=gtk.TreeViewColumn("IP:",renderer, text=0)
        column.set_resizable(True)
        self.treeviewIP.append_column(column)
        renderer=gtk.CellRendererText()
        self.treeviewIP.show()

    def create_a_combobox_for_connection_types(self):
        """ Cretes a combobox with connection types """
        self.store2 = gtk.ListStore(gobject.TYPE_STRING)
        #populate
        self.store2.append(["Every HTTP connection"])
        self.store2.append(["FTP connection"])
        self.cbHost=self.wTree.get_widget("combobox5")
        self.cbHost.set_model(self.store2)
        cell = gtk.CellRendererText()
        self.cbHost.pack_start(cell, True)
        self.cbHost.add_attribute(cell, 'text',0)

    def create_treeview_with_MIME_types(self):
        """ Add list items to tree view. The types are parsed
        from xml file using helper class MIMEtpe
        the result is used to poulate the treeview and the combobox """

        from MIMEtypes import MIMEtype, ParseTypes
        TYPES = ParseTypes("MIMEtypes.xml")

        #Create combobox to select MIME type
        # http://faq.pygtk.org/index.py?req=show&file=faq16.008.htp
        self.comboboxMIME = self.wTree.get_widget("combobox3")
        self.store = gtk.ListStore(gobject.TYPE_STRING)
        #populate
        for element in TYPES:
            self.store.append([str(element.MIME)])
        self.comboboxMIME.set_model(self.store)
        cell = gtk.CellRendererText()
        self.comboboxMIME.pack_start(cell, True)
        self.comboboxMIME.add_attribute(cell, 'text',0)

        # Crete treeview widget with file extensions
        self.treeviewEXT=self.wTree.get_widget("treeview2")
        self.treemodelEXT=gtk.TreeStore(gobject.TYPE_STRING)
        self.treeviewEXT.set_model(self.treemodelEXT)
        # Add treeview header
        self.treeviewEXT.set_headers_visible(True)
        renderer=gtk.CellRendererText()
        column=gtk.TreeViewColumn("Extension:",renderer, text=0)
        column.set_resizable(True)
        self.treeviewEXT.append_column(column)
        renderer=gtk.CellRendererText()

        #populate
        for element in TYPES:
            self.insert_row(self.treemodelEXT,None,element.ext)
        self.treeviewEXT.show()

    def bind_signals(self):
        """ This method creates a dictionary of pairs
        signal naem : event, and then binds them
        using signal_autoconnect method """

        dic = { "on_btnAddFileName_clicked" : \
                self.buttonAddefaultFileName_clicked,
                "on_btnRemoveFileName_clicked" : \
                self.buttonRemoveDefultFileName_clicked,
                "on_btnAddMIMEExtenesion_clicked" : \
                self.buttonAddMIMEExtension_clicked,
                "on_btnAddMIMEType_clicked" : \
                self.buttonAddMIMEType_clicked,
                "on_filechooserbutton2_file_set" : \
                self.filechooserbutton2_file_set,
                "on_filechooserbutton1_file_set" : \
                self.filechooserbutton1_file_set,
                "on_filechooserbutton4_file_set" : \
                self.filechooserbutton4_file_set,
                "on_filechooserbutton3_file_set" : \
                self.filechooserbutton3_file_set,
                "on_btnRemoveMIMEExtension_clicked" : \
                self.buttonRemoveMIMEExtension_clicked,
                "on_btnAddHostName_clicked" : \
                self.buttonAddHostName_clicked,
                "on_btnRemoveHostName_clicked" : \
                self.buttonRemoveHostName_clicked,
                "on_btnAddHost_clicked" : \
                self.btnAddHost_clicked,
                "on_btnAddIP_clicked" : \
                self.btnAddIP_clicked,
                "on_btnRemoveHost_clicked" : \
                self.btnRemoveHost_clicked,
                "on_btnRemoveIP_clicked" : \
                self.btnRemoveIP_clicked
                }
        # Connect signals
        self.wTree.signal_autoconnect (dic)

    def insert_row(self, model,parent,
                   column):
        """Fuction used to add rows to treeview.
        @param model  what model tree view uses for storing data
        @param parent used to nest entries (like dir explorer)
        @param column the text you want to insert
        """
        myiter=model.insert_after(parent,None)
        model.set_value(myiter,0,column)
        return myiter

    def delete_rows(self, treeV):
        """ Function used to remove rows from treeview.
        @param treeV from which treeview we're deleting
        """
        selection = treeV.get_selection()
        model, selected = selection.get_selected()
        model.remove(selected)

    def btnAddHost_clicked(self, button):
        """ Add new host name to treeview """
        self.insert_row(self.treemodelHost,None, self.ShowDialogBox("Add new host name"))

    def btnRemoveHost_clicked(self, button):
        """ Removes selected file name """
        self.delete_rows(self.treeviewHost)

    def btnAddIP_clicked(self, button):
        """ Add new IP address to trreview """
        self.insert_row(self.treemodelIP,None, self.ShowDialogBox("Add new IP address"))

    def btnRemoveIP_clicked(self, button):
        """ Removes selected file name """
        self.delete_rows(self.treeviewIP)

    def buttonAddMIMEType_clicked(self, button):
        """ Add new MIME type to list """
        self.store.append([self.ShowDialogBox("Add new MIME type")])

    def buttonAddHostName_clicked(self, button):
        """ Add new Host name to combobox """
        self.store2.append([self.ShowDialogBox("Add new host")])

    def buttonRemoveHostName_clicked(self, button):
        """ Removes host name from combobox """
        self.store2.remove(self.cbHost.get_active_iter())

    def buttonAddefaultFileName_clicked(self, button):
        """ Adds new entry, with default file name to tree view"""
        self.insert_row(self.treemodel,None, self.ShowDialogBox("Add new file name"))

    def buttonRemoveDefultFileName_clicked(self, button):
        """ Removes selected file name """
        self.delete_rows(self.treeview)

    def buttonAddMIMEExtension_clicked(self, button):
        """ Add new extension """
        self.insert_row(self.treemodelEXT,None, self.ShowDialogBox("Add new extension"))

    def buttonRemoveMIMEExtension_clicked(self, widget):
        """ Remove selected extension name """
        self.delete_rows(self.treeviewEXT)

    def filechooserbutton2_file_set(self, widget):
        """ Place file path in entry field """
        self.wTree.get_widget("enManager").set_text(self.wTree.get_widget("filechooserbutton2").get_filename())

    def filechooserbutton4_file_set(self, widget):
        """ Place file path in entry field """
        self.wTree.get_widget("enSystemFolder").set_text(self.wTree.get_widget("filechooserbutton4").get_filename())

    def filechooserbutton3_file_set(self, widget):
        """ Place file path in entry field """
        self.wTree.get_widget("enDocummentRoot").set_text(self.wTree.get_widget("filechooserbutton3").get_filename())

    def filechooserbutton1_file_set(self, widget):
        """ Place file path in entry field """
        self.wTree.get_widget("enStylesheet").set_text(self.wTree.get_widget("filechooserbutton1").get_filename())

    def ShowDialogBox(self, title):
        """ Shows a typical dilog box with single text box entry field """
        dia = gtk.Dialog(title)
        dia.show()
        entry = gtk.Entry()
        entry.show()
        entry.set_activates_default(True)
        dia.vbox.pack_start(entry)
        dia.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
        dia.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)
        dia.set_default_response(gtk.RESPONSE_OK)
        response = dia.run()
        if response == gtk.RESPONSE_OK:
            name = entry.get_text()
        dia.destroy()
        return name

    def run(self):
        gtk.main()

    # Here begins the functionality implementation
    def get(self, name):
        """ Helper/alias function returns widgetwith specified name """
        return self.wTree.get_widget(name)

    def serialize_config(self):
        """ This function serializes the configurtion settings
        and returns a dictionary with pairs:
        name_of_setting: vlue """
        settings = { 'LANGUGE' : self.get("combobox2").get_active_text().partition(".")[0].capitalize(), \
                     'VERBOSITY' :  self.get("cbVerbosityLevel").get_active_text() }
        return settings


def main():
    app = ConfigGUIGTK()
    app.run()

if __name__ == "__main__":
    main()

