package org.eclipse.jdt.internal.core.jdom;
/*
 * (c) Copyright IBM Corp. 2000, 2001.
 * All Rights Reserved.
 */
import org.eclipse.jdt.internal.compiler.*;
import org.eclipse.jdt.internal.compiler.env.ICompilationUnit;
import org.eclipse.jdt.core.jdom.*;
import org.eclipse.jdt.internal.core.util.ReferenceInfoAdapter;
import java.util.Stack;
/**
 * An abstract DOM builder that contains shared functionality of DOMBuilder and SimpleDOMBuilder.
 */
public class AbstractDOMBuilder extends ReferenceInfoAdapter implements ILineStartFinder {
       /**
        * Set to true when an error is encounterd while
        * fuzzy parsing
        */
       protected boolean fAbort;
       /**
        * True when a compilation unit is being constructed.
        * False when any other type of document fragment is
        * being constructed.
        */
       protected boolean fBuildingCU = false;
       /**
        * True when a compilation unit or type is being
        * constructed. False when any other type of document
        * fragment is being constructed.
        */
       protected boolean fBuildingType= false;
       /**
        * The String on which the JDOM is being created.
        */
       protected char[] fDocument= null;
       /**
        * The source positions of all of the line separators in the document.
        */
       protected int[] fLineStartPositions = new int[] { 0 };
       /**
        * A stack of enclosing scopes used when constructing
        * a compilation unit or type. The top of the stack
        * is the document fragment that children are added to.
        */
       protected Stack fStack = null;        
       /**
        * The number of fields constructed in the current
        * document. This is used when building a single
        * field document fragment, since the DOMBuilder only
        * accepts documents with one field declaration.
        */
       protected int fFieldCount;
       /**
        * The current node being constructed.
        */
       protected DOMNode fNode;
/**
 * AbstractDOMBuilder constructor.
 */
public AbstractDOMBuilder() {
       super(); }
/**
 * Accepts the line separator table and converts it into a line start table.
 *
 * <p>A line separator might corresponds to several characters in the source.
 *
 * @see IDocumentElementRequestor#acceptLineSeparatorPositions
 */
public void acceptLineSeparatorPositions(int[] positions) {
       if (positions != null) {
             int length = positions.length;
             if (length > 0) {
                  fLineStartPositions = new int[length + 1];
                  fLineStartPositions[0] = 0;
                  int documentLength = fDocument.length;
                  for (int i = 0; i < length; i++) {
                      int iPlusOne = i + 1;
                      int positionPlusOne = positions[i] + 1;    
                      if (positionPlusOne < documentLength) {
                         if (iPlusOne < length) {
                           // more separators
                           fLineStartPositions[iPlusOne] = positionPlusOne;
                         } else {
                           // no more separators
                           if (fDocument[positionPlusOne] == '\n') {
                            fLineStartPositions[iPlusOne] = positionPlusOne + 1;
                           } else {
                            fLineStartPositions[iPlusOne] = positionPlusOne; } }
                      } else {
                         fLineStartPositions[iPlusOne] = positionPlusOne; } } } } }
/**
 * Does nothing.
 */
public void acceptProblem(IProblem problem) {}
/**
 * Adds the given node to the current enclosing scope, building the JDOM
 * tree. Nodes are only added to an enclosing scope when a compilation unit or type
 * is being built (since those are the only nodes that have children).
 *
 * <p>NOTE: nodes are added to the JDOM via the method #basicAddChild such that
 * the nodes in the newly created JDOM are not fragmented. 
 */
protected void addChild(IDOMNode child) {
       if (fStack.size() > 0) {
             DOMNode parent = (DOMNode) fStack.peek();
             if (fBuildingCU || fBuildingType) {
                  parent.basicAddChild(child); } } }
/**
 * @see IDOMFactory#createCompilationUnit(String)
 */
public IDOMCompilationUnit createCompilationUnit(char[] contents, char[] name) {
       return createCompilationUnit(new CompilationUnit(contents, name)); }
/**
 * @see IDOMFactory#createCompilationUnit(String)
 */
public IDOMCompilationUnit createCompilationUnit(ICompilationUnit compilationUnit) {
       if (fAbort) {
             return null; }
       fNode.normalize(this);
       return (IDOMCompilationUnit)fNode; }
/**
 * @see IDocumentElementRequestor#enterClass
 */
public void enterCompilationUnit() {
       if (fBuildingCU) {
             IDOMCompilationUnit cu= new DOMCompilationUnit(fDocument, new int[] {0, fDocument.length - 1});
            fStack.push(cu); } }
/**
 * Finishes the configuration of the compilation unit DOM object which
 * was created by a previous enterCompilationUnit call.
 *
 * @see IDocumentElementRequestor#exitCompilationUnit
 */
public void exitCompilationUnit(int declarationEnd) {
       DOMCompilationUnit cu = (DOMCompilationUnit) fStack.pop();
       cu.setSourceRangeEnd(declarationEnd);
       fNode = cu; }
/**
 * Finishes the configuration of the class and interface DOM objects.
 *
 * @param bodyEnd - a source position corresponding to the closing bracket of the class
 * @param declarationEnd - a source position corresponding to the end of the class
 *         declaration.  This can include whitespace and comments following the closing bracket.
 */
protected void exitType(int bodyEnd, int declarationEnd) {
       DOMType type = (DOMType)fStack.pop();
       type.setSourceRangeEnd(declarationEnd);
       type.setCloseBodyRangeStart(bodyEnd);
       type.setCloseBodyRangeEnd(bodyEnd);
       fNode = type; }
/**
 * @see ILineSeparatorFinder
 */
public int getLineStart(int position) {
       int lineSeparatorCount = fLineStartPositions.length;
       // reverse traversal intentional.
       for(int i = lineSeparatorCount - 1; i >= 0; i--) {
             if (fLineStartPositions[i] <= position)
                  return fLineStartPositions[i]; }
       return 0; }
/**
 * Initializes the builder to create a document fragment.
 *
 * @param sourceCode - the document containing the source code to be analyzed
 * @param buildingCompilationUnit - true if a the document is being analyzed to
 *         create a compilation unit, otherwise false
 * @param buildingType - true if the document is being analyzed to create a
 *         type or compilation unit
 */
protected void initializeBuild(char[] sourceCode, boolean buildingCompilationUnit, boolean buildingType) {
       fBuildingCU = buildingCompilationUnit;
       fBuildingType = buildingType;
       fStack = new Stack();
       fDocument = sourceCode;
       fFieldCount = 0;
       fAbort = false; } }
