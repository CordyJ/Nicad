package org.eclipse.jdt.internal.core.search.indexing;
/*
 * (c) Copyright IBM Corp. 2000, 2001.
 * All Rights Reserved.
 */
import org.eclipse.core.resources.*;
import org.eclipse.jdt.internal.core.index.*;
import org.eclipse.jdt.internal.core.search.processing.*;
import org.eclipse.jdt.internal.core.index.impl.*;
import java.io.*;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.IProgressMonitor;
class AddClassFileToIndex extends IndexRequest {
       IFile resource;
       IndexManager manager;
       IPath indexedContainer;
       byte[] contents;
       public AddClassFileToIndex(
             IFile resource,
             IndexManager manager,
             IPath indexedContainer) {
             this.resource = resource;
             this.manager = manager;
             this.indexedContainer = indexedContainer; }
       public boolean belongsTo(String jobFamily) {
             return jobFamily.equals(this.indexedContainer.segment(0)); }
       public boolean execute(IProgressMonitor progressMonitor) {
             if (progressMonitor != null && progressMonitor.isCanceled()) return COMPLETE;
             try {
                  IIndex index = manager.getIndex(this.indexedContainer);
                  if (!resource.isLocal(IResource.DEPTH_ZERO)) {
                      return FAILED; }
                  /* ensure no concurrent write access to index */
                  if (index == null)
                      return COMPLETE;
                  ReadWriteMonitor monitor = manager.getMonitorFor(index);
                  if (monitor == null)
                      return COMPLETE; // index got deleted since acquired
                  try {
                      monitor.enterWrite(); // ask permission to write
                      byte[] contents = this.getContents();
                      if (contents == null)
                         return FAILED;
                      index.add(new IFileDocument(resource, contents), new BinaryIndexer(true));
                  } finally {
                      monitor.exitWrite();  }// free write lock
             } catch (IOException e) {
                  return FAILED; }
             return COMPLETE; }
       private byte[] getContents() {
             if (this.contents == null)
                  this.initializeContents();
             return this.contents; }
       public void initializeContents() {
             if (!resource.isLocal(IResource.DEPTH_ZERO)) {
                  return;
             } else {
                  try {
                      IPath location = resource.getLocation();
                      if (location != null) {
                         ;
                         this.contents =
                           org.eclipse.jdt.internal.compiler.util.Util.getFileByteContent(
                            resource.getLocation().toFile()); }
                  } catch (IOException e) { } } }
       public String toString() {
             return "indexing " + resource.getFullPath();  } }//$NON-NLS-1$
