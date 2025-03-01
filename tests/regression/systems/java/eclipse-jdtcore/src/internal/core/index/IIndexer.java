package org.eclipse.jdt.internal.core.index;
/*
 * (c) Copyright IBM Corp. 2000, 2001.
 * All Rights Reserved.
 */
import org.eclipse.jdt.core.*;
/**
 * An <code>IIndexer</code> indexes ONE document at each time. It adds the document names and
 * the words references to an IIndex. Each IIndexer can index certain types of document, and should
 * not index the other files. 
 */
public interface IIndexer {
       /**
        * Returns the file types the <code>IIndexer</code> handles.
        */
       String[] getFileTypes();
       /**
        * Indexes the given document, adding the document name and the word references 
        * to this document to the given <code>IIndex</code>.The caller should use 
        * <code>shouldIndex()</code> first to determine whether this indexer handles 
        * the given type of file, and only call this method if so. 
        */
       void index(IDocument document, IIndexerOutput output) throws java.io.IOException;
       /**
        * Sets the document types the <code>IIndexer</code> handles.
        */
       public void setFileTypes(String[] fileTypes);
       /**
        * Returns whether the <code>IIndexer</code> can index the given document or not.
        */
       public boolean shouldIndex(IDocument document); }
