package org.eclipse.jdt.internal.codeassist.complete;
import org.eclipse.jdt.internal.compiler.ast.Argument;
import org.eclipse.jdt.internal.compiler.ast.TypeReference;
import org.eclipse.jdt.internal.compiler.lookup.BlockScope;
import org.eclipse.jdt.internal.compiler.lookup.MethodScope;
import org.eclipse.jdt.internal.compiler.lookup.TypeBinding;
import org.eclipse.jdt.internal.compiler.util.CharOperation;
public class CompletionOnArgumentName extends Argument {
       private static final char[] FAKENAMESUFFIX = " ".toCharArray(); //$NON-NLS-1$
       public char[] realName;
       public CompletionOnArgumentName(char[] name , long posNom , TypeReference tr , int modifiers){
             super(CharOperation.concat(name, FAKENAMESUFFIX), posNom, tr, modifiers);
             this.realName = name; }
       public void resolve(BlockScope scope) {
             super.resolve(scope);
             throw new CompletionNodeFound(this, scope); }
       public void bind(MethodScope scope, TypeBinding typeBinding, boolean used) {
             super.bind(scope, typeBinding, used);
             throw new CompletionNodeFound(this, scope); }
       public String toString(int tab) {
             String s = tabString(tab);
             s += "<CompleteOnArgumentName:"; //$NON-NLS-1$
             if (type != null) s += type.toString() + " "; //$NON-NLS-1$
             s += new String(realName);
             if (initialization != null) s += " = " + initialization.toStringExpression(); //$NON-NLS-1$
             s += ">"; //$NON-NLS-1$
             return s; } }
