package org.eclipse.jdt.internal.core;
/*
 * (c) Copyright IBM Corp. 2000, 2001.
 * All Rights Reserved.
 */
import org.eclipse.jdt.internal.codeassist.ISearchRequestor;
import org.eclipse.jdt.internal.compiler.env.ICompilationUnit;
import org.eclipse.jdt.core.IInitializer;
import org.eclipse.jdt.core.IPackageFragment;
import org.eclipse.jdt.core.IType;
import org.eclipse.jdt.core.JavaModelException;
/**
 * Implements <code>IJavaElementRequestor</code>, wrappering and forwarding
 * results onto a <code>org.eclipse.jdt.internal.codeassist.api.ISearchRequestor</code>.
 */
class SearchableEnvironmentRequestor extends JavaElementRequestor implements IJavaElementRequestor {
       /**
        * The <code>ISearchRequestor</code> this JavaElementRequestor wraps
        * and forwards results to.
        */
       protected ISearchRequestor fRequestor;
       /**
        * The <code>ICompilationUNit</code> this JavaElementRequestor will not
        * accept types within.
        */
       protected ICompilationUnit fUnitToSkip;
/**
 * Constructs a SearchableEnvironmentRequestor that wraps the
 * given SearchRequestor.
 */
public SearchableEnvironmentRequestor(ISearchRequestor requestor) {
       fRequestor = requestor;
       fUnitToSkip= null; }
/**
 * Constructs a SearchableEnvironmentRequestor that wraps the
 * given SearchRequestor.  The requestor will not accept types in
 * the <code>unitToSkip</code>.
 */
public SearchableEnvironmentRequestor(ISearchRequestor requestor, ICompilationUnit unitToSkip) {
       fRequestor = requestor;
       fUnitToSkip= unitToSkip; }
/**
 * Do nothing, a SearchRequestor does not accept initializers
 * so there is no need to forward this results.
 *
 * @see IJavaElementRequestor
 */
public void acceptInitializer(IInitializer initializer) { }
/**
 * @see IJavaElementRequestor
 */
public void acceptPackageFragment(IPackageFragment packageFragment) {
       fRequestor.acceptPackage(packageFragment.getElementName().toCharArray()); }
/**
 * @see IJavaElementRequestor
 */
public void acceptType(IType type) {
       try {
             if (fUnitToSkip != null && fUnitToSkip.equals(type.getCompilationUnit())){
                  return; }
             if (type.isClass()) {
                  fRequestor.acceptClass(type.getPackageFragment().getElementName().toCharArray(), type.getElementName().toCharArray(), type.getFlags());
             } else {
                  fRequestor.acceptInterface(type.getPackageFragment().getElementName().toCharArray(), type.getElementName().toCharArray(), type.getFlags()); }
       } catch (JavaModelException npe) { } } }
