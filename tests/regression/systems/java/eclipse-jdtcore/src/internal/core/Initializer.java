package org.eclipse.jdt.internal.core;
/*
 * (c) Copyright IBM Corp. 2000, 2001.
 * All Rights Reserved.
 */
import org.eclipse.core.resources.*;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jdt.core.*;
import org.eclipse.jdt.core.jdom.IDOMInitializer;
import org.eclipse.jdt.core.jdom.IDOMNode;
/**
 * @see IInitializer
 */
/* package */ class Initializer extends Member implements IInitializer {
protected Initializer(IType parent, int occurrenceCount) {
       super(INITIALIZER, parent, ""); //$NON-NLS-1$
       // 0 is not valid: this first occurrence is occurrence 1.
       if (occurrenceCount <= 0)
             throw new IllegalArgumentException();
       fOccurrenceCount = occurrenceCount; }
/**
 * @see JavaElement#equalsDOMNode
 */
protected boolean equalsDOMNode(IDOMNode node) throws JavaModelException {
       if (node.getNodeType() == IDOMNode.INITIALIZER) {
             return node.getContents().trim().equals(getSource());
       } else {
             return false; } }
/**
 * @see JavaElement#getHandleMemento()
 */
public String getHandleMemento(){
       StringBuffer buff= new StringBuffer(((JavaElement)getParent()).getHandleMemento());
       buff.append(getHandleMementoDelimiter());
       buff.append(fOccurrenceCount);
       return buff.toString(); }
/**
 * @see JavaElement#getHandleMemento()
 */
protected char getHandleMementoDelimiter() {
       return JavaElement.JEM_INITIALIZER; }
public int hashCode() {
       return Util.combineHashCodes(fParent.hashCode(), fOccurrenceCount); }
/**
 */
public String readableName() {
       return ((JavaElement)getDeclaringType()).readableName(); }
/**
 * @see ISourceManipulation
 */
public void rename(String name, boolean force, IProgressMonitor monitor) throws JavaModelException {
       throw new JavaModelException(new JavaModelStatus(IJavaModelStatusConstants.INVALID_ELEMENT_TYPES, this)); }
/**
 * @private Debugging purposes
 */
protected void toStringInfo(int tab, StringBuffer buffer, Object info) {
       buffer.append(this.tabString(tab));
       if (info == null) {
             buffer.append("<initializer>"); //$NON-NLS-1$
             buffer.append(" (not open)"); //$NON-NLS-1$
       } else if (info == NO_INFO) {
             buffer.append(getElementName());
       } else {
             try {
                  if (Flags.isStatic(this.getFlags())) {
                      buffer.append("static ");  }//$NON-NLS-1$
                  buffer.append("initializer"); //$NON-NLS-1$
             } catch (JavaModelException e) {
                  buffer.append("<JavaModelException in toString of " + getElementName());  } } } }//$NON-NLS-1$
