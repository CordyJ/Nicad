/*-------------------------------------------------------------------------
 *
 * nbtcompare.c
 *       Comparison functions for btree access method.
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *       $Header: /cvsroot/pgsql/src/backend/access/nbtree/nbtcompare.c,v 1.43 2001/10/25 05:49:21 momjian Exp $
 *
 * NOTES
 *
 *     These functions are stored in pg_amproc.  For each operator class
 *     defined on btrees, they compute
 *
 *              compare(a, b):
 *                       < 0 if a < b,
 *                       = 0 if a == b,
 *                       > 0 if a > b.
 *
 *     The result is always an int32 regardless of the input datatype.
 *
 *     NOTE: although any negative int32 is acceptable for reporting "<",
 *     and any positive int32 is acceptable for reporting ">", routines
 *     that work on 32-bit or wider datatypes can't just return "a - b".
 *     That could overflow and give the wrong answer.      Also, one should not
 *     return INT_MIN to report "<", since some callers will negate the result.
 *
 *     NOTE: it is critical that the comparison function impose a total order
 *     on all non-NULL values of the data type, and that the datatype's
 *     boolean comparison operators (= < >= etc) yield results consistent
 *     with the comparison routine.  Otherwise bad behavior may ensue.
 *     (For example, the comparison operators must NOT punt when faced with
 *     NAN or other funny values; you must devise some collation sequence for
 *     all such values.)  If the datatype is not trivial, this is most
 *     reliably done by having the boolean operators invoke the same
 *     three-way comparison code that the btree function does.  Therefore,
 *     this file contains only btree support for "trivial" datatypes ---
 *     all others are in the /utils/adt/ files that implement their datatypes.
 *
 *     NOTE: these routines must not leak memory, since memory allocated
 *     during an index access won't be recovered till end of query.  This
 *     primarily affects comparison routines for toastable datatypes;
 *     they have to be careful to free any detoasted copy of an input datum.
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "utils/builtins.h"
Datum
btboolcmp(PG_FUNCTION_ARGS) {
       bool   a = PG_GETARG_BOOL(0);
       bool   b = PG_GETARG_BOOL(1);
       PG_RETURN_INT32((int32) a - (int32) b); }
Datum
btint2cmp(PG_FUNCTION_ARGS) {
       int16         a = PG_GETARG_INT16(0);
       int16         b = PG_GETARG_INT16(1);
       PG_RETURN_INT32((int32) a - (int32) b); }
Datum
btint4cmp(PG_FUNCTION_ARGS) {
       int32         a = PG_GETARG_INT32(0);
       int32         b = PG_GETARG_INT32(1);
       if (a > b)
             PG_RETURN_INT32(1);
       else if (a == b)
             PG_RETURN_INT32(0);
       else
             PG_RETURN_INT32(-1); }
Datum
btint8cmp(PG_FUNCTION_ARGS) {
       int64         a = PG_GETARG_INT64(0);
       int64         b = PG_GETARG_INT64(1);
       if (a > b)
             PG_RETURN_INT32(1);
       else if (a == b)
             PG_RETURN_INT32(0);
       else
             PG_RETURN_INT32(-1); }
Datum
btoidcmp(PG_FUNCTION_ARGS) {
       Oid      a = PG_GETARG_OID(0);
       Oid      b = PG_GETARG_OID(1);
       if (a > b)
             PG_RETURN_INT32(1);
       else if (a == b)
             PG_RETURN_INT32(0);
       else
             PG_RETURN_INT32(-1); }
Datum
btoidvectorcmp(PG_FUNCTION_ARGS) {
       Oid        *a = (Oid *) PG_GETARG_POINTER(0);
       Oid        *b = (Oid *) PG_GETARG_POINTER(1);
       int      i;
       for (i = 0; i < INDEX_MAX_KEYS; i++) {
             if (a[i] != b[i]) {
                  if (a[i] > b[i])
                      PG_RETURN_INT32(1);
                  else
                      PG_RETURN_INT32(-1); } }
       PG_RETURN_INT32(0); }
Datum
btcharcmp(PG_FUNCTION_ARGS) {
       char   a = PG_GETARG_CHAR(0);
       char   b = PG_GETARG_CHAR(1);
       /* Be careful to compare chars as unsigned */
       PG_RETURN_INT32((int32) ((uint8) a) - (int32) ((uint8) b)); }
Datum
btnamecmp(PG_FUNCTION_ARGS) {
       Name   a = PG_GETARG_NAME(0);
       Name   b = PG_GETARG_NAME(1);
       PG_RETURN_INT32(strncmp(NameStr(*a), NameStr(*b), NAMEDATALEN)); }
