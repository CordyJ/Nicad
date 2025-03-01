package org.eclipse.jdt.internal.core.newbuilder;
/*
 * (c) Copyright IBM Corp. 2000, 2001.
 * All Rights Reserved.
 */
import org.eclipse.jdt.internal.compiler.env.ICompilationUnit;
import org.eclipse.jdt.internal.compiler.util.CharOperation;
import java.io.*;
public class SourceFile implements ICompilationUnit {
public char[] fileName;
public char[] mainTypeName;
public char[][] packageName;
public SourceFile(String fileName, char[] typeName) {
       this.fileName = fileName.toCharArray();
       CharOperation.replace(this.fileName, '\\', '/');
       int lastIndex = CharOperation.lastIndexOf('/', typeName);
       this.mainTypeName = CharOperation.subarray(typeName, lastIndex + 1, -1);
       this.packageName = CharOperation.splitOn('/', typeName, 0, lastIndex - 1); }
public SourceFile(String fileName, char[] mainTypeName, char[][] packageName) {
       this.fileName = fileName.toCharArray();
       CharOperation.replace(this.fileName, '\\', '/');
       this.mainTypeName = mainTypeName;
       this.packageName = packageName; }
public char[] getContents() {
       // otherwise retrieve it
       BufferedReader reader = null;
       try {
             File file = new File(new String(fileName));
             reader = new BufferedReader(new FileReader(file));
             int length = (int) file.length();
             char[] contents = new char[length];
             int len = 0;
             int readSize = 0;
             while ((readSize != -1) && (len != length)) {
                  // See PR 1FMS89U
                  // We record first the read size. In this case len is the actual read size.
                  len += readSize;
                  readSize = reader.read(contents, len, length - len); }
             reader.close();
             // See PR 1FMS89U
             // Now we need to resize in case the default encoding used more than one byte for each
             // character
             if (len != length)
                  System.arraycopy(contents, 0, (contents = new char[len]), 0, len);   
             return contents;
       } catch (FileNotFoundException e) {
       } catch (IOException e) {
             if (reader != null) {
                  try {
                      reader.close();
                  } catch(IOException ioe) { } }
       };
       return new char[0]; }
public char[] getFileName() {
       return fileName; }
public char[] getMainTypeName() {
       return mainTypeName; }
public char[][] getPackageName() {
       return packageName; }
public String toString() {
       return "SourceFile[" //$NON-NLS-1$
             + new String(fileName) + "]";   } }//$NON-NLS-1$
