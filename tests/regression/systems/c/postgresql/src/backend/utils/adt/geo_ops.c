/*-------------------------------------------------------------------------
 *
 * geo_ops.c
 *       2D geometric operations
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *       $Header: /cvsroot/pgsql/src/backend/utils/adt/geo_ops.c,v 1.60 2001/10/25 05:49:44 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <math.h>
#include <limits.h>
#include <float.h>
#include <ctype.h>
#include "postgres.h"
#include "utils/geo_decls.h"
#ifndef PI
#define PI 3.1415926536
#endif
/*
 * Internal routines
 */
static int     point_inside(Point *p, int npts, Point *plist);
static int     lseg_crossing(double x, double y, double px, double py);
static BOX *box_construct(double x1, double x2, double y1, double y2);
static BOX *box_copy(BOX *box);
static BOX *box_fill(BOX *result, double x1, double x2, double y1, double y2);
static bool box_ov(BOX *box1, BOX *box2);
static double box_ht(BOX *box);
static double box_wd(BOX *box);
static double circle_ar(CIRCLE *circle);
static CIRCLE *circle_copy(CIRCLE *circle);
static LINE *line_construct_pm(Point *pt, double m);
static void line_construct_pts(LINE *line, Point *pt1, Point *pt2);
static bool lseg_intersect_internal(LSEG *l1, LSEG *l2);
static double lseg_dt(LSEG *l1, LSEG *l2);
static bool on_ps_internal(Point *pt, LSEG *lseg);
static void make_bound_box(POLYGON *poly);
static bool plist_same(int npts, Point *p1, Point *p2);
static Point *point_construct(double x, double y);
static Point *point_copy(Point *pt);
static int     single_decode(char *str, float8 *x, char **ss);
static int     single_encode(float8 x, char *str);
static int     pair_decode(char *str, float8 *x, float8 *y, char **s);
static int     pair_encode(float8 x, float8 y, char *str);
static int     pair_count(char *s, char delim);
static int     path_decode(int opentype, int npts, char *str, int *isopen, char **ss, Point *p);
static char *path_encode(bool closed, int npts, Point *pt);
static void statlseg_construct(LSEG *lseg, Point *pt1, Point *pt2);
static double box_ar(BOX *box);
static void box_cn(Point *center, BOX *box);
static Point *interpt_sl(LSEG *lseg, LINE *line);
static bool has_interpt_sl(LSEG *lseg, LINE *line);
static double dist_pl_internal(Point *pt, LINE *line);
static double dist_ps_internal(Point *pt, LSEG *lseg);
static Point *line_interpt_internal(LINE *l1, LINE *l2);
/*
 * Delimiters for input and output strings.
 * LDELIM, RDELIM, and DELIM are left, right, and separator delimiters, respectively.
 * LDELIM_EP, RDELIM_EP are left and right delimiters for paths with endpoints.
 */
#define LDELIM                '('
#define RDELIM                ')'
#define DELIM           ','
#define LDELIM_EP           '['
#define RDELIM_EP           ']'
#define LDELIM_C             '<'
#define RDELIM_C             '>'
/* Maximum number of output digits printed */
#define P_MAXDIG DBL_DIG
#define P_MAXLEN (2*(P_MAXDIG+7)+1)
static int     digits8 = P_MAXDIG;
/*
 * Geometric data types are composed of points.
 * This code tries to support a common format throughout the data types,
 *     to allow for more predictable usage and data type conversion.
 * The fundamental unit is the point. Other units are line segments,
 *     open paths, boxes, closed paths, and polygons (which should be considered
 *     non-intersecting closed paths).
 *
 * Data representation is as follows:
 *     point:                  (x,y)
 *     line segment:             [(x1,y1),(x2,y2)]
 *     box:                          (x1,y1),(x2,y2)
 *     open path:           [(x1,y1),...,(xn,yn)]
 *     closed path:               ((x1,y1),...,(xn,yn))
 *     polygon:         ((x1,y1),...,(xn,yn))
 *
 * For boxes, the points are opposite corners with the first point at the top right.
 * For closed paths and polygons, the points should be reordered to allow
 *     fast and correct equality comparisons.
 *
 * XXX perhaps points in complex shapes should be reordered internally
 *     to allow faster internal operations, but should keep track of input order
 *     and restore that order for text output - tgl 97/01/16
 */
static int
single_decode(char *str, float8 *x, char **s) {
       char     *cp;
       if (!PointerIsValid(str))
             return FALSE;
       while (isspace((unsigned char) *str))
             str++;
       *x = strtod(str, &cp);
#ifdef GEODEBUG
       printf("single_decode- (%x) try decoding %s to %g\n", (cp - str), str, *x);
#endif
       if (cp <= str)
             return FALSE;
       while (isspace((unsigned char) *cp))
             cp++;
       if (s != NULL)
             *s = cp;
       return TRUE;
}      /* single_decode() */
static int
single_encode(float8 x, char *str) {
       sprintf(str, "%.*g", digits8, x);
       return TRUE;
}      /* single_encode() */
static int
pair_decode(char *str, float8 *x, float8 *y, char **s) {
       int      has_delim;
       char     *cp;
       if (!PointerIsValid(str))
             return FALSE;
       while (isspace((unsigned char) *str))
             str++;
       if ((has_delim = (*str == LDELIM)))
             str++;
       while (isspace((unsigned char) *str))
             str++;
       *x = strtod(str, &cp);
       if (cp <= str)
             return FALSE;
       while (isspace((unsigned char) *cp))
             cp++;
       if (*cp++ != DELIM)
             return FALSE;
       while (isspace((unsigned char) *cp))
             cp++;
       *y = strtod(cp, &str);
       if (str <= cp)
             return FALSE;
       while (isspace((unsigned char) *str))
             str++;
       if (has_delim) {
             if (*str != RDELIM)
                  return FALSE;
             str++;
             while (isspace((unsigned char) *str))
                  str++; }
       if (s != NULL)
             *s = str;
       return TRUE; }
static int
pair_encode(float8 x, float8 y, char *str) {
       sprintf(str, "%.*g,%.*g", digits8, x, digits8, y);
       return TRUE; }
static int
path_decode(int opentype, int npts, char *str, int *isopen, char **ss, Point *p) {
       int      depth = 0;
       char     *s,
                     *cp;
       int      i;
       s = str;
       while (isspace((unsigned char) *s))
             s++;
       if ((*isopen = (*s == LDELIM_EP))) {
             /* no open delimiter allowed? */
             if (!opentype)
                  return FALSE;
             depth++;
             s++;
             while (isspace((unsigned char) *s))
                  s++; }
       else if (*s == LDELIM) {
             cp = (s + 1);
             while (isspace((unsigned char) *cp))
                  cp++;
             if (*cp == LDELIM) {
#ifdef NOT_USED
                  /* nested delimiters with only one point? */
                  if (npts <= 1)
                      return FALSE;
#endif
                  depth++;
                  s = cp; }
             else if (strrchr(s, LDELIM) == s) {
                  depth++;
                  s = cp; } }
       for (i = 0; i < npts; i++) {
             if (!pair_decode(s, &(p->x), &(p->y), &s))
                  return FALSE;
             if (*s == DELIM)
                  s++;
             p++; }
       while (depth > 0) {
             if ((*s == RDELIM)
                  || ((*s == RDELIM_EP) && (*isopen) && (depth == 1))) {
                  depth--;
                  s++;
                  while (isspace((unsigned char) *s))
                      s++; }
             else
                  return FALSE; }
       *ss = s;
       return TRUE;
}      /* path_decode() */
static char *
path_encode(bool closed, int npts, Point *pt) {
       char     *result = palloc(npts * (P_MAXLEN + 3) + 2);
       char     *cp;
       int      i;
       cp = result;
       switch (closed) {
             case TRUE:
                  *cp++ = LDELIM;
                  break;
             case FALSE:
                  *cp++ = LDELIM_EP;
                  break;
             default:
                  break; }
       for (i = 0; i < npts; i++) {
             *cp++ = LDELIM;
             if (!pair_encode(pt->x, pt->y, cp))
                  elog(ERROR, "Unable to format path");
             cp += strlen(cp);
             *cp++ = RDELIM;
             *cp++ = DELIM;
             pt++; }
       cp--;
       switch (closed) {
             case TRUE:
                  *cp++ = RDELIM;
                  break;
             case FALSE:
                  *cp++ = RDELIM_EP;
                  break;
             default:
                  break; }
       *cp = '\0';
       return result;
}      /* path_encode() */
/*-------------------------------------------------------------
 * pair_count - count the number of points
 * allow the following notation:
 * '((1,2),(3,4))'
 * '(1,3,2,4)'
 * require an odd number of delim characters in the string
 *-------------------------------------------------------------*/
static int
pair_count(char *s, char delim) {
       int      ndelim = 0;
       while ((s = strchr(s, delim)) != NULL) {
             ndelim++;
             s++; }
       return (ndelim % 2) ? ((ndelim + 1) / 2) : -1; }
/***********************************************************************
 **
 **       Routines for two-dimensional boxes.
 **
 ***********************************************************************/
/*----------------------------------------------------------
 * Formatting and conversion routines.
 *---------------------------------------------------------*/
/*         box_in     -     convert a string to internal form.
 *
 *         External format: (two corners of box)
 *              "(f8, f8), (f8, f8)"
 *              also supports the older style "(f8, f8, f8, f8)"
 */
Datum
box_in(PG_FUNCTION_ARGS) {
       char     *str = PG_GETARG_CSTRING(0);
       BOX        *box = (BOX *) palloc(sizeof(BOX));
       int      isopen;
       char     *s;
       double               x,
                      y;
       if ((!path_decode(FALSE, 2, str, &isopen, &s, &(box->high)))
             || (*s != '\0'))
             elog(ERROR, "Bad box external representation '%s'", str);
       /* reorder corners if necessary... */
       if (box->high.x < box->low.x) {
             x = box->high.x;
             box->high.x = box->low.x;
             box->low.x = x; }
       if (box->high.y < box->low.y) {
             y = box->high.y;
             box->high.y = box->low.y;
             box->low.y = y; }
       PG_RETURN_BOX_P(box); }
/*         box_out -   convert a box to external form.
 */
Datum
box_out(PG_FUNCTION_ARGS) {
       BOX        *box = PG_GETARG_BOX_P(0);
       PG_RETURN_CSTRING(path_encode(-1, 2, &(box->high))); }
/*         box_construct      -       fill in a new box.
 */
static BOX *
box_construct(double x1, double x2, double y1, double y2) {
       BOX        *result = (BOX *) palloc(sizeof(BOX));
       return box_fill(result, x1, x2, y1, y2); }
/*         box_fill     -               fill in a given box struct
 */
static BOX *
box_fill(BOX *result, double x1, double x2, double y1, double y2) {
       if (x1 > x2) {
             result->high.x = x1;
             result->low.x = x2; }
       else {
             result->high.x = x2;
             result->low.x = x1; }
       if (y1 > y2) {
             result->high.y = y1;
             result->low.y = y2; }
       else {
             result->high.y = y2;
             result->low.y = y1; }
       return result; }
/*         box_copy     -               copy a box
 */
static BOX *
box_copy(BOX *box) {
       BOX        *result = (BOX *) palloc(sizeof(BOX));
       memcpy((char *) result, (char *) box, sizeof(BOX));
       return result; }
/*----------------------------------------------------------
 *     Relational operators for BOXes.
 *         <, >, <=, >=, and == are based on box area.
 *---------------------------------------------------------*/
/*         box_same     -               are two boxes identical?
 */
Datum
box_same(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(FPeq(box1->high.x, box2->high.x) &&
                         FPeq(box1->low.x, box2->low.x) &&
                         FPeq(box1->high.y, box2->high.y) &&
                         FPeq(box1->low.y, box2->low.y)); }
/*         box_overlap               -         does box1 overlap box2?
 */
Datum
box_overlap(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(box_ov(box1, box2)); }
static bool
box_ov(BOX *box1, BOX *box2) {
       return ((FPge(box1->high.x, box2->high.x) &&
                   FPle(box1->low.x, box2->high.x)) ||
                  (FPge(box2->high.x, box1->high.x) &&
                   FPle(box2->low.x, box1->high.x)))
             &&
             ((FPge(box1->high.y, box2->high.y) &&
               FPle(box1->low.y, box2->high.y)) ||
              (FPge(box2->high.y, box1->high.y) &&
               FPle(box2->low.y, box1->high.y))); }
/*         box_overleft       -         is the right edge of box1 to the left of
 *                                    the right edge of box2?
 *
 *         This is "less than or equal" for the end of a time range,
 *         when time ranges are stored as rectangles.
 */
Datum
box_overleft(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(FPle(box1->high.x, box2->high.x)); }
/*         box_left     -               is box1 strictly left of box2?
 */
Datum
box_left(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(FPlt(box1->high.x, box2->low.x)); }
/*         box_right   -             is box1 strictly right of box2?
 */
Datum
box_right(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(FPgt(box1->low.x, box2->high.x)); }
/*         box_overright      -       is the left edge of box1 to the right of
 *                                    the left edge of box2?
 *
 *         This is "greater than or equal" for time ranges, when time ranges
 *         are stored as rectangles.
 */
Datum
box_overright(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(FPge(box1->low.x, box2->low.x)); }
/*         box_contained      -       is box1 contained by box2?
 */
Datum
box_contained(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(FPle(box1->high.x, box2->high.x) &&
                         FPge(box1->low.x, box2->low.x) &&
                         FPle(box1->high.y, box2->high.y) &&
                         FPge(box1->low.y, box2->low.y)); }
/*         box_contain               -         does box1 contain box2?
 */
Datum
box_contain(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(FPge(box1->high.x, box2->high.x) &&
                         FPle(box1->low.x, box2->low.x) &&
                         FPge(box1->high.y, box2->high.y) &&
                         FPle(box1->low.y, box2->low.y)); }
/*         box_positionop     -
 *              is box1 entirely {above,below} box2?
 */
Datum
box_below(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(FPle(box1->high.y, box2->low.y)); }
Datum
box_above(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(FPge(box1->low.y, box2->high.y)); }
/*         box_relop   -             is area(box1) relop area(box2), within
 *                                    our accuracy constraint?
 */
Datum
box_lt(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(FPlt(box_ar(box1), box_ar(box2))); }
Datum
box_gt(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(FPgt(box_ar(box1), box_ar(box2))); }
Datum
box_eq(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(FPeq(box_ar(box1), box_ar(box2))); }
Datum
box_le(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(FPle(box_ar(box1), box_ar(box2))); }
Datum
box_ge(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(FPge(box_ar(box1), box_ar(box2))); }
/*----------------------------------------------------------
 *     "Arithmetic" operators on boxes.
 *---------------------------------------------------------*/
/*         box_area     -               returns the area of the box.
 */
Datum
box_area(PG_FUNCTION_ARGS) {
       BOX        *box = PG_GETARG_BOX_P(0);
       PG_RETURN_FLOAT8(box_ar(box)); }
/*         box_width   -             returns the width of the box
 *                                      (horizontal magnitude).
 */
Datum
box_width(PG_FUNCTION_ARGS) {
       BOX        *box = PG_GETARG_BOX_P(0);
       PG_RETURN_FLOAT8(box->high.x - box->low.x); }
/*         box_height         -           returns the height of the box
 *                                      (vertical magnitude).
 */
Datum
box_height(PG_FUNCTION_ARGS) {
       BOX        *box = PG_GETARG_BOX_P(0);
       PG_RETURN_FLOAT8(box->high.y - box->low.y); }
/*         box_distance       -         returns the distance between the
 *                                      center points of two boxes.
 */
Datum
box_distance(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       Point         a,
                      b;
       box_cn(&a, box1);
       box_cn(&b, box2);
       PG_RETURN_FLOAT8(HYPOT(a.x - b.x, a.y - b.y)); }
/*         box_center         -           returns the center point of the box.
 */
Datum
box_center(PG_FUNCTION_ARGS) {
       BOX        *box = PG_GETARG_BOX_P(0);
       Point    *result = (Point *) palloc(sizeof(Point));
       box_cn(result, box);
       PG_RETURN_POINT_P(result); }
/*         box_ar     -     returns the area of the box.
 */
static double
box_ar(BOX *box) {
       return box_wd(box) * box_ht(box); }
/*         box_cn     -     stores the centerpoint of the box into *center.
 */
static void
box_cn(Point *center, BOX *box) {
       center->x = (box->high.x + box->low.x) / 2.0;
       center->y = (box->high.y + box->low.y) / 2.0; }
/*         box_wd     -     returns the width (length) of the box
 *                                      (horizontal magnitude).
 */
static double
box_wd(BOX *box) {
       return box->high.x - box->low.x; }
/*         box_ht     -     returns the height of the box
 *                                      (vertical magnitude).
 */
static double
box_ht(BOX *box) {
       return box->high.y - box->low.y; }
/*----------------------------------------------------------
 *     Funky operations.
 *---------------------------------------------------------*/
/*         box_intersect      -
 *              returns the overlapping portion of two boxes,
 *                or NULL if they do not intersect.
 */
Datum
box_intersect(PG_FUNCTION_ARGS) {
       BOX        *box1 = PG_GETARG_BOX_P(0);
       BOX        *box2 = PG_GETARG_BOX_P(1);
       BOX        *result;
       if (!box_ov(box1, box2))
             PG_RETURN_NULL();
       result = (BOX *) palloc(sizeof(BOX));
       result->high.x = Min(box1->high.x, box2->high.x);
       result->low.x = Max(box1->low.x, box2->low.x);
       result->high.y = Min(box1->high.y, box2->high.y);
       result->low.y = Max(box1->low.y, box2->low.y);
       PG_RETURN_BOX_P(result); }
/*         box_diagonal       -
 *              returns a line segment which happens to be the
 *                positive-slope diagonal of "box".
 */
Datum
box_diagonal(PG_FUNCTION_ARGS) {
       BOX        *box = PG_GETARG_BOX_P(0);
       LSEG     *result = (LSEG *) palloc(sizeof(LSEG));
       statlseg_construct(result, &box->high, &box->low);
       PG_RETURN_LSEG_P(result); }
/***********************************************************************
 **
 **       Routines for 2D lines.
 **          Lines are not intended to be used as ADTs per se,
 **          but their ops are useful tools for other ADT ops.  Thus,
 **          there are few relops.
 **
 ***********************************************************************/
Datum
line_in(PG_FUNCTION_ARGS) {
#ifdef ENABLE_LINE_TYPE
       char     *str = PG_GETARG_CSTRING(0);
#endif
       LINE     *line;
#ifdef ENABLE_LINE_TYPE
       LSEG   lseg;
       int      isopen;
       char     *s;
       if ((!path_decode(TRUE, 2, str, &isopen, &s, &(lseg.p[0])))
             || (*s != '\0'))
             elog(ERROR, "Bad line external representation '%s'", str);
       line = (LINE *) palloc(sizeof(LINE));
       line_construct_pts(line, &lseg.p[0], &lseg.p[1]);
#else
       elog(ERROR, "line not yet implemented");
       line = NULL;
#endif
       PG_RETURN_LINE_P(line); }
Datum
line_out(PG_FUNCTION_ARGS) {
#ifdef ENABLE_LINE_TYPE
       LINE     *line = PG_GETARG_LINE_P(0);
#endif
       char     *result;
#ifdef ENABLE_LINE_TYPE
       LSEG   lseg;
       if (FPzero(line->B))
       {                              /* vertical */
             /* use "x = C" */
             result->A = -1;
             result->B = 0;
             result->C = pt1->x;
#ifdef GEODEBUG
             printf("line_out- line is vertical\n");
#endif
#ifdef NOT_USED
             result->m = DBL_MAX;
#endif
       }
       else if (FPzero(line->A))
       {                              /* horizontal */
             /* use "x = C" */
             result->A = 0;
             result->B = -1;
             result->C = pt1->y;
#ifdef GEODEBUG
             printf("line_out- line is horizontal\n");
#endif
#ifdef NOT_USED
             result->m = 0.0;
#endif
       }
       else { }
       if (FPzero(line->A))   /* horizontal? */ { }
       else if (FPzero(line->B))     /* vertical? */ { }
       else { }
       return path_encode(TRUE, 2, (Point *) &(ls->p[0]));
#else
       elog(ERROR, "line not yet implemented");
       result = NULL;
#endif
       PG_RETURN_CSTRING(result); }
/*----------------------------------------------------------
 *     Conversion routines from one line formula to internal.
 *         Internal form:     Ax+By+C=0
 *---------------------------------------------------------*/
/* line_construct_pm()
 * point-slope
 */
static LINE *
line_construct_pm(Point *pt, double m) {
       LINE     *result = (LINE *) palloc(sizeof(LINE));
       /* use "mx - y + yinter = 0" */
       result->A = m;
       result->B = -1.0;
       if (m == DBL_MAX)
             result->C = pt->y;
       else
             result->C = pt->y - m * pt->x;
#ifdef NOT_USED
       result->m = m;
#endif
       return result; }
/*
 * Fill already-allocated LINE struct from two points on the line
 */
static void
line_construct_pts(LINE *line, Point *pt1, Point *pt2) {
       if (FPeq(pt1->x, pt2->x))
       {                              /* vertical */
             /* use "x = C" */
             line->A = -1;
             line->B = 0;
             line->C = pt1->x;
#ifdef NOT_USED
             line->m = DBL_MAX;
#endif
#ifdef GEODEBUG
             printf("line_construct_pts- line is vertical\n");
#endif
       }
       else if (FPeq(pt1->y, pt2->y))
       {                              /* horizontal */
             /* use "y = C" */
             line->A = 0;
             line->B = -1;
             line->C = pt1->y;
#ifdef NOT_USED
             line->m = 0.0;
#endif
#ifdef GEODEBUG
             printf("line_construct_pts- line is horizontal\n");
#endif
       }
       else {
             /* use "mx - y + yinter = 0" */
             line->A = (pt2->y - pt1->y) / (pt2->x - pt1->x);
             line->B = -1.0;
             line->C = pt1->y - line->A * pt1->x;
#ifdef NOT_USED
             line->m = line->A;
#endif
#ifdef GEODEBUG
             printf("line_construct_pts- line is neither vertical nor horizontal (diffs x=%.*g, y=%.*g\n",
                     digits8, (pt2->x - pt1->x), digits8, (pt2->y - pt1->y));
#endif
       } }
/* line_construct_pp()
 * two points
 */
Datum
line_construct_pp(PG_FUNCTION_ARGS) {
       Point    *pt1 = PG_GETARG_POINT_P(0);
       Point    *pt2 = PG_GETARG_POINT_P(1);
       LINE     *result = (LINE *) palloc(sizeof(LINE));
       line_construct_pts(result, pt1, pt2);
       PG_RETURN_LINE_P(result); }
/*----------------------------------------------------------
 *     Relative position routines.
 *---------------------------------------------------------*/
Datum
line_intersect(PG_FUNCTION_ARGS) {
       LINE     *l1 = PG_GETARG_LINE_P(0);
       LINE     *l2 = PG_GETARG_LINE_P(1);
       PG_RETURN_BOOL(!DatumGetBool(DirectFunctionCall2(line_parallel,
                                                              LinePGetDatum(l1),
                                                              LinePGetDatum(l2)))); }
Datum
line_parallel(PG_FUNCTION_ARGS) {
       LINE     *l1 = PG_GETARG_LINE_P(0);
       LINE     *l2 = PG_GETARG_LINE_P(1);
#ifdef NOT_USED
       PG_RETURN_BOOL(FPeq(l1->m, l2->m));
#endif
       if (FPzero(l1->B))
             PG_RETURN_BOOL(FPzero(l2->B));
       PG_RETURN_BOOL(FPeq(l2->A, l1->A * (l2->B / l1->B))); }
Datum
line_perp(PG_FUNCTION_ARGS) {
       LINE     *l1 = PG_GETARG_LINE_P(0);
       LINE     *l2 = PG_GETARG_LINE_P(1);
#ifdef NOT_USED
       if (l1->m)
             PG_RETURN_BOOL(FPeq(l2->m / l1->m, -1.0));
       else if (l2->m)
             PG_RETURN_BOOL(FPeq(l1->m / l2->m, -1.0));
#endif
       if (FPzero(l1->A))
             PG_RETURN_BOOL(FPzero(l2->B));
       else if (FPzero(l1->B))
             PG_RETURN_BOOL(FPzero(l2->A));
       PG_RETURN_BOOL(FPeq(((l1->A * l2->B) / (l1->B * l2->A)), -1.0)); }
Datum
line_vertical(PG_FUNCTION_ARGS) {
       LINE     *line = PG_GETARG_LINE_P(0);
       PG_RETURN_BOOL(FPzero(line->B)); }
Datum
line_horizontal(PG_FUNCTION_ARGS) {
       LINE     *line = PG_GETARG_LINE_P(0);
       PG_RETURN_BOOL(FPzero(line->A)); }
Datum
line_eq(PG_FUNCTION_ARGS) {
       LINE     *l1 = PG_GETARG_LINE_P(0);
       LINE     *l2 = PG_GETARG_LINE_P(1);
       double               k;
       if (!FPzero(l2->A))
             k = l1->A / l2->A;
       else if (!FPzero(l2->B))
             k = l1->B / l2->B;
       else if (!FPzero(l2->C))
             k = l1->C / l2->C;
       else
             k = 1.0;
       PG_RETURN_BOOL(FPeq(l1->A, k * l2->A) &&
                         FPeq(l1->B, k * l2->B) &&
                         FPeq(l1->C, k * l2->C)); }
/*----------------------------------------------------------
 *     Line arithmetic routines.
 *---------------------------------------------------------*/
/* line_distance()
 * Distance between two lines.
 */
Datum
line_distance(PG_FUNCTION_ARGS) {
       LINE     *l1 = PG_GETARG_LINE_P(0);
       LINE     *l2 = PG_GETARG_LINE_P(1);
       float8               result;
       Point    *tmp;
       if (!DatumGetBool(DirectFunctionCall2(line_parallel,
                                                   LinePGetDatum(l1),
                                                   LinePGetDatum(l2))))
             PG_RETURN_FLOAT8(0.0);
       if (FPzero(l1->B))         /* vertical? */
             PG_RETURN_FLOAT8(fabs(l1->C - l2->C));
       tmp = point_construct(0.0, l1->C);
       result = dist_pl_internal(tmp, l2);
       pfree(tmp);
       PG_RETURN_FLOAT8(result); }
/* line_interpt()
 * Point where two lines l1, l2 intersect (if any)
 */
Datum
line_interpt(PG_FUNCTION_ARGS) {
       LINE     *l1 = PG_GETARG_LINE_P(0);
       LINE     *l2 = PG_GETARG_LINE_P(1);
       Point    *result;
       result = line_interpt_internal(l1, l2);
       if (result == NULL)
             PG_RETURN_NULL();
       PG_RETURN_POINT_P(result); }
/*
 * Internal version of line_interpt
 *
 * returns a NULL pointer if no intersection point
 */
static Point *
line_interpt_internal(LINE *l1, LINE *l2) {
       Point    *result;
       double               x,
                      y;
       /*
        * NOTE: if the lines are identical then we will find they are
        * parallel and report "no intersection".  This is a little weird, but
        * since there's no *unique* intersection, maybe it's appropriate
        * behavior.
        */
       if (DatumGetBool(DirectFunctionCall2(line_parallel,
                                                  LinePGetDatum(l1),
                                                  LinePGetDatum(l2))))
             return NULL;
#ifdef NOT_USED
       if (FPzero(l1->B))         /* l1 vertical? */
             result = point_construct(l2->m * l1->C + l2->C, l1->C);
       else if (FPzero(l2->B))             /* l2 vertical? */
             result = point_construct(l1->m * l2->C + l1->C, l2->C);
       else {
             x = (l1->C - l2->C) / (l2->A - l1->A);
             result = point_construct(x, l1->m * x + l1->C); }
#endif
       if (FPzero(l1->B))         /* l1 vertical? */ {
             x = l1->C;
             y = (l2->A * x + l2->C); }
       else if (FPzero(l2->B))             /* l2 vertical? */ {
             x = l2->C;
             y = (l1->A * x + l1->C); }
       else {
             x = (l1->C - l2->C) / (l2->A - l1->A);
             y = (l1->A * x + l1->C); }
       result = point_construct(x, y);
#ifdef GEODEBUG
       printf("line_interpt- lines are A=%.*g, B=%.*g, C=%.*g, A=%.*g, B=%.*g, C=%.*g\n",
                digits8, l1->A, digits8, l1->B, digits8, l1->C, digits8, l2->A, digits8, l2->B, digits8, l2->C);
       printf("line_interpt- lines intersect at (%.*g,%.*g)\n", digits8, x, digits8, y);
#endif
       return result; }
/***********************************************************************
 **
 **       Routines for 2D paths (sequences of line segments, also
 **          called `polylines').
 **
 **          This is not a general package for geometric paths,
 **          which of course include polygons; the emphasis here
 **          is on (for example) usefulness in wire layout.
 **
 ***********************************************************************/
/*----------------------------------------------------------
 *     String to path / path to string conversion.
 *         External format:
 *              "((xcoord, ycoord),... )"
 *              "[(xcoord, ycoord),... ]"
 *              "(xcoord, ycoord),... "
 *              "[xcoord, ycoord,... ]"
 *         Also support older format:
 *              "(closed, npts, xcoord, ycoord,... )"
 *---------------------------------------------------------*/
Datum
path_in(PG_FUNCTION_ARGS) {
       char     *str = PG_GETARG_CSTRING(0);
       PATH     *path;
       int      isopen;
       char     *s;
       int      npts;
       int      size;
       int      depth = 0;
       if ((npts = pair_count(str, ',')) <= 0)
             elog(ERROR, "Bad path external representation '%s'", str);
       s = str;
       while (isspace((unsigned char) *s))
             s++;
       /* skip single leading paren */
       if ((*s == LDELIM) && (strrchr(s, LDELIM) == s)) {
             s++;
             depth++; }
       size = offsetof(PATH, p[0]) +sizeof(path->p[0]) * npts;
       path = (PATH *) palloc(size);
       path->size = size;
       path->npts = npts;
       if ((!path_decode(TRUE, npts, s, &isopen, &s, &(path->p[0])))
             && (!((depth == 0) && (*s == '\0'))) && !((depth >= 1) && (*s == RDELIM)))
             elog(ERROR, "Bad path external representation '%s'", str);
       path->closed = (!isopen);
       PG_RETURN_PATH_P(path); }
Datum
path_out(PG_FUNCTION_ARGS) {
       PATH     *path = PG_GETARG_PATH_P(0);
       PG_RETURN_CSTRING(path_encode(path->closed, path->npts, path->p)); }
/*----------------------------------------------------------
 *     Relational operators.
 *         These are based on the path cardinality,
 *         as stupid as that sounds.
 *
 *         Better relops and access methods coming soon.
 *---------------------------------------------------------*/
Datum
path_n_lt(PG_FUNCTION_ARGS) {
       PATH     *p1 = PG_GETARG_PATH_P(0);
       PATH     *p2 = PG_GETARG_PATH_P(1);
       PG_RETURN_BOOL(p1->npts < p2->npts); }
Datum
path_n_gt(PG_FUNCTION_ARGS) {
       PATH     *p1 = PG_GETARG_PATH_P(0);
       PATH     *p2 = PG_GETARG_PATH_P(1);
       PG_RETURN_BOOL(p1->npts > p2->npts); }
Datum
path_n_eq(PG_FUNCTION_ARGS) {
       PATH     *p1 = PG_GETARG_PATH_P(0);
       PATH     *p2 = PG_GETARG_PATH_P(1);
       PG_RETURN_BOOL(p1->npts == p2->npts); }
Datum
path_n_le(PG_FUNCTION_ARGS) {
       PATH     *p1 = PG_GETARG_PATH_P(0);
       PATH     *p2 = PG_GETARG_PATH_P(1);
       PG_RETURN_BOOL(p1->npts <= p2->npts); }
Datum
path_n_ge(PG_FUNCTION_ARGS) {
       PATH     *p1 = PG_GETARG_PATH_P(0);
       PATH     *p2 = PG_GETARG_PATH_P(1);
       PG_RETURN_BOOL(p1->npts >= p2->npts); }
/*----------------------------------------------------------
 * Conversion operators.
 *---------------------------------------------------------*/
Datum
path_isclosed(PG_FUNCTION_ARGS) {
       PATH     *path = PG_GETARG_PATH_P(0);
       PG_RETURN_BOOL(path->closed); }
Datum
path_isopen(PG_FUNCTION_ARGS) {
       PATH     *path = PG_GETARG_PATH_P(0);
       PG_RETURN_BOOL(!path->closed); }
Datum
path_npoints(PG_FUNCTION_ARGS) {
       PATH     *path = PG_GETARG_PATH_P(0);
       PG_RETURN_INT32(path->npts); }
Datum
path_close(PG_FUNCTION_ARGS) {
       PATH     *path = PG_GETARG_PATH_P_COPY(0);
       path->closed = TRUE;
       PG_RETURN_PATH_P(path); }
Datum
path_open(PG_FUNCTION_ARGS) {
       PATH     *path = PG_GETARG_PATH_P_COPY(0);
       path->closed = FALSE;
       PG_RETURN_PATH_P(path); }
/* path_inter -
 *         Does p1 intersect p2 at any point?
 *         Use bounding boxes for a quick (O(n)) check, then do a
 *         O(n^2) iterative edge check.
 */
Datum
path_inter(PG_FUNCTION_ARGS) {
       PATH     *p1 = PG_GETARG_PATH_P(0);
       PATH     *p2 = PG_GETARG_PATH_P(1);
       BOX      b1,
                      b2;
       int      i,
                      j;
       LSEG   seg1,
                      seg2;
       if (p1->npts <= 0 || p2->npts <= 0)
             PG_RETURN_BOOL(false);
       b1.high.x = b1.low.x = p1->p[0].x;
       b1.high.y = b1.low.y = p1->p[0].y;
       for (i = 1; i < p1->npts; i++) {
             b1.high.x = Max(p1->p[i].x, b1.high.x);
             b1.high.y = Max(p1->p[i].y, b1.high.y);
             b1.low.x = Min(p1->p[i].x, b1.low.x);
             b1.low.y = Min(p1->p[i].y, b1.low.y); }
       b2.high.x = b2.low.x = p2->p[0].x;
       b2.high.y = b2.low.y = p2->p[0].y;
       for (i = 1; i < p2->npts; i++) {
             b2.high.x = Max(p2->p[i].x, b2.high.x);
             b2.high.y = Max(p2->p[i].y, b2.high.y);
             b2.low.x = Min(p2->p[i].x, b2.low.x);
             b2.low.y = Min(p2->p[i].y, b2.low.y); }
       if (!box_ov(&b1, &b2))
             PG_RETURN_BOOL(false);
       /* pairwise check lseg intersections */
       for (i = 0; i < p1->npts; i++) {
             int           iprev;
             if (i > 0)
                  iprev = i - 1;
             else {
                  if (!p1->closed)
                      continue;
                  iprev = p1->npts - 1;             /* include the closure segment */ }
             for (j = 0; j < p2->npts; j++) {
                  int                jprev;
                  if (j > 0)
                      jprev = j - 1;
                  else {
                      if (!p2->closed)
                         continue;
                      jprev = p2->npts - 1;      /* include the closure segment */ }
                  statlseg_construct(&seg1, &p1->p[iprev], &p1->p[i]);
                  statlseg_construct(&seg2, &p2->p[jprev], &p2->p[j]);
                  if (lseg_intersect_internal(&seg1, &seg2))
                      PG_RETURN_BOOL(true); } }
       /* if we dropped through, no two segs intersected */
       PG_RETURN_BOOL(false); }
/* path_distance()
 * This essentially does a cartesian product of the lsegs in the
 *     two paths, and finds the min distance between any two lsegs
 */
Datum
path_distance(PG_FUNCTION_ARGS) {
       PATH     *p1 = PG_GETARG_PATH_P(0);
       PATH     *p2 = PG_GETARG_PATH_P(1);
       float8               min = 0.0;       /* initialize to keep compiler quiet */
       bool   have_min = false;
       float8               tmp;
       int      i,
                      j;
       LSEG   seg1,
                      seg2;
       for (i = 0; i < p1->npts; i++) {
             int           iprev;
             if (i > 0)
                  iprev = i - 1;
             else {
                  if (!p1->closed)
                      continue;
                  iprev = p1->npts - 1;             /* include the closure segment */ }
             for (j = 0; j < p2->npts; j++) {
                  int                jprev;
                  if (j > 0)
                      jprev = j - 1;
                  else {
                      if (!p2->closed)
                         continue;
                      jprev = p2->npts - 1;      /* include the closure segment */ }
                  statlseg_construct(&seg1, &p1->p[iprev], &p1->p[i]);
                  statlseg_construct(&seg2, &p2->p[jprev], &p2->p[j]);
                  tmp = DatumGetFloat8(DirectFunctionCall2(lseg_distance,
                                                              LsegPGetDatum(&seg1),
                                                            LsegPGetDatum(&seg2)));
                  if (!have_min || tmp < min) {
                      min = tmp;
                      have_min = true; } } }
       if (!have_min)
             PG_RETURN_NULL();
       PG_RETURN_FLOAT8(min); }
/*----------------------------------------------------------
 *     "Arithmetic" operations.
 *---------------------------------------------------------*/
Datum
path_length(PG_FUNCTION_ARGS) {
       PATH     *path = PG_GETARG_PATH_P(0);
       float8               result = 0.0;
       int      i;
       for (i = 0; i < path->npts; i++) {
             int           iprev;
             if (i > 0)
                  iprev = i - 1;
             else {
                  if (!path->closed)
                      continue;
                  iprev = path->npts - 1;         /* include the closure segment */ }
             result += point_dt(&path->p[iprev], &path->p[i]); }
       PG_RETURN_FLOAT8(result); }
/***********************************************************************
 **
 **       Routines for 2D points.
 **
 ***********************************************************************/
/*----------------------------------------------------------
 *     String to point, point to string conversion.
 *         External format:
 *              "(x,y)"
 *              "x,y"
 *---------------------------------------------------------*/
Datum
point_in(PG_FUNCTION_ARGS) {
       char     *str = PG_GETARG_CSTRING(0);
       Point    *point;
       double               x,
                      y;
       char     *s;
       if (!pair_decode(str, &x, &y, &s) || (*s != '\0'))
             elog(ERROR, "Bad point external representation '%s'", str);
       point = (Point *) palloc(sizeof(Point));
       point->x = x;
       point->y = y;
       PG_RETURN_POINT_P(point); }
Datum
point_out(PG_FUNCTION_ARGS) {
       Point    *pt = PG_GETARG_POINT_P(0);
       PG_RETURN_CSTRING(path_encode(-1, 1, pt)); }
static Point *
point_construct(double x, double y) {
       Point    *result = (Point *) palloc(sizeof(Point));
       result->x = x;
       result->y = y;
       return result; }
static Point *
point_copy(Point *pt) {
       Point    *result;
       if (!PointerIsValid(pt))
             return NULL;
       result = (Point *) palloc(sizeof(Point));
       result->x = pt->x;
       result->y = pt->y;
       return result; }
/*----------------------------------------------------------
 *     Relational operators for Points.
 *         Since we do have a sense of coordinates being
 *         "equal" to a given accuracy (point_vert, point_horiz),
 *         the other ops must preserve that sense.  This means
 *         that results may, strictly speaking, be a lie (unless
 *         EPSILON = 0.0).
 *---------------------------------------------------------*/
Datum
point_left(PG_FUNCTION_ARGS) {
       Point    *pt1 = PG_GETARG_POINT_P(0);
       Point    *pt2 = PG_GETARG_POINT_P(1);
       PG_RETURN_BOOL(FPlt(pt1->x, pt2->x)); }
Datum
point_right(PG_FUNCTION_ARGS) {
       Point    *pt1 = PG_GETARG_POINT_P(0);
       Point    *pt2 = PG_GETARG_POINT_P(1);
       PG_RETURN_BOOL(FPgt(pt1->x, pt2->x)); }
Datum
point_above(PG_FUNCTION_ARGS) {
       Point    *pt1 = PG_GETARG_POINT_P(0);
       Point    *pt2 = PG_GETARG_POINT_P(1);
       PG_RETURN_BOOL(FPgt(pt1->y, pt2->y)); }
Datum
point_below(PG_FUNCTION_ARGS) {
       Point    *pt1 = PG_GETARG_POINT_P(0);
       Point    *pt2 = PG_GETARG_POINT_P(1);
       PG_RETURN_BOOL(FPlt(pt1->y, pt2->y)); }
Datum
point_vert(PG_FUNCTION_ARGS) {
       Point    *pt1 = PG_GETARG_POINT_P(0);
       Point    *pt2 = PG_GETARG_POINT_P(1);
       PG_RETURN_BOOL(FPeq(pt1->x, pt2->x)); }
Datum
point_horiz(PG_FUNCTION_ARGS) {
       Point    *pt1 = PG_GETARG_POINT_P(0);
       Point    *pt2 = PG_GETARG_POINT_P(1);
       PG_RETURN_BOOL(FPeq(pt1->y, pt2->y)); }
Datum
point_eq(PG_FUNCTION_ARGS) {
       Point    *pt1 = PG_GETARG_POINT_P(0);
       Point    *pt2 = PG_GETARG_POINT_P(1);
       PG_RETURN_BOOL(FPeq(pt1->x, pt2->x) && FPeq(pt1->y, pt2->y)); }
Datum
point_ne(PG_FUNCTION_ARGS) {
       Point    *pt1 = PG_GETARG_POINT_P(0);
       Point    *pt2 = PG_GETARG_POINT_P(1);
       PG_RETURN_BOOL(FPne(pt1->x, pt2->x) || FPne(pt1->y, pt2->y)); }
/*----------------------------------------------------------
 *     "Arithmetic" operators on points.
 *---------------------------------------------------------*/
Datum
point_distance(PG_FUNCTION_ARGS) {
       Point    *pt1 = PG_GETARG_POINT_P(0);
       Point    *pt2 = PG_GETARG_POINT_P(1);
       PG_RETURN_FLOAT8(HYPOT(pt1->x - pt2->x, pt1->y - pt2->y)); }
double
point_dt(Point *pt1, Point *pt2) {
#ifdef GEODEBUG
       printf("point_dt- segment (%f,%f),(%f,%f) length is %f\n",
                pt1->x, pt1->y, pt2->x, pt2->y, HYPOT(pt1->x - pt2->x, pt1->y - pt2->y));
#endif
       return HYPOT(pt1->x - pt2->x, pt1->y - pt2->y); }
Datum
point_slope(PG_FUNCTION_ARGS) {
       Point    *pt1 = PG_GETARG_POINT_P(0);
       Point    *pt2 = PG_GETARG_POINT_P(1);
       PG_RETURN_FLOAT8(point_sl(pt1, pt2)); }
double
point_sl(Point *pt1, Point *pt2) {
       return (FPeq(pt1->x, pt2->x)
                  ? (double) DBL_MAX
                  : (pt1->y - pt2->y) / (pt1->x - pt2->x)); }
/***********************************************************************
 **
 **       Routines for 2D line segments.
 **
 ***********************************************************************/
/*----------------------------------------------------------
 *     String to lseg, lseg to string conversion.
 *         External forms: "[(x1, y1), (x2, y2)]"
 *                       "(x1, y1), (x2, y2)"
 *                       "x1, y1, x2, y2"
 *         closed form ok     "((x1, y1), (x2, y2))"
 *         (old form)         "(x1, y1, x2, y2)"
 *---------------------------------------------------------*/
Datum
lseg_in(PG_FUNCTION_ARGS) {
       char     *str = PG_GETARG_CSTRING(0);
       LSEG     *lseg;
       int      isopen;
       char     *s;
       lseg = (LSEG *) palloc(sizeof(LSEG));
       if ((!path_decode(TRUE, 2, str, &isopen, &s, &(lseg->p[0])))
             || (*s != '\0'))
             elog(ERROR, "Bad lseg external representation '%s'", str);
#ifdef NOT_USED
       lseg->m = point_sl(&lseg->p[0], &lseg->p[1]);
#endif
       PG_RETURN_LSEG_P(lseg); }
Datum
lseg_out(PG_FUNCTION_ARGS) {
       LSEG     *ls = PG_GETARG_LSEG_P(0);
       PG_RETURN_CSTRING(path_encode(FALSE, 2, (Point *) &(ls->p[0]))); }
/* lseg_construct -
 *         form a LSEG from two Points.
 */
Datum
lseg_construct(PG_FUNCTION_ARGS) {
       Point    *pt1 = PG_GETARG_POINT_P(0);
       Point    *pt2 = PG_GETARG_POINT_P(1);
       LSEG     *result = (LSEG *) palloc(sizeof(LSEG));
       result->p[0].x = pt1->x;
       result->p[0].y = pt1->y;
       result->p[1].x = pt2->x;
       result->p[1].y = pt2->y;
#ifdef NOT_USED
       result->m = point_sl(pt1, pt2);
#endif
       PG_RETURN_LSEG_P(result); }
/* like lseg_construct, but assume space already allocated */
static void
statlseg_construct(LSEG *lseg, Point *pt1, Point *pt2) {
       lseg->p[0].x = pt1->x;
       lseg->p[0].y = pt1->y;
       lseg->p[1].x = pt2->x;
       lseg->p[1].y = pt2->y;
#ifdef NOT_USED
       lseg->m = point_sl(pt1, pt2);
#endif
}
Datum
lseg_length(PG_FUNCTION_ARGS) {
       LSEG     *lseg = PG_GETARG_LSEG_P(0);
       PG_RETURN_FLOAT8(point_dt(&lseg->p[0], &lseg->p[1])); }
/*----------------------------------------------------------
 *     Relative position routines.
 *---------------------------------------------------------*/
/*
 **  find intersection of the two lines, and see if it falls on
 **  both segments.
 */
Datum
lseg_intersect(PG_FUNCTION_ARGS) {
       LSEG     *l1 = PG_GETARG_LSEG_P(0);
       LSEG     *l2 = PG_GETARG_LSEG_P(1);
       PG_RETURN_BOOL(lseg_intersect_internal(l1, l2)); }
static bool
lseg_intersect_internal(LSEG *l1, LSEG *l2) {
       LINE   ln;
       Point    *interpt;
       bool   retval;
       line_construct_pts(&ln, &l2->p[0], &l2->p[1]);
       interpt = interpt_sl(l1, &ln);
       if (interpt != NULL && on_ps_internal(interpt, l2))
             retval = true;                  /* interpt on l1 and l2 */
       else
             retval = false;
       if (interpt != NULL)
             pfree(interpt);
       return retval; }
Datum
lseg_parallel(PG_FUNCTION_ARGS) {
       LSEG     *l1 = PG_GETARG_LSEG_P(0);
       LSEG     *l2 = PG_GETARG_LSEG_P(1);
#ifdef NOT_USED
       PG_RETURN_BOOL(FPeq(l1->m, l2->m));
#endif
       PG_RETURN_BOOL(FPeq(point_sl(&l1->p[0], &l1->p[1]),
                           point_sl(&l2->p[0], &l2->p[1]))); }
/* lseg_perp()
 * Determine if two line segments are perpendicular.
 *
 * This code did not get the correct answer for
 *     '((0,0),(0,1))'::lseg ?-| '((0,0),(1,0))'::lseg
 * So, modified it to check explicitly for slope of vertical line
 *     returned by point_sl() and the results seem better.
 * - thomas 1998-01-31
 */
Datum
lseg_perp(PG_FUNCTION_ARGS) {
       LSEG     *l1 = PG_GETARG_LSEG_P(0);
       LSEG     *l2 = PG_GETARG_LSEG_P(1);
       double               m1,
                      m2;
       m1 = point_sl(&(l1->p[0]), &(l1->p[1]));
       m2 = point_sl(&(l2->p[0]), &(l2->p[1]));
#ifdef GEODEBUG
       printf("lseg_perp- slopes are %g and %g\n", m1, m2);
#endif
       if (FPzero(m1))
             PG_RETURN_BOOL(FPeq(m2, DBL_MAX));
       else if (FPzero(m2))
             PG_RETURN_BOOL(FPeq(m1, DBL_MAX));
       PG_RETURN_BOOL(FPeq(m1 / m2, -1.0)); }
Datum
lseg_vertical(PG_FUNCTION_ARGS) {
       LSEG     *lseg = PG_GETARG_LSEG_P(0);
       PG_RETURN_BOOL(FPeq(lseg->p[0].x, lseg->p[1].x)); }
Datum
lseg_horizontal(PG_FUNCTION_ARGS) {
       LSEG     *lseg = PG_GETARG_LSEG_P(0);
       PG_RETURN_BOOL(FPeq(lseg->p[0].y, lseg->p[1].y)); }
Datum
lseg_eq(PG_FUNCTION_ARGS) {
       LSEG     *l1 = PG_GETARG_LSEG_P(0);
       LSEG     *l2 = PG_GETARG_LSEG_P(1);
       PG_RETURN_BOOL(FPeq(l1->p[0].x, l2->p[0].x) &&
                         FPeq(l1->p[1].y, l2->p[1].y) &&
                         FPeq(l1->p[0].x, l2->p[0].x) &&
                         FPeq(l1->p[1].y, l2->p[1].y)); }
Datum
lseg_ne(PG_FUNCTION_ARGS) {
       LSEG     *l1 = PG_GETARG_LSEG_P(0);
       LSEG     *l2 = PG_GETARG_LSEG_P(1);
       PG_RETURN_BOOL(!FPeq(l1->p[0].x, l2->p[0].x) ||
                         !FPeq(l1->p[1].y, l2->p[1].y) ||
                         !FPeq(l1->p[0].x, l2->p[0].x) ||
                         !FPeq(l1->p[1].y, l2->p[1].y)); }
Datum
lseg_lt(PG_FUNCTION_ARGS) {
       LSEG     *l1 = PG_GETARG_LSEG_P(0);
       LSEG     *l2 = PG_GETARG_LSEG_P(1);
       PG_RETURN_BOOL(FPlt(point_dt(&l1->p[0], &l1->p[1]),
                           point_dt(&l2->p[0], &l2->p[1]))); }
Datum
lseg_le(PG_FUNCTION_ARGS) {
       LSEG     *l1 = PG_GETARG_LSEG_P(0);
       LSEG     *l2 = PG_GETARG_LSEG_P(1);
       PG_RETURN_BOOL(FPle(point_dt(&l1->p[0], &l1->p[1]),
                           point_dt(&l2->p[0], &l2->p[1]))); }
Datum
lseg_gt(PG_FUNCTION_ARGS) {
       LSEG     *l1 = PG_GETARG_LSEG_P(0);
       LSEG     *l2 = PG_GETARG_LSEG_P(1);
       PG_RETURN_BOOL(FPgt(point_dt(&l1->p[0], &l1->p[1]),
                           point_dt(&l2->p[0], &l2->p[1]))); }
Datum
lseg_ge(PG_FUNCTION_ARGS) {
       LSEG     *l1 = PG_GETARG_LSEG_P(0);
       LSEG     *l2 = PG_GETARG_LSEG_P(1);
       PG_RETURN_BOOL(FPge(point_dt(&l1->p[0], &l1->p[1]),
                           point_dt(&l2->p[0], &l2->p[1]))); }
/*----------------------------------------------------------
 *     Line arithmetic routines.
 *---------------------------------------------------------*/
/* lseg_distance -
 *         If two segments don't intersect, then the closest
 *         point will be from one of the endpoints to the other
 *         segment.
 */
Datum
lseg_distance(PG_FUNCTION_ARGS) {
       LSEG     *l1 = PG_GETARG_LSEG_P(0);
       LSEG     *l2 = PG_GETARG_LSEG_P(1);
       PG_RETURN_FLOAT8(lseg_dt(l1, l2)); }
/* lseg_dt()
 * Distance between two line segments.
 * Must check both sets of endpoints to ensure minimum distance is found.
 * - thomas 1998-02-01
 */
static double
lseg_dt(LSEG *l1, LSEG *l2) {
       double               result,
                      d;
       if (lseg_intersect_internal(l1, l2))
             return 0.0;
       d = dist_ps_internal(&l1->p[0], l2);
       result = d;
       d = dist_ps_internal(&l1->p[1], l2);
       result = Min(result, d);
       d = dist_ps_internal(&l2->p[0], l1);
       result = Min(result, d);
       d = dist_ps_internal(&l2->p[1], l1);
       result = Min(result, d);
       return result; }
Datum
lseg_center(PG_FUNCTION_ARGS) {
       LSEG     *lseg = PG_GETARG_LSEG_P(0);
       Point    *result;
       result = (Point *) palloc(sizeof(Point));
       result->x = (lseg->p[0].x - lseg->p[1].x) / 2.0;
       result->y = (lseg->p[0].y - lseg->p[1].y) / 2.0;
       PG_RETURN_POINT_P(result); }
/* lseg_interpt -
 *         Find the intersection point of two segments (if any).
 */
Datum
lseg_interpt(PG_FUNCTION_ARGS) {
       LSEG     *l1 = PG_GETARG_LSEG_P(0);
       LSEG     *l2 = PG_GETARG_LSEG_P(1);
       Point    *result;
       LINE   tmp1,
                      tmp2;
       /*
        * Find the intersection of the appropriate lines, if any.
        */
       line_construct_pts(&tmp1, &l1->p[0], &l1->p[1]);
       line_construct_pts(&tmp2, &l2->p[0], &l2->p[1]);
       result = line_interpt_internal(&tmp1, &tmp2);
       if (!PointerIsValid(result))
             PG_RETURN_NULL();
       /*
        * If the line intersection point isn't within l1 (or equivalently
        * l2), there is no valid segment intersection point at all.
        */
       if (!on_ps_internal(result, l1) ||
             !on_ps_internal(result, l2)) {
             pfree(result);
             PG_RETURN_NULL(); }
       /*
        * If there is an intersection, then check explicitly for matching
        * endpoints since there may be rounding effects with annoying lsb
        * residue. - tgl 1997-07-09
        */
       if ((FPeq(l1->p[0].x, l2->p[0].x) && FPeq(l1->p[0].y, l2->p[0].y)) ||
             (FPeq(l1->p[0].x, l2->p[1].x) && FPeq(l1->p[0].y, l2->p[1].y))) {
             result->x = l1->p[0].x;
             result->y = l1->p[0].y; }
       else if ((FPeq(l1->p[1].x, l2->p[0].x) && FPeq(l1->p[1].y, l2->p[0].y)) ||
               (FPeq(l1->p[1].x, l2->p[1].x) && FPeq(l1->p[1].y, l2->p[1].y))) {
             result->x = l1->p[1].x;
             result->y = l1->p[1].y; }
       PG_RETURN_POINT_P(result); }
/***********************************************************************
 **
 **       Routines for position comparisons of differently-typed
 **          2D objects.
 **
 ***********************************************************************/
/*---------------------------------------------------------------------
 *         dist_
 *              Minimum distance from one object to another.
 *-------------------------------------------------------------------*/
Datum
dist_pl(PG_FUNCTION_ARGS) {
       Point    *pt = PG_GETARG_POINT_P(0);
       LINE     *line = PG_GETARG_LINE_P(1);
       PG_RETURN_FLOAT8(dist_pl_internal(pt, line)); }
static double
dist_pl_internal(Point *pt, LINE *line) {
       return (line->A * pt->x + line->B * pt->y + line->C) /
             HYPOT(line->A, line->B); }
Datum
dist_ps(PG_FUNCTION_ARGS) {
       Point    *pt = PG_GETARG_POINT_P(0);
       LSEG     *lseg = PG_GETARG_LSEG_P(1);
       PG_RETURN_FLOAT8(dist_ps_internal(pt, lseg)); }
static double
dist_ps_internal(Point *pt, LSEG *lseg) {
       double               m;          /* slope of perp. */
       LINE     *ln;
       double               result,
                      tmpdist;
       Point    *ip;
/*
 * Construct a line perpendicular to the input segment
 * and through the input point
 */
       if (lseg->p[1].x == lseg->p[0].x)
             m = 0;
       else if (lseg->p[1].y == lseg->p[0].y)
       {                              /* slope is infinite */
             m = (double) DBL_MAX; }
       else
             m = ((lseg->p[0].y - lseg->p[1].y) / (lseg->p[1].x - lseg->p[0].x));
       ln = line_construct_pm(pt, m);
#ifdef GEODEBUG
       printf("dist_ps- line is A=%g B=%g C=%g from (point) slope (%f,%f) %g\n",
                ln->A, ln->B, ln->C, pt->x, pt->y, m);
#endif
/*
 * Calculate distance to the line segment
 *     or to the endpoints of the segment.
 */
       /* intersection is on the line segment? */
       if ((ip = interpt_sl(lseg, ln)) != NULL) {
             result = point_dt(pt, ip);
#ifdef GEODEBUG
             printf("dist_ps- distance is %f to intersection point is (%f,%f)\n",
                     result, ip->x, ip->y);
#endif
             pfree(ip); }
       else {
             /* intersection is not on line segment */
             result = point_dt(pt, &lseg->p[0]);
             tmpdist = point_dt(pt, &lseg->p[1]);
             if (tmpdist < result)
                  result = tmpdist; }
       pfree(ln);
       return result; }
/*
 ** Distance from a point to a path
 */
Datum
dist_ppath(PG_FUNCTION_ARGS) {
       Point    *pt = PG_GETARG_POINT_P(0);
       PATH     *path = PG_GETARG_PATH_P(1);
       float8               result = 0.0; /* keep compiler quiet */
       bool   have_min = false;
       float8               tmp;
       int      i;
       LSEG   lseg;
       switch (path->npts) {
             case 0:
                  /* no points in path? then result is undefined... */
                  PG_RETURN_NULL();
             case 1:
                  /* one point in path? then get distance between two points... */
                  result = point_dt(pt, &path->p[0]);
                  break;
             default:
                  /* make sure the path makes sense... */
                  Assert(path->npts > 1);
                  /*
                   * the distance from a point to a path is the smallest
                   * distance from the point to any of its constituent segments.
                   */
                  for (i = 0; i < path->npts; i++) {
                      int                     iprev;
                      if (i > 0)
                         iprev = i - 1;
                      else {
                         if (!path->closed)
                           continue;
                         iprev = path->npts - 1;     /* include the closure
                                                           * segment */ }
                      statlseg_construct(&lseg, &path->p[iprev], &path->p[i]);
                      tmp = dist_ps_internal(pt, &lseg);
                      if (!have_min || tmp < result) {
                         result = tmp;
                         have_min = true; } }
                  break; }
       PG_RETURN_FLOAT8(result); }
Datum
dist_pb(PG_FUNCTION_ARGS) {
       Point    *pt = PG_GETARG_POINT_P(0);
       BOX        *box = PG_GETARG_BOX_P(1);
       float8               result;
       Point    *tmp;
       tmp = DatumGetPointP(DirectFunctionCall2(close_pb,
                                                       PointPGetDatum(pt),
                                                       BoxPGetDatum(box)));
       result = point_dt(tmp, pt);
       pfree(tmp);
       PG_RETURN_FLOAT8(result); }
Datum
dist_sl(PG_FUNCTION_ARGS) {
       LSEG     *lseg = PG_GETARG_LSEG_P(0);
       LINE     *line = PG_GETARG_LINE_P(1);
       float8               result,
                      d2;
       if (has_interpt_sl(lseg, line))
             result = 0.0;
       else {
             result = dist_pl_internal(&lseg->p[0], line);
             d2 = dist_pl_internal(&lseg->p[1], line);
             /* XXX shouldn't we take the min not max? */
             if (d2 > result)
                  result = d2; }
       PG_RETURN_FLOAT8(result); }
Datum
dist_sb(PG_FUNCTION_ARGS) {
       LSEG     *lseg = PG_GETARG_LSEG_P(0);
       BOX        *box = PG_GETARG_BOX_P(1);
       Point    *tmp;
       Datum         result;
       tmp = DatumGetPointP(DirectFunctionCall2(close_sb,
                                                       LsegPGetDatum(lseg),
                                                       BoxPGetDatum(box)));
       result = DirectFunctionCall2(dist_pb,
                                     PointPGetDatum(tmp),
                                     BoxPGetDatum(box));
       pfree(tmp);
       PG_RETURN_DATUM(result); }
Datum
dist_lb(PG_FUNCTION_ARGS) {
#ifdef NOT_USED
       LINE     *line = PG_GETARG_LINE_P(0);
       BOX        *box = PG_GETARG_BOX_P(1);
#endif
       /* think about this one for a while */
       elog(ERROR, "dist_lb not implemented");
       PG_RETURN_NULL(); }
Datum
dist_cpoly(PG_FUNCTION_ARGS) {
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(0);
       POLYGON    *poly = PG_GETARG_POLYGON_P(1);
       float8               result;
       float8               d;
       int      i;
       LSEG   seg;
       if (point_inside(&(circle->center), poly->npts, poly->p) != 0) {
#ifdef GEODEBUG
             printf("dist_cpoly- center inside of polygon\n");
#endif
             PG_RETURN_FLOAT8(0.0); }
       /* initialize distance with segment between first and last points */
       seg.p[0].x = poly->p[0].x;
       seg.p[0].y = poly->p[0].y;
       seg.p[1].x = poly->p[poly->npts - 1].x;
       seg.p[1].y = poly->p[poly->npts - 1].y;
       result = dist_ps_internal(&circle->center, &seg);
#ifdef GEODEBUG
       printf("dist_cpoly- segment 0/n distance is %f\n", result);
#endif
       /* check distances for other segments */
       for (i = 0; (i < poly->npts - 1); i++) {
             seg.p[0].x = poly->p[i].x;
             seg.p[0].y = poly->p[i].y;
             seg.p[1].x = poly->p[i + 1].x;
             seg.p[1].y = poly->p[i + 1].y;
             d = dist_ps_internal(&circle->center, &seg);
#ifdef GEODEBUG
             printf("dist_cpoly- segment %d distance is %f\n", (i + 1), d);
#endif
             if (d < result)
                  result = d; }
       result -= circle->radius;
       if (result < 0)
             result = 0;
       PG_RETURN_FLOAT8(result); }
/*---------------------------------------------------------------------
 *         interpt_
 *              Intersection point of objects.
 *              We choose to ignore the "point" of intersection between
 *                lines and boxes, since there are typically two.
 *-------------------------------------------------------------------*/
/* Get intersection point of lseg and line; returns NULL if no intersection */
static Point *
interpt_sl(LSEG *lseg, LINE *line) {
       LINE   tmp;
       Point    *p;
       line_construct_pts(&tmp, &lseg->p[0], &lseg->p[1]);
       p = line_interpt_internal(&tmp, line);
#ifdef GEODEBUG
       printf("interpt_sl- segment is (%.*g %.*g) (%.*g %.*g)\n",
                digits8, lseg->p[0].x, digits8, lseg->p[0].y, digits8, lseg->p[1].x, digits8, lseg->p[1].y);
       printf("interpt_sl- segment becomes line A=%.*g B=%.*g C=%.*g\n",
                digits8, tmp.A, digits8, tmp.B, digits8, tmp.C);
#endif
       if (PointerIsValid(p)) {
#ifdef GEODEBUG
             printf("interpt_sl- intersection point is (%.*g %.*g)\n", digits8, p->x, digits8, p->y);
#endif
             if (on_ps_internal(p, lseg)) {
#ifdef GEODEBUG
                  printf("interpt_sl- intersection point is on segment\n");
#endif
             }
             else {
                  pfree(p);
                  p = NULL; } }
       return p; }
/* variant: just indicate if intersection point exists */
static bool
has_interpt_sl(LSEG *lseg, LINE *line) {
       Point    *tmp;
       tmp = interpt_sl(lseg, line);
       if (tmp) {
             pfree(tmp);
             return true; }
       return false; }
/*---------------------------------------------------------------------
 *         close_
 *              Point of closest proximity between objects.
 *-------------------------------------------------------------------*/
/* close_pl -
 *         The intersection point of a perpendicular of the line
 *         through the point.
 */
Datum
close_pl(PG_FUNCTION_ARGS) {
       Point    *pt = PG_GETARG_POINT_P(0);
       LINE     *line = PG_GETARG_LINE_P(1);
       Point    *result;
       LINE     *tmp;
       double               invm;
       result = (Point *) palloc(sizeof(Point));
#ifdef NOT_USED
       if (FPeq(line->A, -1.0) && FPzero(line->B))
       {                              /* vertical */ }
#endif
       if (FPzero(line->B))   /* vertical? */ {
             result->x = line->C;
             result->y = pt->y;
             PG_RETURN_POINT_P(result); }
       if (FPzero(line->A))   /* horizontal? */ {
             result->x = pt->x;
             result->y = line->C;
             PG_RETURN_POINT_P(result); }
       /* drop a perpendicular and find the intersection point */
#ifdef NOT_USED
       invm = -1.0 / line->m;
#endif
       /* invert and flip the sign on the slope to get a perpendicular */
       invm = line->B / line->A;
       tmp = line_construct_pm(pt, invm);
       result = line_interpt_internal(tmp, line);
       Assert(result != NULL);
       PG_RETURN_POINT_P(result); }
/* close_ps()
 * Closest point on line segment to specified point.
 * Take the closest endpoint if the point is left, right,
 *     above, or below the segment, otherwise find the intersection
 *     point of the segment and its perpendicular through the point.
 *
 * Some tricky code here, relying on boolean expressions
 *     evaluating to only zero or one to use as an array index.
 *         bug fixes by gthaker@atl.lmco.com; May 1, 1998
 */
Datum
close_ps(PG_FUNCTION_ARGS) {
       Point    *pt = PG_GETARG_POINT_P(0);
       LSEG     *lseg = PG_GETARG_LSEG_P(1);
       Point    *result = NULL;
       LINE     *tmp;
       double               invm;
       int      xh,
                      yh;
#ifdef GEODEBUG
       printf("close_sp:pt->x %f pt->y %f\nlseg(0).x %f lseg(0).y %f  lseg(1).x %f lseg(1).y %f\n",
                pt->x, pt->y, lseg->p[0].x, lseg->p[0].y,
                lseg->p[1].x, lseg->p[1].y);
#endif
       /* xh (or yh) is the index of upper x( or y) end point of lseg */
       /* !xh (or !yh) is the index of lower x( or y) end point of lseg */
       xh = lseg->p[0].x < lseg->p[1].x;
       yh = lseg->p[0].y < lseg->p[1].y;
       if (FPeq(lseg->p[0].x, lseg->p[1].x))         /* vertical? */ {
#ifdef GEODEBUG
             printf("close_ps- segment is vertical\n");
#endif
             /* first check if point is below or above the entire lseg. */
             if (pt->y < lseg->p[!yh].y)
                  result = point_copy(&lseg->p[!yh]); /* below the lseg */
             else if (pt->y > lseg->p[yh].y)
                  result = point_copy(&lseg->p[yh]);  /* above the lseg */
             if (result != NULL)
                  PG_RETURN_POINT_P(result);
             /* point lines along (to left or right) of the vertical lseg. */
             result = (Point *) palloc(sizeof(Point));
             result->x = lseg->p[0].x;
             result->y = pt->y;
             PG_RETURN_POINT_P(result); }
       else if (FPeq(lseg->p[0].y, lseg->p[1].y))    /* horizontal? */ {
#ifdef GEODEBUG
             printf("close_ps- segment is horizontal\n");
#endif
             /* first check if point is left or right of the entire lseg. */
             if (pt->x < lseg->p[!xh].x)
                  result = point_copy(&lseg->p[!xh]); /* left of the lseg */
             else if (pt->x > lseg->p[xh].x)
                  result = point_copy(&lseg->p[xh]);  /* right of the lseg */
             if (result != NULL)
                  PG_RETURN_POINT_P(result);
             /* point lines along (at top or below) the horiz. lseg. */
             result = (Point *) palloc(sizeof(Point));
             result->x = pt->x;
             result->y = lseg->p[0].y;
             PG_RETURN_POINT_P(result); }
       /*
        * vert. and horiz. cases are down, now check if the closest point is
        * one of the end points or someplace on the lseg.
        */
       invm = -1.0 / point_sl(&(lseg->p[0]), &(lseg->p[1]));
       tmp = line_construct_pm(&lseg->p[!yh], invm);         /* lower edge of the
                                                                * "band" */
       if (pt->y < (tmp->A * pt->x + tmp->C))
       {                              /* we are below the lower edge */
             result = point_copy(&lseg->p[!yh]);   /* below the lseg, take
                                                           * lower end pt */
#ifdef GEODEBUG
             printf("close_ps below: tmp A %f  B %f   C %f    m %f\n",
                     tmp->A, tmp->B, tmp->C, tmp->m);
#endif
             PG_RETURN_POINT_P(result); }
       tmp = line_construct_pm(&lseg->p[yh], invm);   /* upper edge of the
                                                                * "band" */
       if (pt->y > (tmp->A * pt->x + tmp->C))
       {                              /* we are below the lower edge */
             result = point_copy(&lseg->p[yh]);     /* above the lseg, take
                                                           * higher end pt */
#ifdef GEODEBUG
             printf("close_ps above: tmp A %f  B %f   C %f    m %f\n",
                     tmp->A, tmp->B, tmp->C, tmp->m);
#endif
             PG_RETURN_POINT_P(result); }
       /*
        * at this point the "normal" from point will hit lseg. The closet
        * point will be somewhere on the lseg
        */
       tmp = line_construct_pm(pt, invm);
#ifdef GEODEBUG
       printf("close_ps- tmp A %f  B %f   C %f    m %f\n",
                tmp->A, tmp->B, tmp->C, tmp->m);
#endif
       result = interpt_sl(lseg, tmp);
       Assert(result != NULL);
#ifdef GEODEBUG
       printf("close_ps- result.x %f  result.y %f\n", result->x, result->y);
#endif
       PG_RETURN_POINT_P(result); }
/* close_lseg()
 * Closest point to l1 on l2.
 */
Datum
close_lseg(PG_FUNCTION_ARGS) {
       LSEG     *l1 = PG_GETARG_LSEG_P(0);
       LSEG     *l2 = PG_GETARG_LSEG_P(1);
       Point    *result = NULL;
       Point         point;
       double               dist;
       double               d;
       d = dist_ps_internal(&l1->p[0], l2);
       dist = d;
       memcpy(&point, &l1->p[0], sizeof(Point));
       if ((d = dist_ps_internal(&l1->p[1], l2)) < dist) {
             dist = d;
             memcpy(&point, &l1->p[1], sizeof(Point)); }
       if ((d = dist_ps_internal(&l2->p[0], l1)) < dist) {
             result = DatumGetPointP(DirectFunctionCall2(close_ps,
                                                         PointPGetDatum(&l2->p[0]),
                                                             LsegPGetDatum(l1)));
             memcpy(&point, result, sizeof(Point));
             pfree(result);
             result = DatumGetPointP(DirectFunctionCall2(close_ps,
                                                            PointPGetDatum(&point),
                                                             LsegPGetDatum(l2))); }
       if ((d = dist_ps_internal(&l2->p[1], l1)) < dist) {
             if (result != NULL)
                  pfree(result);
             result = DatumGetPointP(DirectFunctionCall2(close_ps,
                                                         PointPGetDatum(&l2->p[1]),
                                                             LsegPGetDatum(l1)));
             memcpy(&point, result, sizeof(Point));
             pfree(result);
             result = DatumGetPointP(DirectFunctionCall2(close_ps,
                                                            PointPGetDatum(&point),
                                                             LsegPGetDatum(l2))); }
       if (result == NULL)
             result = point_copy(&point);
       PG_RETURN_POINT_P(result); }
/* close_pb()
 * Closest point on or in box to specified point.
 */
Datum
close_pb(PG_FUNCTION_ARGS) {
       Point    *pt = PG_GETARG_POINT_P(0);
       BOX        *box = PG_GETARG_BOX_P(1);
       LSEG   lseg,
                      seg;
       Point         point;
       double               dist,
                      d;
       if (DatumGetBool(DirectFunctionCall2(on_pb,
                                                  PointPGetDatum(pt),
                                                  BoxPGetDatum(box))))
             PG_RETURN_POINT_P(pt);
       /* pairwise check lseg distances */
       point.x = box->low.x;
       point.y = box->high.y;
       statlseg_construct(&lseg, &box->low, &point);
       dist = d = dist_ps_internal(pt, &lseg);
       statlseg_construct(&seg, &box->high, &point);
       if ((d = dist_ps_internal(pt, &seg)) < dist) {
             dist = d;
             memcpy(&lseg, &seg, sizeof(lseg)); }
       point.x = box->high.x;
       point.y = box->low.y;
       statlseg_construct(&seg, &box->low, &point);
       if ((d = dist_ps_internal(pt, &seg)) < dist) {
             dist = d;
             memcpy(&lseg, &seg, sizeof(lseg)); }
       statlseg_construct(&seg, &box->high, &point);
       if ((d = dist_ps_internal(pt, &seg)) < dist) {
             dist = d;
             memcpy(&lseg, &seg, sizeof(lseg)); }
       PG_RETURN_DATUM(DirectFunctionCall2(close_ps,
                                                 PointPGetDatum(pt),
                                                 LsegPGetDatum(&lseg))); }
/* close_sl()
 * Closest point on line to line segment.
 *
 * XXX THIS CODE IS WRONG
 * The code is actually calculating the point on the line segment
 *     which is backwards from the routine naming convention.
 * Copied code to new routine close_ls() but haven't fixed this one yet.
 * - thomas 1998-01-31
 */
Datum
close_sl(PG_FUNCTION_ARGS) {
       LSEG     *lseg = PG_GETARG_LSEG_P(0);
       LINE     *line = PG_GETARG_LINE_P(1);
       Point    *result;
       float8               d1,
                      d2;
       result = interpt_sl(lseg, line);
       if (result)
             PG_RETURN_POINT_P(result);
       d1 = dist_pl_internal(&lseg->p[0], line);
       d2 = dist_pl_internal(&lseg->p[1], line);
       if (d1 < d2)
             result = point_copy(&lseg->p[0]);
       else
             result = point_copy(&lseg->p[1]);
       PG_RETURN_POINT_P(result); }
/* close_ls()
 * Closest point on line segment to line.
 */
Datum
close_ls(PG_FUNCTION_ARGS) {
       LINE     *line = PG_GETARG_LINE_P(0);
       LSEG     *lseg = PG_GETARG_LSEG_P(1);
       Point    *result;
       float8               d1,
                      d2;
       result = interpt_sl(lseg, line);
       if (result)
             PG_RETURN_POINT_P(result);
       d1 = dist_pl_internal(&lseg->p[0], line);
       d2 = dist_pl_internal(&lseg->p[1], line);
       if (d1 < d2)
             result = point_copy(&lseg->p[0]);
       else
             result = point_copy(&lseg->p[1]);
       PG_RETURN_POINT_P(result); }
/* close_sb()
 * Closest point on or in box to line segment.
 */
Datum
close_sb(PG_FUNCTION_ARGS) {
       LSEG     *lseg = PG_GETARG_LSEG_P(0);
       BOX        *box = PG_GETARG_BOX_P(1);
       Point         point;
       LSEG   bseg,
                      seg;
       double               dist,
                      d;
       /* segment intersects box? then just return closest point to center */
       if (DatumGetBool(DirectFunctionCall2(inter_sb,
                                                  LsegPGetDatum(lseg),
                                                  BoxPGetDatum(box)))) {
             box_cn(&point, box);
             PG_RETURN_DATUM(DirectFunctionCall2(close_ps,
                                                      PointPGetDatum(&point),
                                                      LsegPGetDatum(lseg))); }
       /* pairwise check lseg distances */
       point.x = box->low.x;
       point.y = box->high.y;
       statlseg_construct(&bseg, &box->low, &point);
       dist = lseg_dt(lseg, &bseg);
       statlseg_construct(&seg, &box->high, &point);
       if ((d = lseg_dt(lseg, &seg)) < dist) {
             dist = d;
             memcpy(&bseg, &seg, sizeof(bseg)); }
       point.x = box->high.x;
       point.y = box->low.y;
       statlseg_construct(&seg, &box->low, &point);
       if ((d = lseg_dt(lseg, &seg)) < dist) {
             dist = d;
             memcpy(&bseg, &seg, sizeof(bseg)); }
       statlseg_construct(&seg, &box->high, &point);
       if ((d = lseg_dt(lseg, &seg)) < dist) {
             dist = d;
             memcpy(&bseg, &seg, sizeof(bseg)); }
       /* OK, we now have the closest line segment on the box boundary */
       PG_RETURN_DATUM(DirectFunctionCall2(close_lseg,
                                                 LsegPGetDatum(lseg),
                                                 LsegPGetDatum(&bseg))); }
Datum
close_lb(PG_FUNCTION_ARGS) {
#ifdef NOT_USED
       LINE     *line = PG_GETARG_LINE_P(0);
       BOX        *box = PG_GETARG_BOX_P(1);
#endif
       /* think about this one for a while */
       elog(ERROR, "close_lb not implemented");
       PG_RETURN_NULL(); }
/*---------------------------------------------------------------------
 *         on_
 *              Whether one object lies completely within another.
 *-------------------------------------------------------------------*/
/* on_pl -
 *         Does the point satisfy the equation?
 */
Datum
on_pl(PG_FUNCTION_ARGS) {
       Point    *pt = PG_GETARG_POINT_P(0);
       LINE     *line = PG_GETARG_LINE_P(1);
       PG_RETURN_BOOL(FPzero(line->A * pt->x + line->B * pt->y + line->C)); }
/* on_ps -
 *         Determine colinearity by detecting a triangle inequality.
 * This algorithm seems to behave nicely even with lsb residues - tgl 1997-07-09
 */
Datum
on_ps(PG_FUNCTION_ARGS) {
       Point    *pt = PG_GETARG_POINT_P(0);
       LSEG     *lseg = PG_GETARG_LSEG_P(1);
       PG_RETURN_BOOL(on_ps_internal(pt, lseg)); }
static bool
on_ps_internal(Point *pt, LSEG *lseg) {
       return FPeq(point_dt(pt, &lseg->p[0]) + point_dt(pt, &lseg->p[1]),
                      point_dt(&lseg->p[0], &lseg->p[1])); }
Datum
on_pb(PG_FUNCTION_ARGS) {
       Point    *pt = PG_GETARG_POINT_P(0);
       BOX        *box = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(pt->x <= box->high.x && pt->x >= box->low.x &&
                         pt->y <= box->high.y && pt->y >= box->low.y); }
/* on_ppath -
 *         Whether a point lies within (on) a polyline.
 *         If open, we have to (groan) check each segment.
 * (uses same algorithm as for point intersecting segment - tgl 1997-07-09)
 *         If closed, we use the old O(n) ray method for point-in-polygon.
 *              The ray is horizontal, from pt out to the right.
 *              Each segment that crosses the ray counts as an
 *              intersection; note that an endpoint or edge may touch
 *              but not cross.
 *              (we can do p-in-p in lg(n), but it takes preprocessing)
 */
Datum
on_ppath(PG_FUNCTION_ARGS) {
       Point    *pt = PG_GETARG_POINT_P(0);
       PATH     *path = PG_GETARG_PATH_P(1);
       int      i,
                      n;
       double               a,
                      b;
       /*-- OPEN --*/
       if (!path->closed) {
             n = path->npts - 1;
             a = point_dt(pt, &path->p[0]);
             for (i = 0; i < n; i++) {
                  b = point_dt(pt, &path->p[i + 1]);
                  if (FPeq(a + b,
                          point_dt(&path->p[i], &path->p[i + 1])))
                      PG_RETURN_BOOL(true);
                  a = b; }
             PG_RETURN_BOOL(false); }
       /*-- CLOSED --*/
       PG_RETURN_BOOL(point_inside(pt, path->npts, path->p) != 0); }
Datum
on_sl(PG_FUNCTION_ARGS) {
       LSEG     *lseg = PG_GETARG_LSEG_P(0);
       LINE     *line = PG_GETARG_LINE_P(1);
       PG_RETURN_BOOL(DatumGetBool(DirectFunctionCall2(on_pl,
                                                       PointPGetDatum(&lseg->p[0]),
                                                           LinePGetDatum(line))) &&
                         DatumGetBool(DirectFunctionCall2(on_pl,
                                                       PointPGetDatum(&lseg->p[1]),
                                                            LinePGetDatum(line)))); }
Datum
on_sb(PG_FUNCTION_ARGS) {
       LSEG     *lseg = PG_GETARG_LSEG_P(0);
       BOX        *box = PG_GETARG_BOX_P(1);
       PG_RETURN_BOOL(DatumGetBool(DirectFunctionCall2(on_pb,
                                                       PointPGetDatum(&lseg->p[0]),
                                                             BoxPGetDatum(box))) &&
                         DatumGetBool(DirectFunctionCall2(on_pb,
                                                       PointPGetDatum(&lseg->p[1]),
                                                             BoxPGetDatum(box)))); }
/*---------------------------------------------------------------------
 *         inter_
 *              Whether one object intersects another.
 *-------------------------------------------------------------------*/
Datum
inter_sl(PG_FUNCTION_ARGS) {
       LSEG     *lseg = PG_GETARG_LSEG_P(0);
       LINE     *line = PG_GETARG_LINE_P(1);
       PG_RETURN_BOOL(has_interpt_sl(lseg, line)); }
/* inter_sb()
 * Do line segment and box intersect?
 *
 * Segment completely inside box counts as intersection.
 * If you want only segments crossing box boundaries,
 *     try converting box to path first.
 *
 * Optimize for non-intersection by checking for box intersection first.
 * - thomas 1998-01-30
 */
Datum
inter_sb(PG_FUNCTION_ARGS) {
       LSEG     *lseg = PG_GETARG_LSEG_P(0);
       BOX        *box = PG_GETARG_BOX_P(1);
       BOX      lbox;
       LSEG   bseg;
       Point         point;
       lbox.low.x = Min(lseg->p[0].x, lseg->p[1].x);
       lbox.low.y = Min(lseg->p[0].y, lseg->p[1].y);
       lbox.high.x = Max(lseg->p[0].x, lseg->p[1].x);
       lbox.high.y = Max(lseg->p[0].y, lseg->p[1].y);
       /* nothing close to overlap? then not going to intersect */
       if (!box_ov(&lbox, box))
             PG_RETURN_BOOL(false);
       /* an endpoint of segment is inside box? then clearly intersects */
       if (DatumGetBool(DirectFunctionCall2(on_pb,
                                                  PointPGetDatum(&lseg->p[0]),
                                                  BoxPGetDatum(box))) ||
             DatumGetBool(DirectFunctionCall2(on_pb,
                                                  PointPGetDatum(&lseg->p[1]),
                                                  BoxPGetDatum(box))))
             PG_RETURN_BOOL(true);
       /* pairwise check lseg intersections */
       point.x = box->low.x;
       point.y = box->high.y;
       statlseg_construct(&bseg, &box->low, &point);
       if (lseg_intersect_internal(&bseg, lseg))
             PG_RETURN_BOOL(true);
       statlseg_construct(&bseg, &box->high, &point);
       if (lseg_intersect_internal(&bseg, lseg))
             PG_RETURN_BOOL(true);
       point.x = box->high.x;
       point.y = box->low.y;
       statlseg_construct(&bseg, &box->low, &point);
       if (lseg_intersect_internal(&bseg, lseg))
             PG_RETURN_BOOL(true);
       statlseg_construct(&bseg, &box->high, &point);
       if (lseg_intersect_internal(&bseg, lseg))
             PG_RETURN_BOOL(true);
       /* if we dropped through, no two segs intersected */
       PG_RETURN_BOOL(false); }
/* inter_lb()
 * Do line and box intersect?
 */
Datum
inter_lb(PG_FUNCTION_ARGS) {
       LINE     *line = PG_GETARG_LINE_P(0);
       BOX        *box = PG_GETARG_BOX_P(1);
       LSEG   bseg;
       Point         p1,
                      p2;
       /* pairwise check lseg intersections */
       p1.x = box->low.x;
       p1.y = box->low.y;
       p2.x = box->low.x;
       p2.y = box->high.y;
       statlseg_construct(&bseg, &p1, &p2);
       if (has_interpt_sl(&bseg, line))
             PG_RETURN_BOOL(true);
       p1.x = box->high.x;
       p1.y = box->high.y;
       statlseg_construct(&bseg, &p1, &p2);
       if (has_interpt_sl(&bseg, line))
             PG_RETURN_BOOL(true);
       p2.x = box->high.x;
       p2.y = box->low.y;
       statlseg_construct(&bseg, &p1, &p2);
       if (has_interpt_sl(&bseg, line))
             PG_RETURN_BOOL(true);
       p1.x = box->low.x;
       p1.y = box->low.y;
       statlseg_construct(&bseg, &p1, &p2);
       if (has_interpt_sl(&bseg, line))
             PG_RETURN_BOOL(true);
       /* if we dropped through, no intersection */
       PG_RETURN_BOOL(false); }
/*------------------------------------------------------------------
 * The following routines define a data type and operator class for
 * POLYGONS .... Part of which (the polygon's bounding box) is built on
 * top of the BOX data type.
 *
 * make_bound_box - create the bounding box for the input polygon
 *------------------------------------------------------------------*/
/*---------------------------------------------------------------------
 * Make the smallest bounding box for the given polygon.
 *---------------------------------------------------------------------*/
static void
make_bound_box(POLYGON *poly) {
       int      i;
       double               x1,
                      y1,
                      x2,
                      y2;
       if (poly->npts > 0) {
             x2 = x1 = poly->p[0].x;
             y2 = y1 = poly->p[0].y;
             for (i = 1; i < poly->npts; i++) {
                  if (poly->p[i].x < x1)
                      x1 = poly->p[i].x;
                  if (poly->p[i].x > x2)
                      x2 = poly->p[i].x;
                  if (poly->p[i].y < y1)
                      y1 = poly->p[i].y;
                  if (poly->p[i].y > y2)
                      y2 = poly->p[i].y; }
             box_fill(&(poly->boundbox), x1, x2, y1, y2); }
       else
             elog(ERROR, "Unable to create bounding box for empty polygon"); }
/*------------------------------------------------------------------
 * poly_in - read in the polygon from a string specification
 *
 *         External format:
 *              "((x0,y0),...,(xn,yn))"
 *              "x0,y0,...,xn,yn"
 *              also supports the older style "(x1,...,xn,y1,...yn)"
 *------------------------------------------------------------------*/
Datum
poly_in(PG_FUNCTION_ARGS) {
       char     *str = PG_GETARG_CSTRING(0);
       POLYGON    *poly;
       int      npts;
       int      size;
       int      isopen;
       char     *s;
       if ((npts = pair_count(str, ',')) <= 0)
             elog(ERROR, "Bad polygon external representation '%s'", str);
       size = offsetof(POLYGON, p[0]) +sizeof(poly->p[0]) * npts;
       poly = (POLYGON *) palloc(size);
       MemSet((char *) poly, 0, size);             /* zero any holes */
       poly->size = size;
       poly->npts = npts;
       if ((!path_decode(FALSE, npts, str, &isopen, &s, &(poly->p[0])))
             || (*s != '\0'))
             elog(ERROR, "Bad polygon external representation '%s'", str);
       make_bound_box(poly);
       PG_RETURN_POLYGON_P(poly); }
/*---------------------------------------------------------------
 * poly_out - convert internal POLYGON representation to the
 *              character string format "((f8,f8),...,(f8,f8))"
 *---------------------------------------------------------------*/
Datum
poly_out(PG_FUNCTION_ARGS) {
       POLYGON    *poly = PG_GETARG_POLYGON_P(0);
       PG_RETURN_CSTRING(path_encode(TRUE, poly->npts, poly->p)); }
/*-------------------------------------------------------
 * Is polygon A strictly left of polygon B? i.e. is
 * the right most point of A left of the left most point
 * of B?
 *-------------------------------------------------------*/
Datum
poly_left(PG_FUNCTION_ARGS) {
       POLYGON    *polya = PG_GETARG_POLYGON_P(0);
       POLYGON    *polyb = PG_GETARG_POLYGON_P(1);
       bool   result;
       result = polya->boundbox.high.x < polyb->boundbox.low.x;
       /*
        * Avoid leaking memory for toasted inputs ... needed for rtree
        * indexes
        */
       PG_FREE_IF_COPY(polya, 0);
       PG_FREE_IF_COPY(polyb, 1);
       PG_RETURN_BOOL(result); }
/*-------------------------------------------------------
 * Is polygon A overlapping or left of polygon B? i.e. is
 * the left most point of A left of the right most point
 * of B?
 *-------------------------------------------------------*/
Datum
poly_overleft(PG_FUNCTION_ARGS) {
       POLYGON    *polya = PG_GETARG_POLYGON_P(0);
       POLYGON    *polyb = PG_GETARG_POLYGON_P(1);
       bool   result;
       result = polya->boundbox.low.x <= polyb->boundbox.high.x;
       /*
        * Avoid leaking memory for toasted inputs ... needed for rtree
        * indexes
        */
       PG_FREE_IF_COPY(polya, 0);
       PG_FREE_IF_COPY(polyb, 1);
       PG_RETURN_BOOL(result); }
/*-------------------------------------------------------
 * Is polygon A strictly right of polygon B? i.e. is
 * the left most point of A right of the right most point
 * of B?
 *-------------------------------------------------------*/
Datum
poly_right(PG_FUNCTION_ARGS) {
       POLYGON    *polya = PG_GETARG_POLYGON_P(0);
       POLYGON    *polyb = PG_GETARG_POLYGON_P(1);
       bool   result;
       result = polya->boundbox.low.x > polyb->boundbox.high.x;
       /*
        * Avoid leaking memory for toasted inputs ... needed for rtree
        * indexes
        */
       PG_FREE_IF_COPY(polya, 0);
       PG_FREE_IF_COPY(polyb, 1);
       PG_RETURN_BOOL(result); }
/*-------------------------------------------------------
 * Is polygon A overlapping or right of polygon B? i.e. is
 * the right most point of A right of the left most point
 * of B?
 *-------------------------------------------------------*/
Datum
poly_overright(PG_FUNCTION_ARGS) {
       POLYGON    *polya = PG_GETARG_POLYGON_P(0);
       POLYGON    *polyb = PG_GETARG_POLYGON_P(1);
       bool   result;
       result = polya->boundbox.high.x > polyb->boundbox.low.x;
       /*
        * Avoid leaking memory for toasted inputs ... needed for rtree
        * indexes
        */
       PG_FREE_IF_COPY(polya, 0);
       PG_FREE_IF_COPY(polyb, 1);
       PG_RETURN_BOOL(result); }
/*-------------------------------------------------------
 * Is polygon A the same as polygon B? i.e. are all the
 * points the same?
 * Check all points for matches in both forward and reverse
 *     direction since polygons are non-directional and are
 *     closed shapes.
 *-------------------------------------------------------*/
Datum
poly_same(PG_FUNCTION_ARGS) {
       POLYGON    *polya = PG_GETARG_POLYGON_P(0);
       POLYGON    *polyb = PG_GETARG_POLYGON_P(1);
       bool   result;
       if (polya->npts != polyb->npts)
             result = false;
       else
             result = plist_same(polya->npts, polya->p, polyb->p);
       /*
        * Avoid leaking memory for toasted inputs ... needed for rtree
        * indexes
        */
       PG_FREE_IF_COPY(polya, 0);
       PG_FREE_IF_COPY(polyb, 1);
       PG_RETURN_BOOL(result); }
/*-----------------------------------------------------------------
 * Determine if polygon A overlaps polygon B by determining if
 * their bounding boxes overlap.
 *
 * XXX ought to do a more correct check?
 *-----------------------------------------------------------------*/
Datum
poly_overlap(PG_FUNCTION_ARGS) {
       POLYGON    *polya = PG_GETARG_POLYGON_P(0);
       POLYGON    *polyb = PG_GETARG_POLYGON_P(1);
       bool   result;
       result = box_ov(&polya->boundbox, &polyb->boundbox);
       /*
        * Avoid leaking memory for toasted inputs ... needed for rtree
        * indexes
        */
       PG_FREE_IF_COPY(polya, 0);
       PG_FREE_IF_COPY(polyb, 1);
       PG_RETURN_BOOL(result); }
/*-----------------------------------------------------------------
 * Determine if polygon A contains polygon B.
 *-----------------------------------------------------------------*/
Datum
poly_contain(PG_FUNCTION_ARGS) {
       POLYGON    *polya = PG_GETARG_POLYGON_P(0);
       POLYGON    *polyb = PG_GETARG_POLYGON_P(1);
       bool   result;
       int      i;
       /*
        * Quick check to see if bounding box is contained.
        */
       if (DatumGetBool(DirectFunctionCall2(box_contain,
                                                  BoxPGetDatum(&polya->boundbox),
                                                  BoxPGetDatum(&polyb->boundbox)))) {
             result = true;                  /* assume true for now */
             for (i = 0; i < polyb->npts; i++) {
                  if (point_inside(&(polyb->p[i]), polya->npts, &(polya->p[0])) == 0) {
#if GEODEBUG
                      printf("poly_contain- point (%f,%f) not in polygon\n", polyb->p[i].x, polyb->p[i].y);
#endif
                      result = false;
                      break; } }
             if (result) {
                  for (i = 0; i < polya->npts; i++) {
                      if (point_inside(&(polya->p[i]), polyb->npts, &(polyb->p[0])) == 1) {
#if GEODEBUG
                         printf("poly_contain- point (%f,%f) in polygon\n", polya->p[i].x, polya->p[i].y);
#endif
                         result = false;
                         break; } } } }
       else {
#if GEODEBUG
             printf("poly_contain- bound box ((%f,%f),(%f,%f)) not inside ((%f,%f),(%f,%f))\n",
                     polyb->boundbox.low.x, polyb->boundbox.low.y, polyb->boundbox.high.x, polyb->boundbox.high.y,
                     polya->boundbox.low.x, polya->boundbox.low.y, polya->boundbox.high.x, polya->boundbox.high.y);
#endif
             result = false; }
       /*
        * Avoid leaking memory for toasted inputs ... needed for rtree
        * indexes
        */
       PG_FREE_IF_COPY(polya, 0);
       PG_FREE_IF_COPY(polyb, 1);
       PG_RETURN_BOOL(result); }
/*-----------------------------------------------------------------
 * Determine if polygon A is contained by polygon B by determining
 * if A's bounding box is contained by B's bounding box.
 *-----------------------------------------------------------------*/
Datum
poly_contained(PG_FUNCTION_ARGS) {
       Datum         polya = PG_GETARG_DATUM(0);
       Datum         polyb = PG_GETARG_DATUM(1);
       PG_RETURN_DATUM(DirectFunctionCall2(poly_contain, polyb, polya)); }
/* poly_contain_pt()
 * Test to see if the point is inside the polygon.
 * Code adapted from integer-based routines in
 *     Wn: A Server for the HTTP
 *     File: wn/image.c
 *     Version 1.15.1
 *     Copyright (C) 1995  <by John Franks>
 * (code offered for use by J. Franks in Linux Journal letter.)
 */
Datum
poly_contain_pt(PG_FUNCTION_ARGS) {
       POLYGON    *poly = PG_GETARG_POLYGON_P(0);
       Point    *p = PG_GETARG_POINT_P(1);
       PG_RETURN_BOOL(point_inside(p, poly->npts, poly->p) != 0); }
Datum
pt_contained_poly(PG_FUNCTION_ARGS) {
       Point    *p = PG_GETARG_POINT_P(0);
       POLYGON    *poly = PG_GETARG_POLYGON_P(1);
       PG_RETURN_BOOL(point_inside(p, poly->npts, poly->p) != 0); }
Datum
poly_distance(PG_FUNCTION_ARGS) {
#ifdef NOT_USED
       POLYGON    *polya = PG_GETARG_POLYGON_P(0);
       POLYGON    *polyb = PG_GETARG_POLYGON_P(1);
#endif
       elog(ERROR, "poly_distance not implemented");
       PG_RETURN_NULL(); }
/***********************************************************************
 **
 **       Routines for 2D points.
 **
 ***********************************************************************/
Datum
construct_point(PG_FUNCTION_ARGS) {
       float8               x = PG_GETARG_FLOAT8(0);
       float8               y = PG_GETARG_FLOAT8(1);
       PG_RETURN_POINT_P(point_construct(x, y)); }
Datum
point_add(PG_FUNCTION_ARGS) {
       Point    *p1 = PG_GETARG_POINT_P(0);
       Point    *p2 = PG_GETARG_POINT_P(1);
       Point    *result;
       result = (Point *) palloc(sizeof(Point));
       result->x = (p1->x + p2->x);
       result->y = (p1->y + p2->y);
       PG_RETURN_POINT_P(result); }
Datum
point_sub(PG_FUNCTION_ARGS) {
       Point    *p1 = PG_GETARG_POINT_P(0);
       Point    *p2 = PG_GETARG_POINT_P(1);
       Point    *result;
       result = (Point *) palloc(sizeof(Point));
       result->x = (p1->x - p2->x);
       result->y = (p1->y - p2->y);
       PG_RETURN_POINT_P(result); }
Datum
point_mul(PG_FUNCTION_ARGS) {
       Point    *p1 = PG_GETARG_POINT_P(0);
       Point    *p2 = PG_GETARG_POINT_P(1);
       Point    *result;
       result = (Point *) palloc(sizeof(Point));
       result->x = (p1->x * p2->x) - (p1->y * p2->y);
       result->y = (p1->x * p2->y) + (p1->y * p2->x);
       PG_RETURN_POINT_P(result); }
Datum
point_div(PG_FUNCTION_ARGS) {
       Point    *p1 = PG_GETARG_POINT_P(0);
       Point    *p2 = PG_GETARG_POINT_P(1);
       Point    *result;
       double               div;
       result = (Point *) palloc(sizeof(Point));
       div = (p2->x * p2->x) + (p2->y * p2->y);
       if (div == 0.0)
             elog(ERROR, "point_div:  divide by 0.0 error");
       result->x = ((p1->x * p2->x) + (p1->y * p2->y)) / div;
       result->y = ((p2->x * p1->y) - (p2->y * p1->x)) / div;
       PG_RETURN_POINT_P(result); }
/***********************************************************************
 **
 **       Routines for 2D boxes.
 **
 ***********************************************************************/
Datum
points_box(PG_FUNCTION_ARGS) {
       Point    *p1 = PG_GETARG_POINT_P(0);
       Point    *p2 = PG_GETARG_POINT_P(1);
       PG_RETURN_BOX_P(box_construct(p1->x, p2->x, p1->y, p2->y)); }
Datum
box_add(PG_FUNCTION_ARGS) {
       BOX        *box = PG_GETARG_BOX_P(0);
       Point    *p = PG_GETARG_POINT_P(1);
       PG_RETURN_BOX_P(box_construct((box->high.x + p->x),
                                      (box->low.x + p->x),
                                      (box->high.y + p->y),
                                      (box->low.y + p->y))); }
Datum
box_sub(PG_FUNCTION_ARGS) {
       BOX        *box = PG_GETARG_BOX_P(0);
       Point    *p = PG_GETARG_POINT_P(1);
       PG_RETURN_BOX_P(box_construct((box->high.x - p->x),
                                      (box->low.x - p->x),
                                      (box->high.y - p->y),
                                      (box->low.y - p->y))); }
Datum
box_mul(PG_FUNCTION_ARGS) {
       BOX        *box = PG_GETARG_BOX_P(0);
       Point    *p = PG_GETARG_POINT_P(1);
       BOX        *result;
       Point    *high,
                     *low;
       high = DatumGetPointP(DirectFunctionCall2(point_mul,
                                                        PointPGetDatum(&box->high),
                                                        PointPGetDatum(p)));
       low = DatumGetPointP(DirectFunctionCall2(point_mul,
                                                       PointPGetDatum(&box->low),
                                                       PointPGetDatum(p)));
       result = box_construct(high->x, low->x, high->y, low->y);
       pfree(high);
       pfree(low);
       PG_RETURN_BOX_P(result); }
Datum
box_div(PG_FUNCTION_ARGS) {
       BOX        *box = PG_GETARG_BOX_P(0);
       Point    *p = PG_GETARG_POINT_P(1);
       BOX        *result;
       Point    *high,
                     *low;
       high = DatumGetPointP(DirectFunctionCall2(point_div,
                                                        PointPGetDatum(&box->high),
                                                        PointPGetDatum(p)));
       low = DatumGetPointP(DirectFunctionCall2(point_div,
                                                       PointPGetDatum(&box->low),
                                                       PointPGetDatum(p)));
       result = box_construct(high->x, low->x, high->y, low->y);
       pfree(high);
       pfree(low);
       PG_RETURN_BOX_P(result); }
/***********************************************************************
 **
 **       Routines for 2D paths.
 **
 ***********************************************************************/
/* path_add()
 * Concatenate two paths (only if they are both open).
 */
Datum
path_add(PG_FUNCTION_ARGS) {
       PATH     *p1 = PG_GETARG_PATH_P(0);
       PATH     *p2 = PG_GETARG_PATH_P(1);
       PATH     *result;
       int      size;
       int      i;
       if (p1->closed || p2->closed)
             PG_RETURN_NULL();
       size = offsetof(PATH, p[0]) +sizeof(p1->p[0]) * (p1->npts + p2->npts);
       result = (PATH *) palloc(size);
       result->size = size;
       result->npts = (p1->npts + p2->npts);
       result->closed = p1->closed;
       for (i = 0; i < p1->npts; i++) {
             result->p[i].x = p1->p[i].x;
             result->p[i].y = p1->p[i].y; }
       for (i = 0; i < p2->npts; i++) {
             result->p[i + p1->npts].x = p2->p[i].x;
             result->p[i + p1->npts].y = p2->p[i].y; }
       PG_RETURN_PATH_P(result); }
/* path_add_pt()
 * Translation operators.
 */
Datum
path_add_pt(PG_FUNCTION_ARGS) {
       PATH     *path = PG_GETARG_PATH_P_COPY(0);
       Point    *point = PG_GETARG_POINT_P(1);
       int      i;
       for (i = 0; i < path->npts; i++) {
             path->p[i].x += point->x;
             path->p[i].y += point->y; }
       PG_RETURN_PATH_P(path); }
Datum
path_sub_pt(PG_FUNCTION_ARGS) {
       PATH     *path = PG_GETARG_PATH_P_COPY(0);
       Point    *point = PG_GETARG_POINT_P(1);
       int      i;
       for (i = 0; i < path->npts; i++) {
             path->p[i].x -= point->x;
             path->p[i].y -= point->y; }
       PG_RETURN_PATH_P(path); }
/* path_mul_pt()
 * Rotation and scaling operators.
 */
Datum
path_mul_pt(PG_FUNCTION_ARGS) {
       PATH     *path = PG_GETARG_PATH_P_COPY(0);
       Point    *point = PG_GETARG_POINT_P(1);
       Point    *p;
       int      i;
       for (i = 0; i < path->npts; i++) {
             p = DatumGetPointP(DirectFunctionCall2(point_mul,
                                                       PointPGetDatum(&path->p[i]),
                                                         PointPGetDatum(point)));
             path->p[i].x = p->x;
             path->p[i].y = p->y;
             pfree(p); }
       PG_RETURN_PATH_P(path); }
Datum
path_div_pt(PG_FUNCTION_ARGS) {
       PATH     *path = PG_GETARG_PATH_P_COPY(0);
       Point    *point = PG_GETARG_POINT_P(1);
       Point    *p;
       int      i;
       for (i = 0; i < path->npts; i++) {
             p = DatumGetPointP(DirectFunctionCall2(point_div,
                                                       PointPGetDatum(&path->p[i]),
                                                         PointPGetDatum(point)));
             path->p[i].x = p->x;
             path->p[i].y = p->y;
             pfree(p); }
       PG_RETURN_PATH_P(path); }
Datum
path_center(PG_FUNCTION_ARGS) {
#ifdef NOT_USED
       PATH     *path = PG_GETARG_PATH_P(0);
#endif
       elog(ERROR, "path_center not implemented");
       PG_RETURN_NULL(); }
Datum
path_poly(PG_FUNCTION_ARGS) {
       PATH     *path = PG_GETARG_PATH_P(0);
       POLYGON    *poly;
       int      size;
       int      i;
       /* This is not very consistent --- other similar cases return NULL ... */
       if (!path->closed)
             elog(ERROR, "Open path cannot be converted to polygon");
       size = offsetof(POLYGON, p[0]) +sizeof(poly->p[0]) * path->npts;
       poly = (POLYGON *) palloc(size);
       poly->size = size;
       poly->npts = path->npts;
       for (i = 0; i < path->npts; i++) {
             poly->p[i].x = path->p[i].x;
             poly->p[i].y = path->p[i].y; }
       make_bound_box(poly);
       PG_RETURN_POLYGON_P(poly); }
/***********************************************************************
 **
 **       Routines for 2D polygons.
 **
 ***********************************************************************/
Datum
poly_npoints(PG_FUNCTION_ARGS) {
       POLYGON    *poly = PG_GETARG_POLYGON_P(0);
       PG_RETURN_INT32(poly->npts); }
Datum
poly_center(PG_FUNCTION_ARGS) {
       POLYGON    *poly = PG_GETARG_POLYGON_P(0);
       Datum         result;
       CIRCLE           *circle;
       circle = DatumGetCircleP(DirectFunctionCall1(poly_circle,
                                                           PolygonPGetDatum(poly)));
       result = DirectFunctionCall1(circle_center,
                                     CirclePGetDatum(circle));
       pfree(circle);
       PG_RETURN_DATUM(result); }
Datum
poly_box(PG_FUNCTION_ARGS) {
       POLYGON    *poly = PG_GETARG_POLYGON_P(0);
       BOX        *box;
       if (poly->npts < 1)
             PG_RETURN_NULL();
       box = box_copy(&poly->boundbox);
       PG_RETURN_BOX_P(box); }
/* box_poly()
 * Convert a box to a polygon.
 */
Datum
box_poly(PG_FUNCTION_ARGS) {
       BOX        *box = PG_GETARG_BOX_P(0);
       POLYGON    *poly;
       int      size;
       /* map four corners of the box to a polygon */
       size = offsetof(POLYGON, p[0]) +sizeof(poly->p[0]) * 4;
       poly = (POLYGON *) palloc(size);
       poly->size = size;
       poly->npts = 4;
       poly->p[0].x = box->low.x;
       poly->p[0].y = box->low.y;
       poly->p[1].x = box->low.x;
       poly->p[1].y = box->high.y;
       poly->p[2].x = box->high.x;
       poly->p[2].y = box->high.y;
       poly->p[3].x = box->high.x;
       poly->p[3].y = box->low.y;
       box_fill(&poly->boundbox, box->high.x, box->low.x,
                   box->high.y, box->low.y);
       PG_RETURN_POLYGON_P(poly); }
Datum
poly_path(PG_FUNCTION_ARGS) {
       POLYGON    *poly = PG_GETARG_POLYGON_P(0);
       PATH     *path;
       int      size;
       int      i;
       size = offsetof(PATH, p[0]) +sizeof(path->p[0]) * poly->npts;
       path = (PATH *) palloc(size);
       path->size = size;
       path->npts = poly->npts;
       path->closed = TRUE;
       for (i = 0; i < poly->npts; i++) {
             path->p[i].x = poly->p[i].x;
             path->p[i].y = poly->p[i].y; }
       PG_RETURN_PATH_P(path); }
/***********************************************************************
 **
 **       Routines for circles.
 **
 ***********************************************************************/
/*----------------------------------------------------------
 * Formatting and conversion routines.
 *---------------------------------------------------------*/
/*         circle_in   -             convert a string to internal form.
 *
 *         External format: (center and radius of circle)
 *              "((f8,f8)<f8>)"
 *              also supports quick entry style "(f8,f8,f8)"
 */
Datum
circle_in(PG_FUNCTION_ARGS) {
       char     *str = PG_GETARG_CSTRING(0);
       CIRCLE           *circle;
       char     *s,
                     *cp;
       int      depth = 0;
       circle = (CIRCLE *) palloc(sizeof(CIRCLE));
       s = str;
       while (isspace((unsigned char) *s))
             s++;
       if ((*s == LDELIM_C) || (*s == LDELIM)) {
             depth++;
             cp = (s + 1);
             while (isspace((unsigned char) *cp))
                  cp++;
             if (*cp == LDELIM)
                  s = cp; }
       if (!pair_decode(s, &circle->center.x, &circle->center.y, &s))
             elog(ERROR, "Bad circle external representation '%s'", str);
       if (*s == DELIM)
             s++;
       while (isspace((unsigned char) *s))
             s++;
       if ((!single_decode(s, &circle->radius, &s)) || (circle->radius < 0))
             elog(ERROR, "Bad circle external representation '%s'", str);
       while (depth > 0) {
             if ((*s == RDELIM)
                  || ((*s == RDELIM_C) && (depth == 1))) {
                  depth--;
                  s++;
                  while (isspace((unsigned char) *s))
                      s++; }
             else
                  elog(ERROR, "Bad circle external representation '%s'", str); }
       if (*s != '\0')
             elog(ERROR, "Bad circle external representation '%s'", str);
       PG_RETURN_CIRCLE_P(circle); }
/*         circle_out         -           convert a circle to external form.
 */
Datum
circle_out(PG_FUNCTION_ARGS) {
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(0);
       char     *result;
       char     *cp;
       result = palloc(3 * (P_MAXLEN + 1) + 3);
       cp = result;
       *cp++ = LDELIM_C;
       *cp++ = LDELIM;
       if (!pair_encode(circle->center.x, circle->center.y, cp))
             elog(ERROR, "Unable to format circle");
       cp += strlen(cp);
       *cp++ = RDELIM;
       *cp++ = DELIM;
       if (!single_encode(circle->radius, cp))
             elog(ERROR, "Unable to format circle");
       cp += strlen(cp);
       *cp++ = RDELIM_C;
       *cp = '\0';
       PG_RETURN_CSTRING(result); }
/*----------------------------------------------------------
 *     Relational operators for CIRCLEs.
 *         <, >, <=, >=, and == are based on circle area.
 *---------------------------------------------------------*/
/*         circles identical?
 */
Datum
circle_same(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPeq(circle1->radius, circle2->radius) &&
                         FPeq(circle1->center.x, circle2->center.x) &&
                         FPeq(circle1->center.y, circle2->center.y)); }
/*         circle_overlap     -     does circle1 overlap circle2?
 */
Datum
circle_overlap(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPle(point_dt(&circle1->center, &circle2->center),
                           circle1->radius + circle2->radius)); }
/*         circle_overleft -   is the right edge of circle1 to the left of
 *                                    the right edge of circle2?
 */
Datum
circle_overleft(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPle((circle1->center.x + circle1->radius),
                           (circle2->center.x + circle2->radius))); }
/*         circle_left               -         is circle1 strictly left of circle2?
 */
Datum
circle_left(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPle((circle1->center.x + circle1->radius),
                           (circle2->center.x - circle2->radius))); }
/*         circle_right       -         is circle1 strictly right of circle2?
 */
Datum
circle_right(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPge((circle1->center.x - circle1->radius),
                           (circle2->center.x + circle2->radius))); }
/*         circle_overright     -               is the left edge of circle1 to the right of
 *                                    the left edge of circle2?
 */
Datum
circle_overright(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPge((circle1->center.x - circle1->radius),
                           (circle2->center.x - circle2->radius))); }
/*         circle_contained     -               is circle1 contained by circle2?
 */
Datum
circle_contained(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPle((point_dt(&circle1->center, &circle2->center) + circle1->radius), circle2->radius)); }
/*         circle_contain     -     does circle1 contain circle2?
 */
Datum
circle_contain(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPle((point_dt(&circle1->center, &circle2->center) + circle2->radius), circle1->radius)); }
/*         circle_positionop   -
 *              is circle1 entirely {above,below} circle2?
 */
Datum
circle_below(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPle(circle1->center.y + circle1->radius,
                           circle2->center.y - circle2->radius)); }
Datum
circle_above(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPge(circle1->center.y - circle1->radius,
                           circle2->center.y + circle2->radius)); }
/*         circle_relop       -         is area(circle1) relop area(circle2), within
 *                                    our accuracy constraint?
 */
Datum
circle_eq(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPeq(circle_ar(circle1), circle_ar(circle2))); }
Datum
circle_ne(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPne(circle_ar(circle1), circle_ar(circle2))); }
Datum
circle_lt(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPlt(circle_ar(circle1), circle_ar(circle2))); }
Datum
circle_gt(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPgt(circle_ar(circle1), circle_ar(circle2))); }
Datum
circle_le(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPle(circle_ar(circle1), circle_ar(circle2))); }
Datum
circle_ge(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       PG_RETURN_BOOL(FPge(circle_ar(circle1), circle_ar(circle2))); }
/*----------------------------------------------------------
 *     "Arithmetic" operators on circles.
 *---------------------------------------------------------*/
static CIRCLE *
circle_copy(CIRCLE *circle) {
       CIRCLE           *result;
       if (!PointerIsValid(circle))
             return NULL;
       result = (CIRCLE *) palloc(sizeof(CIRCLE));
       memcpy((char *) result, (char *) circle, sizeof(CIRCLE));
       return result; }
/* circle_add_pt()
 * Translation operator.
 */
Datum
circle_add_pt(PG_FUNCTION_ARGS) {
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(0);
       Point    *point = PG_GETARG_POINT_P(1);
       CIRCLE           *result;
       result = circle_copy(circle);
       result->center.x += point->x;
       result->center.y += point->y;
       PG_RETURN_CIRCLE_P(result); }
Datum
circle_sub_pt(PG_FUNCTION_ARGS) {
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(0);
       Point    *point = PG_GETARG_POINT_P(1);
       CIRCLE           *result;
       result = circle_copy(circle);
       result->center.x -= point->x;
       result->center.y -= point->y;
       PG_RETURN_CIRCLE_P(result); }
/* circle_mul_pt()
 * Rotation and scaling operators.
 */
Datum
circle_mul_pt(PG_FUNCTION_ARGS) {
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(0);
       Point    *point = PG_GETARG_POINT_P(1);
       CIRCLE           *result;
       Point    *p;
       result = circle_copy(circle);
       p = DatumGetPointP(DirectFunctionCall2(point_mul,
                                                  PointPGetDatum(&circle->center),
                                                    PointPGetDatum(point)));
       result->center.x = p->x;
       result->center.y = p->y;
       pfree(p);
       result->radius *= HYPOT(point->x, point->y);
       PG_RETURN_CIRCLE_P(result); }
Datum
circle_div_pt(PG_FUNCTION_ARGS) {
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(0);
       Point    *point = PG_GETARG_POINT_P(1);
       CIRCLE           *result;
       Point    *p;
       result = circle_copy(circle);
       p = DatumGetPointP(DirectFunctionCall2(point_div,
                                                  PointPGetDatum(&circle->center),
                                                    PointPGetDatum(point)));
       result->center.x = p->x;
       result->center.y = p->y;
       pfree(p);
       result->radius /= HYPOT(point->x, point->y);
       PG_RETURN_CIRCLE_P(result); }
/*         circle_area               -         returns the area of the circle.
 */
Datum
circle_area(PG_FUNCTION_ARGS) {
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(0);
       PG_RETURN_FLOAT8(circle_ar(circle)); }
/*         circle_diameter -   returns the diameter of the circle.
 */
Datum
circle_diameter(PG_FUNCTION_ARGS) {
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(0);
       PG_RETURN_FLOAT8(2 * circle->radius); }
/*         circle_radius      -       returns the radius of the circle.
 */
Datum
circle_radius(PG_FUNCTION_ARGS) {
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(0);
       PG_RETURN_FLOAT8(circle->radius); }
/*         circle_distance -   returns the distance between
 *                                      two circles.
 */
Datum
circle_distance(PG_FUNCTION_ARGS) {
       CIRCLE           *circle1 = PG_GETARG_CIRCLE_P(0);
       CIRCLE           *circle2 = PG_GETARG_CIRCLE_P(1);
       float8               result;
       result = point_dt(&circle1->center, &circle2->center)
             - (circle1->radius + circle2->radius);
       if (result < 0)
             result = 0;
       PG_RETURN_FLOAT8(result); }
Datum
circle_contain_pt(PG_FUNCTION_ARGS) {
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(0);
       Point    *point = PG_GETARG_POINT_P(1);
       double               d;
       d = point_dt(&circle->center, point);
       PG_RETURN_BOOL(d <= circle->radius); }
Datum
pt_contained_circle(PG_FUNCTION_ARGS) {
       Point    *point = PG_GETARG_POINT_P(0);
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(1);
       double               d;
       d = point_dt(&circle->center, point);
       PG_RETURN_BOOL(d <= circle->radius); }
/*         dist_pc -   returns the distance between
 *                         a point and a circle.
 */
Datum
dist_pc(PG_FUNCTION_ARGS) {
       Point    *point = PG_GETARG_POINT_P(0);
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(1);
       float8               result;
       result = point_dt(point, &circle->center) - circle->radius;
       if (result < 0)
             result = 0;
       PG_RETURN_FLOAT8(result); }
/*         circle_center      -       returns the center point of the circle.
 */
Datum
circle_center(PG_FUNCTION_ARGS) {
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(0);
       Point    *result;
       result = (Point *) palloc(sizeof(Point));
       result->x = circle->center.x;
       result->y = circle->center.y;
       PG_RETURN_POINT_P(result); }
/*         circle_ar   -             returns the area of the circle.
 */
static double
circle_ar(CIRCLE *circle) {
       return PI * (circle->radius * circle->radius); }
/*----------------------------------------------------------
 *     Conversion operators.
 *---------------------------------------------------------*/
Datum
cr_circle(PG_FUNCTION_ARGS) {
       Point    *center = PG_GETARG_POINT_P(0);
       float8               radius = PG_GETARG_FLOAT8(1);
       CIRCLE           *result;
       result = (CIRCLE *) palloc(sizeof(CIRCLE));
       result->center.x = center->x;
       result->center.y = center->y;
       result->radius = radius;
       PG_RETURN_CIRCLE_P(result); }
Datum
circle_box(PG_FUNCTION_ARGS) {
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(0);
       BOX        *box;
       double               delta;
       box = (BOX *) palloc(sizeof(BOX));
       delta = circle->radius / sqrt(2.0);
       box->high.x = circle->center.x + delta;
       box->low.x = circle->center.x - delta;
       box->high.y = circle->center.y + delta;
       box->low.y = circle->center.y - delta;
       PG_RETURN_BOX_P(box); }
/* box_circle()
 * Convert a box to a circle.
 */
Datum
box_circle(PG_FUNCTION_ARGS) {
       BOX        *box = PG_GETARG_BOX_P(0);
       CIRCLE           *circle;
       circle = (CIRCLE *) palloc(sizeof(CIRCLE));
       circle->center.x = (box->high.x + box->low.x) / 2;
       circle->center.y = (box->high.y + box->low.y) / 2;
       circle->radius = point_dt(&circle->center, &box->high);
       PG_RETURN_CIRCLE_P(circle); }
Datum
circle_poly(PG_FUNCTION_ARGS) {
       int32         npts = PG_GETARG_INT32(0);
       CIRCLE           *circle = PG_GETARG_CIRCLE_P(1);
       POLYGON    *poly;
       int      size;
       int      i;
       double               angle;
       if (FPzero(circle->radius) || (npts < 2))
             elog(ERROR, "Unable to convert circle to polygon");
       size = offsetof(POLYGON, p[0]) +(sizeof(poly->p[0]) * npts);
       poly = (POLYGON *) palloc(size);
       MemSet((char *) poly, 0, size);             /* zero any holes */
       poly->size = size;
       poly->npts = npts;
       for (i = 0; i < npts; i++) {
             angle = i * (2 * PI / npts);
             poly->p[i].x = circle->center.x - (circle->radius * cos(angle));
             poly->p[i].y = circle->center.y + (circle->radius * sin(angle)); }
       make_bound_box(poly);
       PG_RETURN_POLYGON_P(poly); }
/*         poly_circle               - convert polygon to circle
 *
 * XXX This algorithm should use weighted means of line segments
 *     rather than straight average values of points - tgl 97/01/21.
 */
Datum
poly_circle(PG_FUNCTION_ARGS) {
       POLYGON    *poly = PG_GETARG_POLYGON_P(0);
       CIRCLE           *circle;
       int      i;
       if (poly->npts < 2)
             elog(ERROR, "Unable to convert polygon to circle");
       circle = (CIRCLE *) palloc(sizeof(CIRCLE));
       circle->center.x = 0;
       circle->center.y = 0;
       circle->radius = 0;
       for (i = 0; i < poly->npts; i++) {
             circle->center.x += poly->p[i].x;
             circle->center.y += poly->p[i].y; }
       circle->center.x /= poly->npts;
       circle->center.y /= poly->npts;
       for (i = 0; i < poly->npts; i++)
             circle->radius += point_dt(&poly->p[i], &circle->center);
       circle->radius /= poly->npts;
       if (FPzero(circle->radius))
             elog(ERROR, "Unable to convert polygon to circle");
       PG_RETURN_CIRCLE_P(circle); }
/***********************************************************************
 **
 **       Private routines for multiple types.
 **
 ***********************************************************************/
#define HIT_IT INT_MAX
static int
point_inside(Point *p, int npts, Point *plist) {
       double               x0,
                      y0;
       double               px,
                      py;
       int      i;
       double               x,
                      y;
       int      cross,
                      crossnum;
/*
 * We calculate crossnum, which is twice the crossing number of a
 * ray from the origin parallel to the positive X axis.
 * A coordinate change is made to move the test point to the origin.
 * Then the function lseg_crossing() is called to calculate the crossnum of
 * one segment of the translated polygon with the ray which is the
 * positive X-axis.
 */
       crossnum = 0;
       i = 0;
       if (npts <= 0)
             return 0;
       x0 = plist[0].x - p->x;
       y0 = plist[0].y - p->y;
       px = x0;
       py = y0;
       for (i = 1; i < npts; i++) {
             x = plist[i].x - p->x;
             y = plist[i].y - p->y;
             if ((cross = lseg_crossing(x, y, px, py)) == HIT_IT)
                  return 2;
             crossnum += cross;
             px = x;
             py = y; }
       if ((cross = lseg_crossing(x0, y0, px, py)) == HIT_IT)
             return 2;
       crossnum += cross;
       if (crossnum != 0)
             return 1;
       return 0; }
/* lseg_crossing()
 * The function lseg_crossing() returns +2, or -2 if the segment from (x,y)
 * to previous (x,y) crosses the positive X-axis positively or negatively.
 * It returns +1 or -1 if one endpoint is on this ray, or 0 if both are.
 * It returns 0 if the ray and the segment don't intersect.
 * It returns HIT_IT if the segment contains (0,0)
 */
static int
lseg_crossing(double x, double y, double px, double py) {
       double               z;
       int      sgn;
       /* If (px,py) = (0,0) and not first call we have already sent HIT_IT */
       if (FPzero(y)) {
             if (FPzero(x)) {
                  return HIT_IT; }
             else if (FPgt(x, 0)) {
                  if (FPzero(py))
                      return FPgt(px, 0) ? 0 : HIT_IT;
                  return FPlt(py, 0) ? 1 : -1; }
             else
             {                         /* x < 0 */
                  if (FPzero(py))
                      return FPlt(px, 0) ? 0 : HIT_IT;
                  return 0; } }
       /* Now we know y != 0;        set sgn to sign of y */
       sgn = (FPgt(y, 0) ? 1 : -1);
       if (FPzero(py))
             return FPlt(px, 0) ? 0 : sgn;
       if (FPgt((sgn * py), 0))
       {                              /* y and py have same sign */
             return 0; }
       else
       {                              /* y and py have opposite signs */
             if (FPge(x, 0) && FPgt(px, 0))
                  return 2 * sgn;
             if (FPlt(x, 0) && FPle(px, 0))
                  return 0;
             z = (x - px) * y - (y - py) * x;
             if (FPzero(z))
                  return HIT_IT;
             return FPgt((sgn * z), 0) ? 0 : 2 * sgn; } }
static bool
plist_same(int npts, Point *p1, Point *p2) {
       int      i,
                      ii,
                      j;
       /* find match for first point */
       for (i = 0; i < npts; i++) {
             if ((FPeq(p2[i].x, p1[0].x))
                  && (FPeq(p2[i].y, p1[0].y))) {
                  /* match found? then look forward through remaining points */
                  for (ii = 1, j = i + 1; ii < npts; ii++, j++) {
                      if (j >= npts)
                         j = 0;
                      if ((!FPeq(p2[j].x, p1[ii].x))
                         || (!FPeq(p2[j].y, p1[ii].y))) {
#ifdef GEODEBUG
                         printf("plist_same- %d failed forward match with %d\n", j, ii);
#endif
                         break; } }
#ifdef GEODEBUG
                  printf("plist_same- ii = %d/%d after forward match\n", ii, npts);
#endif
                  if (ii == npts)
                      return TRUE;
                  /* match not found forwards? then look backwards */
                  for (ii = 1, j = i - 1; ii < npts; ii++, j--) {
                      if (j < 0)
                         j = (npts - 1);
                      if ((!FPeq(p2[j].x, p1[ii].x))
                         || (!FPeq(p2[j].y, p1[ii].y))) {
#ifdef GEODEBUG
                         printf("plist_same- %d failed reverse match with %d\n", j, ii);
#endif
                         break; } }
#ifdef GEODEBUG
                  printf("plist_same- ii = %d/%d after reverse match\n", ii, npts);
#endif
                  if (ii == npts)
                      return TRUE; } }
       return FALSE; }
