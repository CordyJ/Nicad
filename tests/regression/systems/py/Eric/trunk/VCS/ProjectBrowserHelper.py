# -*- coding: utf-8 -*-

# Copyright (c) 2005 - 2010 Detlev Offenbach <detlev@die-offenbachs.de>
#

"""
Module implementing the base class of the VCS project browser helper.
"""

import os

from PyQt4.QtCore import QObject
from PyQt4.QtGui import QDialog

from KdeQt.KQApplication import e4App

from UI.DeleteFilesConfirmationDialog import DeleteFilesConfirmationDialog

from Project.ProjectBrowserModel import ProjectBrowserSimpleDirectoryItem, \
    ProjectBrowserFileItem, ProjectBrowserDirectoryItem

from RepositoryInfoDialog import VcsRepositoryInfoDialog

import Preferences

class VcsProjectBrowserHelper(QObject):
    """
    Class implementing the base class of the VCS project browser helper.
    """
    def __init__(self, vcsObject, browserObject, projectObject, isTranslationsBrowser,
        parent = None, name = None):
        """
        Constructor
        
        @param vcsObject reference to the vcs object
        @param browserObject reference to the project browser object
        @param projectObject reference to the project object
        @param isTranslationsBrowser flag indicating, the helper is requested for the
            translations browser (this needs some special treatment)
        @param parent parent widget (QWidget)
        @param name name of this object (string or QString)
        """
        QObject.__init__(self, parent)
        if name:
            self.setObjectName(name)
        
        self.vcs = vcsObject
        self.browser = browserObject
        self.isTranslationsBrowser = isTranslationsBrowser
        self.project = projectObject
    
    def addVCSMenus(self, mainMenu, multiMenu, backMenu, dirMenu, dirMultiMenu):
        """
        Public method to add the VCS entries to the various project browser menus.
        
        @param mainMenu reference to the main menu (QPopupMenu)
        @param multiMenu reference to the multiple selection menu (QPopupMenu)
        @param backMenu reference to the background menu (QPopupMenu)
        @param dirMenu reference to the directory menu (QPopupMenu)
        @param dirMultiMenu reference to the multiple selection directory
            menu (QPopupMenu)
        """
        self._addVCSMenu(mainMenu)
        self._addVCSMenuMulti(multiMenu)
        self._addVCSMenuBack(backMenu)
        self._addVCSMenuDir(dirMenu)
        self._addVCSMenuDirMulti(dirMultiMenu)
    
    def showContextMenu(self, menu, standardItems):
        """
        Slot called before the context menu is shown. 
        
        It enables/disables the VCS menu entries depending on the overall 
        VCS status and the file status.
        
        @param menu reference to the menu to be shown
        @param standardItems array of standard items that need activation/deactivation
            depending on the overall VCS status
        """
        raise RuntimeError('Not implemented')
    
    def showContextMenuMulti(self, menu, standardItems):
        """
        Slot called before the context menu (multiple selections) is shown. 
        
        It enables/disables the VCS menu entries depending on the overall 
        VCS status and the files status.
        
        @param menu reference to the menu to be shown
        @param standardItems array of standard items that need activation/deactivation
            depending on the overall VCS status
        """
        raise RuntimeError('Not implemented')
    
    def showContextMenuDir(self, menu, standardItems):
        """
        Slot called before the context menu is shown. 
        
        It enables/disables the VCS menu entries depending on the overall 
        VCS status and the directory status.
        
        @param menu reference to the menu to be shown
        @param standardItems array of standard items that need activation/deactivation
            depending on the overall VCS status
        """
        raise RuntimeError('Not implemented')
    
    def showContextMenuDirMulti(self, menu, standardItems):
        """
        Slot called before the context menu is shown. 
        
        It enables/disables the VCS menu entries depending on the overall 
        VCS status and the directory status.
        
        @param menu reference to the menu to be shown
        @param standardItems array of standard items that need activation/deactivation
            depending on the overall VCS status
        """
        raise RuntimeError('Not implemented')

    ############################################################################
    ## General menu handling methods below
    ############################################################################
    
    def _VCSUpdate(self):
        """
        Protected slot called by the context menu to update a file from the
        VCS repository.
        """
        if self.isTranslationsBrowser:
            names = [unicode(itm.dirName()) \
                for itm in self.browser.getSelectedItems(\
                    [ProjectBrowserSimpleDirectoryItem])]
            if not names:
                names = [unicode(itm.fileName()) \
                    for itm in self.browser.getSelectedItems([ProjectBrowserFileItem])]
        else:
            names = []
            for itm in self.browser.getSelectedItems():
                try:
                    name = unicode(itm.fileName())
                except AttributeError:
                    name = unicode(itm.dirName())
                names.append(name)
        self.vcs.vcsUpdate(names)
        
    def _VCSCommit(self):
        """
        Protected slot called by the context menu to commit the changes to the
        VCS repository.
        """
        if self.isTranslationsBrowser:
            names = [unicode(itm.dirName()) \
                for itm in self.browser.getSelectedItems(\
                    [ProjectBrowserSimpleDirectoryItem])]
            if not names:
                names = [unicode(itm.fileName()) \
                    for itm in self.browser.getSelectedItems([ProjectBrowserFileItem])]
        else:
            names = []
            for itm in self.browser.getSelectedItems():
                try:
                    name = unicode(itm.fileName())
                except AttributeError:
                    name = unicode(itm.dirName())
                names.append(name)
        if Preferences.getVCS("AutoSaveFiles"):
            vm = e4App().getObject("ViewManager")
            for name in names:
                vm.saveEditor(name)
        self.vcs.vcsCommit(names, '')
        
    def _VCSAdd(self):
        """
        Protected slot called by the context menu to add the selected file to the
        VCS repository.
        """
        if self.isTranslationsBrowser:
            items = self.browser.getSelectedItems([ProjectBrowserSimpleDirectoryItem])
            if items:
                names = [unicode(itm.dirName()) for itm in items]
                qnames = []
            else:
                items = self.browser.getSelectedItems([ProjectBrowserFileItem])
                names = []
                qnames = []
                for itm in items:
                    name = unicode(itm.fileName())
                    if name.endswith('.qm'):
                        qnames.append(name)
                    else:
                        names.append(name)
        else:
            names = []
            for itm in self.browser.getSelectedItems():
                try:
                    name = unicode(itm.fileName())
                except AttributeError:
                    name = unicode(itm.dirName())
                names.append(name)
            qnames = []
        
        if not len(names + qnames):
            return
        
        if len(names + qnames) == 1:
            if names:
                self.vcs.vcsAdd(names[0], os.path.isdir(names[0]))
            else:
                if self.vcs.canDetectBinaries:
                    self.vcs.vcsAdd(qnames)
                else:
                    self.vcs.vcsAddBinary(qnames)
        else:
            if self.vcs.canDetectBinaries:
                self.vcs.vcsAdd(names + qnames)
            else:
                self.vcs.vcsAdd(names)
                if len(qnames):
                    self.vcs.vcsAddBinary(qnames)
        for fn in names + qnames:
            self._updateVCSStatus(fn)
        
    def _VCSAddTree(self):
        """
        Protected slot called by the context menu.
        
        It is used to add the selected
        directory tree to the VCS repository.
        """
        items = self.browser.getSelectedItems()
        names = []
        for itm in self.browser.getSelectedItems():
            try:
                name = unicode(itm.fileName())
            except AttributeError:
                name = unicode(itm.dirName())
            names.append(name)
        self.vcs.vcsAddTree(names)
        for fn in names:
            self._updateVCSStatus(fn)
        
    def _VCSRemove(self):
        """
        Protected slot called by the context menu to remove the selected file from the
        VCS repository.
        """
        if self.isTranslationsBrowser:
            items = self.browser.getSelectedItems([ProjectBrowserSimpleDirectoryItem])
            if items:
                return      # not supported
            
            isRemoveDirs = False
            items = self.browser.getSelectedItems([ProjectBrowserFileItem])
            names = [unicode(itm.fileName()) for itm in items]
            
            dlg = DeleteFilesConfirmationDialog(self.parent(),
                self.trUtf8("Remove from repository (and disk)"),
                self.trUtf8("Do you really want to remove these translation files from"
                    " the repository (and disk)?"),
                names)
        else:
            items = self.browser.getSelectedItems()
            isRemoveDirs = len(items) == \
                self.browser.getSelectedItemsCount(\
                [ProjectBrowserSimpleDirectoryItem, ProjectBrowserDirectoryItem])
            if isRemoveDirs:
                names = [unicode(itm.dirName()) for itm in items]
            else:
                names = [unicode(itm.fileName()) for itm in items]
            files = [\
                name.replace(self.browser.project.ppath+os.sep, '') \
                for name in names]
            
            dlg = DeleteFilesConfirmationDialog(self.parent(),
                self.trUtf8("Remove from repository (and disk)"),
                self.trUtf8("Do you really want to remove these files/directories"
                    " from the repository (and disk)?"),
                files)
        
        if dlg.exec_() == QDialog.Accepted:
            status = self.vcs.vcsRemove(names)
            if status:
                if isRemoveDirs:
                    self.browser._removeDir() # remove directories from Project
                else:
                    self.browser._removeFile() # remove file(s) from project
        
    def _VCSLog(self):
        """
        Protected slot called by the context menu to show the VCS log of a file/directory.
        """
        itm = self.browser.currentItem()
        try:
            fn = unicode(itm.fileName())
        except AttributeError:
            fn = unicode(itm.dirName())
        self.vcs.vcsLog(fn)
        
    def _VCSDiff(self):
        """
        Protected slot called by the context menu to show the difference of a
        file/directory to the repository.
        """
        names = []
        for itm in self.browser.getSelectedItems():
            try:
                name = unicode(itm.fileName())
            except AttributeError:
                name = unicode(itm.dirName())
            names.append(name)
        self.vcs.vcsDiff(names)
        
    def _VCSStatus(self):
        """
        Protected slot called by the context menu to show the status of a file.
        """
        if self.isTranslationsBrowser:
            items = self.browser.getSelectedItems([ProjectBrowserSimpleDirectoryItem])
            if items:
                names = [unicode(itm.dirName()) for itm in items]
            else:
                items = self.browser.getSelectedItems([ProjectBrowserFileItem])
                names = [unicode(itm.fileName()) for itm in items]
        else:
            names = []
            for itm in self.browser.getSelectedItems():
                try:
                    name = unicode(itm.fileName())
                except AttributeError:
                    name = unicode(itm.dirName())
                names.append(name)
        self.vcs.vcsStatus(names)

    def _VCSRevert(self):
        """
        Protected slot called by the context menu to revert changes made to a file.
        """
        names = []
        for itm in self.browser.getSelectedItems():
            try:
                name = unicode(itm.fileName())
            except AttributeError:
                name = unicode(itm.dirName())
            names.append(name)
        self.vcs.vcsRevert(names)

    def _VCSMerge(self):
        """
        Protected slot called by the context menu to merge changes into to a file.
        """
        itm = self.browser.currentItem()
        try:
            name = unicode(itm.fileName())
        except AttributeError:
            name = unicode(itm.dirName())
        self.vcs.vcsMerge(name)
    
    def _VCSInfoDisplay(self):
        """
        Protected slot called to show some vcs information.
        """
        info = self.vcs.vcsRepositoryInfos(self.project.ppath)
        dlg = VcsRepositoryInfoDialog(None, info)
        dlg.exec_()

    def _updateVCSStatus(self, name):
        """
        Protected method to update the VCS status of an item.
        
        @param name filename or directoryname of the item to be updated (string)
        """
        self.project.getModel().updateVCSStatus(name)
