package org.eclipse.jdt.internal.compiler.batch;
/*
 * (c) Copyright IBM Corp. 2000, 2001.
 * All Rights Reserved.
 */
import java.io.*;
import java.util.*;
import java.util.zip.*;
import org.eclipse.jdt.internal.compiler.env.*;
import org.eclipse.jdt.internal.compiler.classfmt.*;
import org.eclipse.jdt.internal.compiler.util.Util;
class ClasspathJar implements FileSystem.Classpath {
       ZipFile zipFile;
       Hashtable directoryCache;
       ClasspathJar(File file) throws IOException {
             zipFile = new ZipFile(file);
             buildDirectoryStructure(); }
       void buildDirectoryStructure() {
             directoryCache = new Hashtable(101);
             for (Enumeration e = zipFile.entries(); e.hasMoreElements();) {
                  String fileName = ((ZipEntry) e.nextElement()).getName();
                  // extract the package name
                  int last = fileName.lastIndexOf('/');
                  if (last > 0 && directoryCache.get(fileName.substring(0, last)) == null) {
                      // add the package name & all of its parent packages
                      for (int i = 0; i <= last; i++) {
                         i = fileName.indexOf('/', i);
                         String packageName = fileName.substring(0, i);
                         if (directoryCache.get(packageName) == null)
                           directoryCache.put(packageName, packageName); } } } }
       public boolean exists(String filename, char[][] packageName) {
             return zipFile.getEntry(FileSystem.assembleName(filename, packageName, '/'))
                  != null; }
       public boolean isPackage(char[][] compoundName, char[] packageName) {
             return directoryCache.get(
                  FileSystem.assembleName(packageName, compoundName, '/'))
                  != null; }
       public long lastModified(String filename, char[][] packageName) {
             ZipEntry entry =
                  zipFile.getEntry(FileSystem.assembleName(filename, packageName, '/'));
             if (entry == null)
                  return -1L;
             else
                  return entry.getTime(); }
       public NameEnvironmentAnswer readClassFile(
             String filename,
             char[][] packageName) {
             try {
                  return new NameEnvironmentAnswer(
                      ClassFileReader.read(
                         zipFile,
                         FileSystem.assembleName(filename, packageName, '/')));
             } catch (Exception e) {
                  return null;  } }// treat as if class file is missing
       public NameEnvironmentAnswer readJavaFile(
             String filename,
             char[][] packageName) {
             InputStream stream = null;
             try {
                  String fullName = FileSystem.assembleName(filename, packageName, '/');
                  ZipEntry entry = zipFile.getEntry(fullName);
                  stream = new BufferedInputStream(zipFile.getInputStream(entry));
                  char[] contents = Util.getInputStreamAsCharArray(stream, (int) entry.getSize(), null);
                  return new NameEnvironmentAnswer(new CompilationUnit(contents, fullName, null));
             } catch (Exception e) {
                  return null; // treat as if source file is missing
             } finally {
                  if (stream != null) {
                      try {
                         stream.close();
                      } catch (IOException e) { } } } }
       public String toString() {
             return "Classpath for jar file " + zipFile;  }//$NON-NLS-1$
       public void reset() {
             try {
                  if (this.zipFile != null) {
                      this.zipFile.close(); }
                  directoryCache = new Hashtable(101);
             } catch(IOException e) { } } }
