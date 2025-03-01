package org.eclipse.jdt.core.search;
/*
 * (c) Copyright IBM Corp. 2000, 2001.
 * All Rights Reserved.
 */
import org.eclipse.jdt.internal.core.search.processing.*;
/**
 * <p>
 * This interface defines the constants used by the search engine.
 * </p>
 * <p>
 * This interface declares constants only; it is not intended to be implemented.
 * </p>
 * @see org.eclipse.jdt.core.search.SearchEngine
 */
public interface IJavaSearchConstants {
       /**
        * The nature of searched element or the nature
        * of match in unknown.
        */
       int UNKNOWN = -1;
       /* Nature of searched element */
       /**
        * The searched element is a type.
        */
       int TYPE= 0;
       /**
        * The searched element is a method.
        */
       int METHOD= 1;
       /**
        * The searched element is a package.
        */
       int PACKAGE= 2;
       /**
        * The searched element is a constructor.
        */
       int CONSTRUCTOR= 3;
       /**
        * The searched element is a field.
        */
       int FIELD= 4;
       /**
        * The searched element is a class. 
        * More selective than using TYPE
        */
       int CLASS= 5;
       /**
        * The searched element is an interface.
        * More selective than using TYPE
        */
       int INTERFACE= 6;
       /* Nature of match */
       /**
        * The search result is a declaration.
        * Can be used in conjunction with any of the nature of searched elements
        * so as to better narrow down the search.
        */
       int DECLARATIONS= 0;
       /**
        * The search result is a type that implements an interface. 
        * Used in conjunction with either TYPE or CLASS or INTERFACE, it will
        * respectively search for any type implementing/extending an interface, or
        * rather exclusively search for classes implementing an interface, or interfaces 
        * extending an interface.
        */
       int IMPLEMENTORS= 1;
       /**
        * The search result is a reference.
        * Can be used in conjunction with any of the nature of searched elements
        * so as to better narrow down the search.
        * References can contain implementors since they are more generic kind
        * of matches.
        */
       int REFERENCES= 2;
       /**
        * The search result is a declaration, a reference, or an implementor 
        * of an interface.
        * Can be used in conjunction with any of the nature of searched elements
        * so as to better narrow down the search.
        */
       int ALL_OCCURRENCES= 3;
       /**
        * When searching for field matches, it will exclusively find read accesses, as
        * opposed to write accesses. Note that some expressions are considered both
        * as field read/write accesses: e.g. x++; x+= 1;
        * 
        * @since 2.0
        */
       int READ_ACCESSES = 4;
       /**
        * When searching for field matches, it will exclusively find write accesses, as
        * opposed to read accesses. Note that some expressions are considered both
        * as field read/write accesses: e.g. x++; x+= 1;
        * 
        * @since 2.0
        */
       int WRITE_ACCESSES = 5;
       /** 
        * @deprecated - use READ_ACCESSES instead (will be discarded before 2.0)
        * @since 2.0
        */
       int READ_REFERENCES = READ_ACCESSES;
       /** 
        * @deprecated - use WRITE_ACCESSES instead (will be discarded before 2.0)
        * @since 2.0
        */
       int WRITE_REFERENCES = WRITE_ACCESSES;
       /* Syntactic match modes */
       /**
        * The search pattern matches exactly the search result,
        * i.e. the source of the search result equals the search pattern.
        */
       int EXACT_MATCH = 0;
       /**
        * The search pattern is a prefix of the search result.
        */
       int PREFIX_MATCH = 1;
       /**
        * The search pattern contains one or more wild-cards ('*') where a 
        * wild-card can replace 0 or more characters in the search result.
        */
       int PATTERN_MATCH = 2;
       /* Case sensitivity */
       /**
        * The search pattern matches the search result only
        * if case are the same.
        */
       boolean CASE_SENSITIVE = true;
       /**
        * The search pattern ignores cases in the search result.
        */
       boolean CASE_INSENSITIVE = false;
       /* Waiting policies */
       /**
        * The search operation starts immediately, even if the underlying indexer
        * has not finished indexing the workspace. Results will more likely
        * not contain all the matches.
        */
       int FORCE_IMMEDIATE_SEARCH = IJob.ForceImmediate;
       /**
        * The search operation throws an <code>org.eclipse.core.runtime.OperationCanceledException</code>
        * if the underlying indexer has not finished indexing the workspace.
        */
       int CANCEL_IF_NOT_READY_TO_SEARCH = IJob.CancelIfNotReady;
       /**
        * The search operation waits for the underlying indexer to finish indexing 
        * the workspace before starting the search.
        */
       int WAIT_UNTIL_READY_TO_SEARCH = IJob.WaitUntilReady; }
