# -*- coding: utf-8 -*-

# Copyright (c) 2006 - 2010 Detlev Offenbach <detlev@die-offenbachs.de>
#

"""
Module implementing the Help Viewers configuration page.
"""

from PyQt4.QtCore import QDir, QString, pyqtSignature
from PyQt4.QtGui import QButtonGroup

from KdeQt import KQFileDialog

from E4Gui.E4Completers import E4FileCompleter

from ConfigurationPageBase import ConfigurationPageBase
from Ui_HelpViewersPage import Ui_HelpViewersPage

import Preferences
import Utilities

class HelpViewersPage(ConfigurationPageBase, Ui_HelpViewersPage):
    """
    Class implementing the Help Viewers configuration page.
    """
    def __init__(self):
        """
        Constructor
        """
        ConfigurationPageBase.__init__(self)
        self.setupUi(self)
        self.setObjectName("HelpViewersPage")
        
        self.helpViewerGroup = QButtonGroup()
        self.helpViewerGroup.addButton(self.helpBrowserButton)
        self.helpViewerGroup.addButton(self.qtAssistantButton)
        self.helpViewerGroup.addButton(self.webBrowserButton)
        self.helpViewerGroup.addButton(self.customViewerButton)
        
        self.customViewerCompleter = E4FileCompleter(self.customViewerEdit)
        
        # set initial values
        hvId = Preferences.getHelp("HelpViewerType")
        if hvId == 1:
            self.helpBrowserButton.setChecked(True)
        elif hvId == 2:
            self.qtAssistantButton.setChecked(True)
        elif hvId == 3:
            self.webBrowserButton.setChecked(True)
        else:
            self.customViewerButton.setChecked(True)
        self.customViewerEdit.setText(\
            Preferences.getHelp("CustomViewer"))
        
    def save(self):
        """
        Public slot to save the Help Viewers configuration.
        """
        if self.helpBrowserButton.isChecked():
            hvId = 1
        elif self.qtAssistantButton.isChecked():
            hvId = 2
        elif self.webBrowserButton.isChecked():
            hvId = 3
        elif self.customViewerButton.isChecked():
            hvId = 4
        Preferences.setHelp("HelpViewerType", hvId)
        Preferences.setHelp("CustomViewer",
            self.customViewerEdit.text())
        
    @pyqtSignature("")
    def on_customViewerSelectionButton_clicked(self):
        """
        Private slot to handle the custom viewer selection.
        """
        file = KQFileDialog.getOpenFileName(\
            self,
            self.trUtf8("Select Custom Viewer"),
            self.customViewerEdit.text(),
            QString())
        
        if not file.isNull():
            self.customViewerEdit.setText(Utilities.toNativeSeparators(file))
        
    @pyqtSignature("")
    def on_webbrowserButton_clicked(self):
        """
        Private slot to handle the Web browser selection.
        """
        file = KQFileDialog.getOpenFileName(\
            self,
            self.trUtf8("Select Web-Browser"),
            self.webbrowserEdit.text(),
            QString())
        
        if not file.isNull():
            self.webbrowserEdit.setText(Utilities.toNativeSeparators(file))
        
    @pyqtSignature("")
    def on_pdfviewerButton_clicked(self):
        """
        Private slot to handle the PDF viewer selection.
        """
        file = KQFileDialog.getOpenFileName(\
            self,
            self.trUtf8("Select PDF-Viewer"),
            self.pdfviewerEdit.text(),
            QString())
        
        if not file.isNull():
            self.pdfviewerEdit.setText(Utilities.toNativeSeparators(file))
        
    @pyqtSignature("")
    def on_chmviewerButton_clicked(self):
        """
        Private slot to handle the CHM viewer selection.
        """
        file = KQFileDialog.getOpenFileName(\
            self,
            self.trUtf8("Select CHM-Viewer"),
            self.chmviewerEdit.text(),
            QString())
        
        if not file.isNull():
            self.chmviewerEdit.setText(Utilities.toNativeSeparators(file))
    
def create(dlg):
    """
    Module function to create the configuration page.
    
    @param dlg reference to the configuration dialog
    """
    page = HelpViewersPage()
    return page
