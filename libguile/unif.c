/* Copyright (C) 1995,1996,1997,1998,2000,2001,2002,2003,2004 Free Software Foundation, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


/*
  This file has code for arrays in lots of variants (double, integer,
  unsigned etc. ). It suffers from hugely repetitive code because
  there is similar (but different) code for every variant included. (urg.)

  --hwn
*/


#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "libguile/_scm.h"
#include "libguile/__scm.h"
#include "libguile/eq.h"
#include "libguile/chars.h"
#include "libguile/eval.h"
#include "libguile/fports.h"
#include "libguile/smob.h"
#include "libguile/feature.h"
#include "libguile/root.h"
#include "libguile/strings.h"
#include "libguile/srfi-13.h"
#include "libguile/srfi-4.h"
#include "libguile/vectors.h"
#include "libguile/list.h"
#include "libguile/deprecation.h"

#include "libguile/validate.h"
#include "libguile/unif.h"
#include "libguile/ramap.h"
#include "libguile/print.h"
#include "libguile/read.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif


/* The set of uniform scm_vector types is:
 *  Vector of:		 Called:   Replaced by:
 * unsigned char	string
 * char			byvect     s8
 * boolean		bvect      
 * signed long		ivect      s32
 * unsigned long	uvect      u32
 * float		fvect      f32
 * double		dvect      d32
 * complex double	cvect
 * short		svect      s16
 * long long		llvect     s64
 */

scm_t_bits scm_tc16_array;
static SCM exactly_one_third;

/* Silly function used not to modify the semantics of the silly
 * prototype system in order to be backward compatible.
 */
static int
singp (SCM obj)
{
  if (!SCM_REALP (obj))
    return 0;
  else
    {
      double x = SCM_REAL_VALUE (obj);
      float fx = x;
      return (- SCM_FLTMAX < x) && (x < SCM_FLTMAX) && (fx == x);
    }
}

static SCM scm_i_proc_make_vector;
static SCM scm_i_proc_make_string;
static SCM scm_i_proc_make_u1vector;

#if SCM_ENABLE_DEPRECATED

SCM_SYMBOL (scm_sym_s, "s");
SCM_SYMBOL (scm_sym_l, "l");

SCM scm_make_u1vector (SCM len, SCM fill);

SCM_DEFINE (scm_make_u1vector, "make-u1vector", 1, 1, 0,
	    (SCM len, SCM fill),
	    "...")
#define FUNC_NAME s_scm_make_u1vector
{
  long k = scm_to_long (len);
  if (k > 0)
    {
      long i;
      SCM_ASSERT_RANGE (1, scm_from_long (k),
			k <= SCM_BITVECTOR_MAX_LENGTH);
      i = sizeof (long) * ((k + SCM_LONG_BIT - 1) / SCM_LONG_BIT);
      return scm_cell (SCM_MAKE_BITVECTOR_TAG (k), 
		       (scm_t_bits) scm_gc_malloc (i, "vector"));
    }
  else
    return scm_cell (SCM_MAKE_BITVECTOR_TAG (0), 0);
}
#undef FUNC_NAME

static SCM
scm_i_convert_old_prototype (SCM proto)
{
  SCM new_proto;

  /* All new 'prototypes' are creator procedures. 
   */
  if (scm_is_true (scm_procedure_p (proto)))
    return proto;

  if (scm_is_eq (proto, SCM_BOOL_T))
    new_proto = scm_i_proc_make_u1vector;
  else if (scm_is_eq (proto, SCM_MAKE_CHAR ('a')))
    new_proto = scm_i_proc_make_string;
  else if (scm_is_eq (proto, SCM_MAKE_CHAR (0)))
    new_proto = scm_i_proc_make_s8vector;
  else if (scm_is_eq (proto, scm_sym_s))
    new_proto = scm_i_proc_make_s16vector;
  else if (scm_is_true (scm_eqv_p (proto, scm_from_int (1))))
    new_proto = scm_i_proc_make_u32vector;
  else if (scm_is_true (scm_eqv_p (proto, scm_from_int (-1))))
    new_proto = scm_i_proc_make_s32vector;
  else if (scm_is_eq (proto, scm_sym_l))
    new_proto = scm_i_proc_make_s64vector;
  else if (scm_is_true (scm_eqv_p (proto, scm_from_double (1.0))))
    new_proto = scm_i_proc_make_f32vector;
  else if (scm_is_true (scm_eqv_p (proto, scm_divide (scm_from_int (1),
							 scm_from_int (3)))))
    new_proto = scm_i_proc_make_f64vector;
  else if (scm_is_true (scm_eqv_p (proto, scm_c_make_rectangular (0, 1))))
    new_proto = scm_i_proc_make_c64vector;
  else if (scm_is_null (proto))
    new_proto = scm_i_proc_make_vector;
  else
    new_proto = proto;

  scm_c_issue_deprecation_warning
    ("Using prototypes with arrays is deprecated.  "
     "Use creator functions instead.");

  return new_proto;
}

#endif

SCM 
scm_make_uve (long k, SCM prot)
#define FUNC_NAME "scm_make_uve"
{
#if SCM_ENABLE_DEPRECATED
  prot = scm_i_convert_old_prototype (prot);
#endif
  return scm_call_2 (prot, scm_from_long (k), SCM_UNDEFINED);
}
#undef FUNC_NAME

SCM_DEFINE (scm_array_p, "array?", 1, 1, 0,
           (SCM v, SCM prot),
	    "Return @code{#t} if the @var{obj} is an array, and @code{#f} if\n"
	    "not.  The @var{prototype} argument is used with uniform arrays\n"
	    "and is described elsewhere.")
#define FUNC_NAME s_scm_array_p
{
  int nprot;
  int enclosed;
  nprot = SCM_UNBNDP (prot);
  enclosed = 0;
  if (SCM_IMP (v))
    return SCM_BOOL_F;

  while (SCM_ARRAYP (v))
    {
      if (nprot)
	return SCM_BOOL_T;
      if (enclosed++)
	return SCM_BOOL_F;
      v = SCM_ARRAY_V (v);
    }

  /* XXX - clean up
   */
  if (scm_is_uniform_vector (v))
    {
      if (nprot)
	return SCM_BOOL_T;
      else
	{
#if SCM_ENABLE_DEPRECATED
	  prot = scm_i_convert_old_prototype (prot);
#endif
	  return scm_eq_p (prot, scm_i_uniform_vector_creator (v));
	}
    }
  else if (scm_is_true (scm_vector_p (v)))
    {
      if (nprot)
	return SCM_BOOL_T;
      else
	{
#if SCM_ENABLE_DEPRECATED
	  prot = scm_i_convert_old_prototype (prot);
#endif
	  return scm_eq_p (prot, scm_i_proc_make_vector);
	}
    }

  if (nprot)
    {
      switch (SCM_TYP7 (v))
 	{
 	case scm_tc7_bvect:
 	case scm_tc7_string:
 	case scm_tc7_uvect:
 	case scm_tc7_ivect:
 	case scm_tc7_svect:
#if SCM_SIZEOF_LONG_LONG != 0
 	case scm_tc7_llvect:
#endif
 	case scm_tc7_fvect:
 	case scm_tc7_dvect:
 	case scm_tc7_cvect:
 	case scm_tc7_vector:
 	case scm_tc7_wvect:
	  return SCM_BOOL_T;
 	default:
	  return SCM_BOOL_F;
	}
    }
  else
    {
      int protp = 0;

      switch (SCM_TYP7 (v))
 	{
 	case scm_tc7_bvect:
 	  protp = (scm_is_eq (prot, SCM_BOOL_T));
          break;
 	case scm_tc7_string:
 	  protp = SCM_CHARP(prot) && (SCM_CHAR (prot) != '\0');
          break;
 	case scm_tc7_uvect:
 	  protp = SCM_I_INUMP(prot) && SCM_I_INUM(prot)>0;
          break;
 	case scm_tc7_ivect:
 	  protp = SCM_I_INUMP(prot) && SCM_I_INUM(prot)<=0;
          break;
 	case scm_tc7_svect:
 	  protp = scm_is_symbol (prot)
 	    && (1 == scm_i_symbol_length (prot))
 	    && ('s' == scm_i_symbol_chars (prot)[0]);
          break;
#if SCM_SIZEOF_LONG_LONG != 0
 	case scm_tc7_llvect:
 	  protp = scm_is_symbol (prot)
 	    && (1 == scm_i_symbol_length (prot))
 	    && ('l' == scm_i_symbol_chars (prot)[0]);
          break;
#endif
 	case scm_tc7_fvect:
 	  protp = singp (prot);
          break;
 	case scm_tc7_dvect:
 	  protp = ((SCM_REALP(prot) && ! singp (prot))
                   || (SCM_FRACTIONP (prot)
                       && scm_num_eq_p (exactly_one_third, prot)));
          break;
 	case scm_tc7_cvect:
 	  protp = SCM_COMPLEXP(prot);
          break;
 	case scm_tc7_vector:
 	case scm_tc7_wvect:
 	  protp = scm_is_null(prot);
          break;
 	default:
 	  /* no default */
 	  ;
 	}
      return scm_from_bool(protp);
    }
}
#undef FUNC_NAME


SCM_DEFINE (scm_array_rank, "array-rank", 1, 0, 0, 
           (SCM ra),
	    "Return the number of dimensions of @var{obj}.  If @var{obj} is\n"
	    "not an array, @code{0} is returned.")
#define FUNC_NAME s_scm_array_rank
{
  if (scm_is_uniform_vector (ra))
    return scm_from_int (1);

  if (SCM_IMP (ra))
    return SCM_INUM0;
  switch (SCM_TYP7 (ra))
    {
    default:
      return SCM_INUM0;
    case scm_tc7_string:
    case scm_tc7_vector:
    case scm_tc7_wvect:
    case scm_tc7_uvect:
    case scm_tc7_ivect:
    case scm_tc7_fvect:
    case scm_tc7_cvect:
    case scm_tc7_dvect:
#if SCM_SIZEOF_LONG_LONG != 0
    case scm_tc7_llvect:
#endif
    case scm_tc7_svect:
      return scm_from_int (1);
    case scm_tc7_smob:
      if (SCM_ARRAYP (ra))
	return scm_from_size_t (SCM_ARRAY_NDIM (ra));
      return SCM_INUM0;
    }
}
#undef FUNC_NAME


SCM_DEFINE (scm_array_dimensions, "array-dimensions", 1, 0, 0, 
           (SCM ra),
	    "@code{Array-dimensions} is similar to @code{array-shape} but replaces\n"
	    "elements with a @code{0} minimum with one greater than the maximum. So:\n"
	    "@lisp\n"
	    "(array-dimensions (make-array 'foo '(-1 3) 5)) @result{} ((-1 3) 5)\n"
	    "@end lisp")
#define FUNC_NAME s_scm_array_dimensions
{
  SCM res = SCM_EOL;
  size_t k;
  scm_t_array_dim *s;
  if (SCM_IMP (ra))
    return SCM_BOOL_F;

  if (scm_is_uniform_vector (ra))
    return scm_cons (scm_uniform_vector_length (ra), SCM_EOL);

  switch (SCM_TYP7 (ra))
    {
    default:
      return SCM_BOOL_F;
    case scm_tc7_string:
    case scm_tc7_vector:
    case scm_tc7_wvect:
    case scm_tc7_bvect:
    case scm_tc7_uvect:
    case scm_tc7_ivect:
    case scm_tc7_fvect:
    case scm_tc7_cvect:
    case scm_tc7_dvect:
    case scm_tc7_svect:
#if SCM_SIZEOF_LONG_LONG != 0
    case scm_tc7_llvect:
#endif
      return scm_cons (scm_uniform_vector_length (ra), SCM_EOL);
    case scm_tc7_smob:
      if (!SCM_ARRAYP (ra))
	return SCM_BOOL_F;
      k = SCM_ARRAY_NDIM (ra);
      s = SCM_ARRAY_DIMS (ra);
      while (k--)
	res = scm_cons (s[k].lbnd
			? scm_cons2 (scm_from_long (s[k].lbnd),
				     scm_from_long (s[k].ubnd),
				     SCM_EOL)
			: scm_from_long (1 + s[k].ubnd),
			res);
      return res;
    }
}
#undef FUNC_NAME


SCM_DEFINE (scm_shared_array_root, "shared-array-root", 1, 0, 0, 
           (SCM ra),
	    "Return the root vector of a shared array.")
#define FUNC_NAME s_scm_shared_array_root
{
  SCM_ASSERT (SCM_ARRAYP (ra), ra, SCM_ARG1, FUNC_NAME);
  return SCM_ARRAY_V (ra);
}
#undef FUNC_NAME


SCM_DEFINE (scm_shared_array_offset, "shared-array-offset", 1, 0, 0, 
           (SCM ra),
	    "Return the root vector index of the first element in the array.")
#define FUNC_NAME s_scm_shared_array_offset
{
  SCM_ASSERT (SCM_ARRAYP (ra), ra, SCM_ARG1, FUNC_NAME);
  return scm_from_int (SCM_ARRAY_BASE (ra));
}
#undef FUNC_NAME


SCM_DEFINE (scm_shared_array_increments, "shared-array-increments", 1, 0, 0, 
           (SCM ra),
	    "For each dimension, return the distance between elements in the root vector.")
#define FUNC_NAME s_scm_shared_array_increments
{
  SCM res = SCM_EOL;
  size_t k;
  scm_t_array_dim *s;
  SCM_ASSERT (SCM_ARRAYP (ra), ra, SCM_ARG1, FUNC_NAME);
  k = SCM_ARRAY_NDIM (ra);
  s = SCM_ARRAY_DIMS (ra);
  while (k--)
    res = scm_cons (scm_from_long (s[k].inc), res);
  return res;
}
#undef FUNC_NAME


static char s_bad_ind[] = "Bad scm_array index";


long 
scm_aind (SCM ra, SCM args, const char *what)
#define FUNC_NAME what
{
  SCM ind;
  register long j;
  register unsigned long pos = SCM_ARRAY_BASE (ra);
  register unsigned long k = SCM_ARRAY_NDIM (ra);
  scm_t_array_dim *s = SCM_ARRAY_DIMS (ra);
  if (scm_is_integer (args))
    {
      if (k != 1)
	scm_error_num_args_subr (what);
      return pos + (scm_to_long (args) - s->lbnd) * (s->inc);
    }
  while (k && scm_is_pair (args))
    {
      ind = SCM_CAR (args);
      args = SCM_CDR (args);
      if (!scm_is_integer (ind))
	scm_misc_error (what, s_bad_ind, SCM_EOL);
      j = scm_to_long (ind);
      if (j < s->lbnd || j > s->ubnd)
	scm_out_of_range (what, ind);
      pos += (j - s->lbnd) * (s->inc);
      k--;
      s++;
    }
  if (k != 0 || !scm_is_null (args))
    scm_error_num_args_subr (what);

  return pos;
}
#undef FUNC_NAME


SCM 
scm_make_ra (int ndim)
{
  SCM ra;
  SCM_DEFER_INTS;
  SCM_NEWSMOB(ra, ((scm_t_bits) ndim << 17) + scm_tc16_array,
              scm_gc_malloc ((sizeof (scm_t_array) +
			      ndim * sizeof (scm_t_array_dim)),
			     "array"));
  SCM_ARRAY_V (ra) = scm_nullvect;
  SCM_ALLOW_INTS;
  return ra;
}

static char s_bad_spec[] = "Bad scm_array dimension";
/* Increments will still need to be set. */


SCM 
scm_shap2ra (SCM args, const char *what)
{
  scm_t_array_dim *s;
  SCM ra, spec, sp;
  int ndim = scm_ilength (args);
  if (ndim < 0)
    scm_misc_error (what, s_bad_spec, SCM_EOL);

  ra = scm_make_ra (ndim);
  SCM_ARRAY_BASE (ra) = 0;
  s = SCM_ARRAY_DIMS (ra);
  for (; !scm_is_null (args); s++, args = SCM_CDR (args))
    {
      spec = SCM_CAR (args);
      if (scm_is_integer (spec))
	{
	  if (scm_to_long (spec) < 0)
	    scm_misc_error (what, s_bad_spec, SCM_EOL);
	  s->lbnd = 0;
	  s->ubnd = scm_to_long (spec) - 1;
	  s->inc = 1;
	}
      else
	{
	  if (!scm_is_pair (spec) || !scm_is_integer (SCM_CAR (spec)))
	    scm_misc_error (what, s_bad_spec, SCM_EOL);
	  s->lbnd = scm_to_long (SCM_CAR (spec));
	  sp = SCM_CDR (spec);
	  if (!scm_is_pair (sp) 
	      || !scm_is_integer (SCM_CAR (sp))
	      || !scm_is_null (SCM_CDR (sp)))
	    scm_misc_error (what, s_bad_spec, SCM_EOL);
	  s->ubnd = scm_to_long (SCM_CAR (sp));
	  s->inc = 1;
	}
    }
  return ra;
}

SCM_DEFINE (scm_dimensions_to_uniform_array, "dimensions->uniform-array", 2, 1, 0,
	    (SCM dims, SCM prot, SCM fill),
	    "@deffnx {Scheme Procedure} make-uniform-vector length prototype [fill]\n"
	    "Create and return a uniform array or vector of type\n"
	    "corresponding to @var{prototype} with dimensions @var{dims} or\n"
	    "length @var{length}.  If @var{fill} is supplied, it's used to\n"
	    "fill the array, otherwise @var{prototype} is used.")
#define FUNC_NAME s_scm_dimensions_to_uniform_array
{
  size_t k;
  unsigned long rlen = 1;
  scm_t_array_dim *s;
  SCM ra;
  
  if (scm_is_integer (dims))
    {
      SCM answer = scm_make_uve (scm_to_long (dims), prot);
      if (!SCM_UNBNDP (fill))
	scm_array_fill_x (answer, fill);
      else if (scm_is_symbol (prot) || scm_is_eq (prot, SCM_MAKE_CHAR (0)))
	scm_array_fill_x (answer, scm_from_int (0));
      else if (scm_is_false (scm_procedure_p (prot)))
	scm_array_fill_x (answer, prot);
      return answer;
    }
  
  SCM_ASSERT (scm_is_null (dims) || scm_is_pair (dims),
              dims, SCM_ARG1, FUNC_NAME);
  ra = scm_shap2ra (dims, FUNC_NAME);
  SCM_SET_ARRAY_CONTIGUOUS_FLAG (ra);
  s = SCM_ARRAY_DIMS (ra);
  k = SCM_ARRAY_NDIM (ra);

  while (k--)
    {
      s[k].inc = rlen;
      SCM_ASSERT_RANGE (1, dims, s[k].lbnd <= s[k].ubnd);
      rlen = (s[k].ubnd - s[k].lbnd + 1) * s[k].inc;
    }

  SCM_ARRAY_V (ra) = scm_make_uve (rlen, prot);

  if (!SCM_UNBNDP (fill))
    scm_array_fill_x (ra, fill);
  else if (scm_is_symbol (prot) || scm_is_eq (prot, SCM_MAKE_CHAR (0)))
    scm_array_fill_x (ra, scm_from_int (0));
  else if (scm_is_false (scm_procedure_p (prot)))
    scm_array_fill_x (ra, prot);

  if (1 == SCM_ARRAY_NDIM (ra) && 0 == SCM_ARRAY_BASE (ra))
    if (s->ubnd < s->lbnd || (0 == s->lbnd && 1 == s->inc))
      return SCM_ARRAY_V (ra);
  return ra;
}
#undef FUNC_NAME


void 
scm_ra_set_contp (SCM ra)
{
  size_t k = SCM_ARRAY_NDIM (ra);
  if (k)
    {
      long inc = SCM_ARRAY_DIMS (ra)[k - 1].inc;
      while (k--)
	{
	  if (inc != SCM_ARRAY_DIMS (ra)[k].inc)
	    {
	      SCM_CLR_ARRAY_CONTIGUOUS_FLAG (ra);
	      return;
	    }
	  inc *= (SCM_ARRAY_DIMS (ra)[k].ubnd 
		  - SCM_ARRAY_DIMS (ra)[k].lbnd + 1);
	}
    }
  SCM_SET_ARRAY_CONTIGUOUS_FLAG (ra);
}


SCM_DEFINE (scm_make_shared_array, "make-shared-array", 2, 0, 1,
           (SCM oldra, SCM mapfunc, SCM dims),
	    "@code{make-shared-array} can be used to create shared subarrays of other\n"
	    "arrays.  The @var{mapper} is a function that translates coordinates in\n"
	    "the new array into coordinates in the old array.  A @var{mapper} must be\n"
	    "linear, and its range must stay within the bounds of the old array, but\n"
	    "it can be otherwise arbitrary.  A simple example:\n"
	    "@lisp\n"
	    "(define fred (make-array #f 8 8))\n"
	    "(define freds-diagonal\n"
	    "  (make-shared-array fred (lambda (i) (list i i)) 8))\n"
	    "(array-set! freds-diagonal 'foo 3)\n"
	    "(array-ref fred 3 3) @result{} foo\n"
	    "(define freds-center\n"
	    "  (make-shared-array fred (lambda (i j) (list (+ 3 i) (+ 3 j))) 2 2))\n"
	    "(array-ref freds-center 0 0) @result{} foo\n"
	    "@end lisp")
#define FUNC_NAME s_scm_make_shared_array
{
  SCM ra;
  SCM inds, indptr;
  SCM imap;
  size_t k, i;
  long old_min, new_min, old_max, new_max;
  scm_t_array_dim *s;

  SCM_VALIDATE_REST_ARGUMENT (dims);
  SCM_VALIDATE_ARRAY (1, oldra);
  SCM_VALIDATE_PROC (2, mapfunc);
  ra = scm_shap2ra (dims, FUNC_NAME);
  if (SCM_ARRAYP (oldra))
    {
      SCM_ARRAY_V (ra) = SCM_ARRAY_V (oldra);
      old_min = old_max = SCM_ARRAY_BASE (oldra);
      s = SCM_ARRAY_DIMS (oldra);
      k = SCM_ARRAY_NDIM (oldra);
      while (k--)
	{
	  if (s[k].inc > 0)
	    old_max += (s[k].ubnd - s[k].lbnd) * s[k].inc;
	  else
	    old_min += (s[k].ubnd - s[k].lbnd) * s[k].inc;
	}
    }
  else
    {
      SCM_ARRAY_V (ra) = oldra;
      old_min = 0;
      old_max = scm_to_long (scm_uniform_vector_length (oldra)) - 1;
    }
  inds = SCM_EOL;
  s = SCM_ARRAY_DIMS (ra);
  for (k = 0; k < SCM_ARRAY_NDIM (ra); k++)
    {
      inds = scm_cons (scm_from_long (s[k].lbnd), inds);
      if (s[k].ubnd < s[k].lbnd)
	{
	  if (1 == SCM_ARRAY_NDIM (ra))
	    ra = scm_make_uve (0L, scm_array_prototype (ra));
	  else
	    SCM_ARRAY_V (ra) = scm_make_uve (0L, scm_array_prototype (ra));
	  return ra;
	}
    }
  imap = scm_apply_0 (mapfunc, scm_reverse (inds));
  if (SCM_ARRAYP (oldra))
      i = (size_t) scm_aind (oldra, imap, FUNC_NAME);
  else
    {
      if (!scm_is_integer (imap))
	{
	  if (scm_ilength (imap) != 1 || !scm_is_integer (SCM_CAR (imap)))
	    SCM_MISC_ERROR (s_bad_ind, SCM_EOL);
	  imap = SCM_CAR (imap);
	}
      i = scm_to_size_t (imap);
    }
  SCM_ARRAY_BASE (ra) = new_min = new_max = i;
  indptr = inds;
  k = SCM_ARRAY_NDIM (ra);
  while (k--)
    {
      if (s[k].ubnd > s[k].lbnd)
	{
	  SCM_SETCAR (indptr, scm_sum (SCM_CAR (indptr), scm_from_int (1)));
	  imap = scm_apply_0 (mapfunc, scm_reverse (inds));
	  if (SCM_ARRAYP (oldra))

	      s[k].inc = scm_aind (oldra, imap, FUNC_NAME) - i;
	  else
	    {
	      if (!scm_is_integer (imap))
		{
		  if (scm_ilength (imap) != 1 || !scm_is_integer (SCM_CAR (imap)))
		    SCM_MISC_ERROR (s_bad_ind, SCM_EOL);
		  imap = SCM_CAR (imap);
		}
	      s[k].inc = scm_to_long (imap) - i;
	    }
	  i += s[k].inc;
	  if (s[k].inc > 0)
	    new_max += (s[k].ubnd - s[k].lbnd) * s[k].inc;
	  else
	    new_min += (s[k].ubnd - s[k].lbnd) * s[k].inc;
	}
      else
	s[k].inc = new_max - new_min + 1;	/* contiguous by default */
      indptr = SCM_CDR (indptr);
    }
  if (old_min > new_min || old_max < new_max)
    SCM_MISC_ERROR ("mapping out of range", SCM_EOL);
  if (1 == SCM_ARRAY_NDIM (ra) && 0 == SCM_ARRAY_BASE (ra))
    {
      SCM v = SCM_ARRAY_V (ra);
      unsigned long int length = scm_to_ulong (scm_uniform_vector_length (v));
      if (1 == s->inc && 0 == s->lbnd && length == 1 + s->ubnd)
	return v;
      if (s->ubnd < s->lbnd)
	return scm_make_uve (0L, scm_array_prototype (ra));
    }
  scm_ra_set_contp (ra);
  return ra;
}
#undef FUNC_NAME


/* args are RA . DIMS */
SCM_DEFINE (scm_transpose_array, "transpose-array", 1, 0, 1, 
           (SCM ra, SCM args),
	    "Return an array sharing contents with @var{array}, but with\n"
	    "dimensions arranged in a different order.  There must be one\n"
	    "@var{dim} argument for each dimension of @var{array}.\n"
	    "@var{dim0}, @var{dim1}, @dots{} should be integers between 0\n"
	    "and the rank of the array to be returned.  Each integer in that\n"
	    "range must appear at least once in the argument list.\n"
	    "\n"
	    "The values of @var{dim0}, @var{dim1}, @dots{} correspond to\n"
	    "dimensions in the array to be returned, their positions in the\n"
	    "argument list to dimensions of @var{array}.  Several @var{dim}s\n"
	    "may have the same value, in which case the returned array will\n"
	    "have smaller rank than @var{array}.\n"
	    "\n"
	    "@lisp\n"
	    "(transpose-array '#2((a b) (c d)) 1 0) @result{} #2((a c) (b d))\n"
	    "(transpose-array '#2((a b) (c d)) 0 0) @result{} #1(a d)\n"
	    "(transpose-array '#3(((a b c) (d e f)) ((1 2 3) (4 5 6))) 1 1 0) @result{}\n"
	    "                #2((a 4) (b 5) (c 6))\n"
	    "@end lisp")
#define FUNC_NAME s_scm_transpose_array
{
  SCM res, vargs;
  SCM const *ve = &vargs;
  scm_t_array_dim *s, *r;
  int ndim, i, k;

  SCM_VALIDATE_REST_ARGUMENT (args);
  SCM_ASSERT (SCM_NIMP (ra), ra, SCM_ARG1, FUNC_NAME);

  if (scm_is_uniform_vector (ra))
    {
      /* Make sure that we are called with a single zero as
	 arguments. 
      */
      if (scm_is_null (args) || !scm_is_null (SCM_CDR (args)))
	SCM_WRONG_NUM_ARGS ();
      SCM_VALIDATE_INT_COPY (SCM_ARG2, SCM_CAR (args), i);
      SCM_ASSERT_RANGE (SCM_ARG2, SCM_CAR (args), i == 0);
      return ra;
    }

  switch (SCM_TYP7 (ra))
    {
    default:
    badarg:SCM_WRONG_TYPE_ARG (1, ra);
    case scm_tc7_bvect:
    case scm_tc7_string:
    case scm_tc7_uvect:
    case scm_tc7_ivect:
    case scm_tc7_fvect:
    case scm_tc7_dvect:
    case scm_tc7_cvect:
    case scm_tc7_svect:
#if SCM_SIZEOF_LONG_LONG != 0
    case scm_tc7_llvect:
#endif
      if (scm_is_null (args) || !scm_is_null (SCM_CDR (args)))
	SCM_WRONG_NUM_ARGS ();
      SCM_VALIDATE_INT_COPY (SCM_ARG2, SCM_CAR (args), i);
      SCM_ASSERT_RANGE (SCM_ARG2, SCM_CAR (args), i == 0);
      return ra;
    case scm_tc7_smob:
      SCM_ASRTGO (SCM_ARRAYP (ra), badarg);
      vargs = scm_vector (args);
      if (SCM_VECTOR_LENGTH (vargs) != SCM_ARRAY_NDIM (ra))
	SCM_WRONG_NUM_ARGS ();
      ve = SCM_VELTS (vargs);
      ndim = 0;
      for (k = 0; k < SCM_ARRAY_NDIM (ra); k++)
	{
	  i = scm_to_signed_integer (ve[k], 0, SCM_ARRAY_NDIM(ra));
	  if (ndim < i)
	    ndim = i;
	}
      ndim++;
      res = scm_make_ra (ndim);
      SCM_ARRAY_V (res) = SCM_ARRAY_V (ra);
      SCM_ARRAY_BASE (res) = SCM_ARRAY_BASE (ra);
      for (k = ndim; k--;)
	{
	  SCM_ARRAY_DIMS (res)[k].lbnd = 0;
	  SCM_ARRAY_DIMS (res)[k].ubnd = -1;
	}
      for (k = SCM_ARRAY_NDIM (ra); k--;)
	{
	  i = scm_to_int (ve[k]);
	  s = &(SCM_ARRAY_DIMS (ra)[k]);
	  r = &(SCM_ARRAY_DIMS (res)[i]);
	  if (r->ubnd < r->lbnd)
	    {
	      r->lbnd = s->lbnd;
	      r->ubnd = s->ubnd;
	      r->inc = s->inc;
	      ndim--;
	    }
	  else
	    {
	      if (r->ubnd > s->ubnd)
		r->ubnd = s->ubnd;
	      if (r->lbnd < s->lbnd)
		{
		  SCM_ARRAY_BASE (res) += (s->lbnd - r->lbnd) * r->inc;
		  r->lbnd = s->lbnd;
		}
	      r->inc += s->inc;
	    }
	}
      if (ndim > 0)
	SCM_MISC_ERROR ("bad argument list", SCM_EOL);
      scm_ra_set_contp (res);
      return res;
    }
}
#undef FUNC_NAME

/* args are RA . AXES */
SCM_DEFINE (scm_enclose_array, "enclose-array", 1, 0, 1, 
           (SCM ra, SCM axes),
	    "@var{dim0}, @var{dim1} @dots{} should be nonnegative integers less than\n"
	    "the rank of @var{array}.  @var{enclose-array} returns an array\n"
	    "resembling an array of shared arrays.  The dimensions of each shared\n"
	    "array are the same as the @var{dim}th dimensions of the original array,\n"
	    "the dimensions of the outer array are the same as those of the original\n"
	    "array that did not match a @var{dim}.\n\n"
	    "An enclosed array is not a general Scheme array.  Its elements may not\n"
	    "be set using @code{array-set!}.  Two references to the same element of\n"
	    "an enclosed array will be @code{equal?} but will not in general be\n"
	    "@code{eq?}.  The value returned by @var{array-prototype} when given an\n"
	    "enclosed array is unspecified.\n\n"
	    "examples:\n"
	    "@lisp\n"
	    "(enclose-array '#3(((a b c) (d e f)) ((1 2 3) (4 5 6))) 1) @result{}\n"
	    "   #<enclosed-array (#1(a d) #1(b e) #1(c f)) (#1(1 4) #1(2 5) #1(3 6))>\n\n"
	    "(enclose-array '#3(((a b c) (d e f)) ((1 2 3) (4 5 6))) 1 0) @result{}\n"
	    "   #<enclosed-array #2((a 1) (d 4)) #2((b 2) (e 5)) #2((c 3) (f 6))>\n"
	    "@end lisp")
#define FUNC_NAME s_scm_enclose_array
{
  SCM axv, res, ra_inr;
  const char *c_axv;
  scm_t_array_dim vdim, *s = &vdim;
  int ndim, j, k, ninr, noutr;

  SCM_VALIDATE_REST_ARGUMENT (axes);
  if (scm_is_null (axes))
      axes = scm_cons ((SCM_ARRAYP (ra) ? scm_from_size_t (SCM_ARRAY_NDIM (ra) - 1) : SCM_INUM0), SCM_EOL);
  ninr = scm_ilength (axes);
  if (ninr < 0)
    SCM_WRONG_NUM_ARGS ();
  ra_inr = scm_make_ra (ninr);
  SCM_ASRTGO (SCM_NIMP (ra), badarg1);

  if (scm_is_uniform_vector (ra))
    goto uniform_vector;

  switch SCM_TYP7 (ra)
    {
    default:
    badarg1:SCM_WRONG_TYPE_ARG (1, ra);
    case scm_tc7_string:
    case scm_tc7_bvect:
    case scm_tc7_uvect:
    case scm_tc7_ivect:
    case scm_tc7_fvect:
    case scm_tc7_dvect:
    case scm_tc7_cvect:
    case scm_tc7_vector:
    case scm_tc7_wvect:
    case scm_tc7_svect:
#if SCM_SIZEOF_LONG_LONG != 0
    case scm_tc7_llvect:
#endif
    uniform_vector:
      s->lbnd = 0;
      s->ubnd = scm_to_long (scm_uniform_vector_length (ra)) - 1;
      s->inc = 1;
      SCM_ARRAY_V (ra_inr) = ra;
      SCM_ARRAY_BASE (ra_inr) = 0;
      ndim = 1;
      break;
    case scm_tc7_smob:
      SCM_ASRTGO (SCM_ARRAYP (ra), badarg1);
      s = SCM_ARRAY_DIMS (ra);
      SCM_ARRAY_V (ra_inr) = SCM_ARRAY_V (ra);
      SCM_ARRAY_BASE (ra_inr) = SCM_ARRAY_BASE (ra);
      ndim = SCM_ARRAY_NDIM (ra);
      break;
    }
  noutr = ndim - ninr;
  if (noutr < 0)
    SCM_WRONG_NUM_ARGS ();
  axv = scm_make_string (scm_from_int (ndim), SCM_MAKE_CHAR (0));
  res = scm_make_ra (noutr);
  SCM_ARRAY_BASE (res) = SCM_ARRAY_BASE (ra_inr);
  SCM_ARRAY_V (res) = ra_inr;
  for (k = 0; k < ninr; k++, axes = SCM_CDR (axes))
    {
      if (!scm_is_integer (SCM_CAR (axes)))
	SCM_MISC_ERROR ("bad axis", SCM_EOL);
      j = scm_to_int (SCM_CAR (axes));
      SCM_ARRAY_DIMS (ra_inr)[k].lbnd = s[j].lbnd;
      SCM_ARRAY_DIMS (ra_inr)[k].ubnd = s[j].ubnd;
      SCM_ARRAY_DIMS (ra_inr)[k].inc = s[j].inc;
      scm_c_string_set_x (axv, j, SCM_MAKE_CHAR (1));
    }
  c_axv = scm_i_string_chars (axv);
  for (j = 0, k = 0; k < noutr; k++, j++)
    {
      while (c_axv[j])
	j++;
      SCM_ARRAY_DIMS (res)[k].lbnd = s[j].lbnd;
      SCM_ARRAY_DIMS (res)[k].ubnd = s[j].ubnd;
      SCM_ARRAY_DIMS (res)[k].inc = s[j].inc;
    }
  scm_remember_upto_here_1 (axv);
  scm_ra_set_contp (ra_inr);
  scm_ra_set_contp (res);
  return res;
}
#undef FUNC_NAME



SCM_DEFINE (scm_array_in_bounds_p, "array-in-bounds?", 1, 0, 1, 
           (SCM v, SCM args),
	    "Return @code{#t} if its arguments would be acceptable to\n"
	    "@code{array-ref}.")
#define FUNC_NAME s_scm_array_in_bounds_p
{
  SCM ind = SCM_EOL;
  long pos = 0;
  register size_t k;
  register long j;
  scm_t_array_dim *s;

  SCM_VALIDATE_REST_ARGUMENT (args);
  SCM_ASRTGO (SCM_NIMP (v), badarg1);
  if (SCM_NIMP (args))

    {
      ind = SCM_CAR (args);
      args = SCM_CDR (args);
      pos = scm_to_long (ind);
    }
tail:

  if (scm_is_uniform_vector (v))
    goto uniform_vector;

  switch SCM_TYP7 (v)
    {
    default:
    badarg1:SCM_WRONG_TYPE_ARG (1, v);
    wna: SCM_WRONG_NUM_ARGS ();
    case scm_tc7_smob:
      k = SCM_ARRAY_NDIM (v);
      s = SCM_ARRAY_DIMS (v);
      pos = SCM_ARRAY_BASE (v);
      if (!k)
	{
	  SCM_ASRTGO (scm_is_null (ind), wna);
	  ind = SCM_INUM0;
	}
      else
	while (!0)
	  {
	    j = scm_to_long (ind);
	    if (!(j >= (s->lbnd) && j <= (s->ubnd)))
	      {
		SCM_ASRTGO (--k == scm_ilength (args), wna);
		return SCM_BOOL_F;
	      }
	    pos += (j - s->lbnd) * (s->inc);
	    if (!(--k && SCM_NIMP (args)))
	      break;
	    ind = SCM_CAR (args);
	    args = SCM_CDR (args);
	    s++;
	    if (!scm_is_integer (ind))
	      SCM_MISC_ERROR (s_bad_ind, SCM_EOL);
	  }
      SCM_ASRTGO (0 == k, wna);
      v = SCM_ARRAY_V (v);
      goto tail;
    case scm_tc7_bvect:
    case scm_tc7_string:
    case scm_tc7_uvect:
    case scm_tc7_ivect:
    case scm_tc7_fvect:
    case scm_tc7_dvect:
    case scm_tc7_cvect:
    case scm_tc7_svect:
#if SCM_SIZEOF_LONG_LONG != 0
    case scm_tc7_llvect:
#endif
    case scm_tc7_vector:
    case scm_tc7_wvect:
    uniform_vector:
      {
	unsigned long length = scm_to_ulong (scm_uniform_vector_length (v));
	SCM_ASRTGO (scm_is_null (args) && scm_is_integer (ind), wna);
	return scm_from_bool(pos >= 0 && pos < length);
      }
    }
}
#undef FUNC_NAME


SCM_DEFINE (scm_array_ref, "array-ref", 1, 0, 1,
           (SCM v, SCM args),
	    "Return the element at the @code{(index1, index2)} element in\n"
	    "@var{array}.")
#define FUNC_NAME s_scm_array_ref
{
  long pos;

  if (SCM_IMP (v))
    {
      SCM_ASRTGO (scm_is_null (args), badarg);
      return v;
    }
  else if (SCM_ARRAYP (v))
    {
      pos = scm_aind (v, args, FUNC_NAME);
      v = SCM_ARRAY_V (v);
    }
  else
    {
      unsigned long int length;
      if (SCM_NIMP (args))
	{
	  SCM_ASSERT (scm_is_pair (args), args, SCM_ARG2, FUNC_NAME);
	  pos = scm_to_long (SCM_CAR (args));
	  SCM_ASRTGO (scm_is_null (SCM_CDR (args)), wna);
	}
      else
	{
	  pos = scm_to_long (args);
	}
      length = scm_to_ulong (scm_uniform_vector_length (v));
      SCM_ASRTGO (pos >= 0 && pos < length, outrng);
    }

  if (scm_is_uniform_vector (v))
    return scm_uniform_vector_ref (v, scm_from_long (pos));

  switch SCM_TYP7 (v)
    {
    default:
      if (scm_is_null (args))
 return v;
    badarg:
      SCM_WRONG_TYPE_ARG (1, v);
      /* not reached */

    outrng:
      scm_out_of_range (FUNC_NAME, scm_from_long (pos));
    wna:
      SCM_WRONG_NUM_ARGS ();
    case scm_tc7_smob:
      {				/* enclosed */
	int k = SCM_ARRAY_NDIM (v);
	SCM res = scm_make_ra (k);
	SCM_ARRAY_V (res) = SCM_ARRAY_V (v);
	SCM_ARRAY_BASE (res) = pos;
	while (k--)
	  {
	    SCM_ARRAY_DIMS (res)[k].lbnd = SCM_ARRAY_DIMS (v)[k].lbnd;
	    SCM_ARRAY_DIMS (res)[k].ubnd = SCM_ARRAY_DIMS (v)[k].ubnd;
	    SCM_ARRAY_DIMS (res)[k].inc = SCM_ARRAY_DIMS (v)[k].inc;
	  }
	return res;
      }
    case scm_tc7_bvect:
      if (SCM_BITVEC_REF (v, pos))
	return SCM_BOOL_T;
      else
	return SCM_BOOL_F;
    case scm_tc7_string:
      return scm_c_string_ref (v, pos);
    case scm_tc7_uvect:
    return scm_from_ulong (((unsigned long *) SCM_VELTS (v))[pos]);
    case scm_tc7_ivect:
      return scm_from_long (((signed long *) SCM_VELTS (v))[pos]);

    case scm_tc7_svect:
      return scm_from_short (((short *) SCM_CELL_WORD_1 (v))[pos]);
#if SCM_SIZEOF_LONG_LONG != 0
    case scm_tc7_llvect:
      return scm_from_long_long (((long long *) SCM_CELL_WORD_1 (v))[pos]);
#endif

    case scm_tc7_fvect:
      return scm_from_double (((float *) SCM_CELL_WORD_1 (v))[pos]);
    case scm_tc7_dvect:
      return scm_from_double (((double *) SCM_CELL_WORD_1 (v))[pos]);
    case scm_tc7_cvect:
      return scm_c_make_rectangular (((double *) SCM_CELL_WORD_1(v))[2*pos],
				     ((double *) SCM_CELL_WORD_1(v))[2*pos+1]);
    case scm_tc7_vector:
    case scm_tc7_wvect:
      return SCM_VELTS (v)[pos];
    }
}
#undef FUNC_NAME

/* Internal version of scm_uniform_vector_ref for uves that does no error checking and
   tries to recycle conses.  (Make *sure* you want them recycled.) */

SCM 
scm_cvref (SCM v, unsigned long pos, SCM last)
#define FUNC_NAME "scm_cvref"
{
  if (scm_is_uniform_vector (v))
    return scm_uniform_vector_ref (v, scm_from_ulong (pos));

  switch SCM_TYP7 (v)
    {
    default:
      SCM_WRONG_TYPE_ARG (SCM_ARG1, v);
    case scm_tc7_bvect:
      if (SCM_BITVEC_REF(v, pos))
	return SCM_BOOL_T;
      else
	return SCM_BOOL_F;
    case scm_tc7_string:
      return scm_c_string_ref (v, pos);
    case scm_tc7_uvect:
      return scm_from_ulong (((unsigned long *) SCM_VELTS (v))[pos]);
    case scm_tc7_ivect:
      return scm_from_long (((signed long *) SCM_VELTS (v))[pos]);
    case scm_tc7_svect:
      return scm_from_short (((short *) SCM_CELL_WORD_1 (v))[pos]);
#if SCM_SIZEOF_LONG_LONG != 0
    case scm_tc7_llvect:
      return scm_from_long_long (((long long *) SCM_CELL_WORD_1 (v))[pos]);
#endif
    case scm_tc7_fvect:
      if (SCM_REALP (last) && !scm_is_eq (last, scm_flo0))
	{
	  SCM_REAL_VALUE (last) = ((float *) SCM_CELL_WORD_1 (v))[pos];
	  return last;
	}
      return scm_from_double (((float *) SCM_CELL_WORD_1 (v))[pos]);
    case scm_tc7_dvect:
      if (SCM_REALP (last) && !scm_is_eq (last, scm_flo0))
	{
	  SCM_REAL_VALUE (last) = ((double *) SCM_CELL_WORD_1 (v))[pos];
	  return last;
	}
      return scm_from_double (((double *) SCM_CELL_WORD_1 (v))[pos]);
    case scm_tc7_cvect:
      if (SCM_COMPLEXP (last))
	{
	  SCM_COMPLEX_REAL (last) = ((double *) SCM_CELL_WORD_1 (v))[2 * pos];
	  SCM_COMPLEX_IMAG (last) = ((double *) SCM_CELL_WORD_1 (v))[2 * pos + 1];
	  return last;
	}
      return scm_c_make_rectangular (((double *) SCM_CELL_WORD_1(v))[2*pos],
				     ((double *) SCM_CELL_WORD_1(v))[2*pos+1]);
    case scm_tc7_vector:
    case scm_tc7_wvect:
      return SCM_VELTS (v)[pos];
    case scm_tc7_smob:
      {				/* enclosed scm_array */
	int k = SCM_ARRAY_NDIM (v);
	SCM res = scm_make_ra (k);
	SCM_ARRAY_V (res) = SCM_ARRAY_V (v);
	SCM_ARRAY_BASE (res) = pos;
	while (k--)
	  {
	    SCM_ARRAY_DIMS (res)[k].ubnd = SCM_ARRAY_DIMS (v)[k].ubnd;
	    SCM_ARRAY_DIMS (res)[k].lbnd = SCM_ARRAY_DIMS (v)[k].lbnd;
	    SCM_ARRAY_DIMS (res)[k].inc = SCM_ARRAY_DIMS (v)[k].inc;
	  }
	return res;
      }
    }
}
#undef FUNC_NAME


SCM_REGISTER_PROC(s_uniform_array_set1_x, "uniform-array-set1!", 3, 0, 0, scm_array_set_x);


/* Note that args may be a list or an immediate object, depending which
   PROC is used (and it's called from C too).  */
SCM_DEFINE (scm_array_set_x, "array-set!", 2, 0, 1, 
           (SCM v, SCM obj, SCM args),
	    "@deffnx {Scheme Procedure} uniform-array-set1! v obj args\n"
	    "Set the element at the @code{(index1, index2)} element in @var{array} to\n"
	    "@var{new-value}.  The value returned by array-set! is unspecified.")
#define FUNC_NAME s_scm_array_set_x           
{
  long pos = 0;

  SCM_ASRTGO (SCM_NIMP (v), badarg1);
  if (SCM_ARRAYP (v))
    {
      pos = scm_aind (v, args, FUNC_NAME);
      v = SCM_ARRAY_V (v);
    }
  else
    {
      unsigned long int length;
      if (scm_is_pair (args))
	{
	  SCM_ASRTGO (scm_is_null (SCM_CDR (args)), wna);
	  pos = scm_to_long (SCM_CAR (args));
	}
      else
	{
	  pos = scm_to_long (args);
	}
      length = scm_to_ulong (scm_uniform_vector_length (v));
      SCM_ASRTGO (pos >= 0 && pos < length, outrng);
    }

  if (scm_is_uniform_vector (v))
    return scm_uniform_vector_set_x (v, scm_from_long (pos), obj);

  switch (SCM_TYP7 (v))
    {
    default: badarg1:
      SCM_WRONG_TYPE_ARG (1, v);
      /* not reached */
    outrng:
      scm_out_of_range (FUNC_NAME, scm_from_long (pos));
    wna:
      SCM_WRONG_NUM_ARGS ();
    case scm_tc7_smob:		/* enclosed */
      goto badarg1;
    case scm_tc7_bvect:
      if (scm_is_false (obj))
	SCM_BITVEC_CLR(v, pos);
      else if (scm_is_eq (obj, SCM_BOOL_T))
	SCM_BITVEC_SET(v, pos);
      else
	badobj:SCM_WRONG_TYPE_ARG (2, obj);
      break;
    case scm_tc7_string:
      SCM_ASRTGO (SCM_CHARP (obj), badobj);
      scm_c_string_set_x (v, pos, obj);
      break;
    case scm_tc7_uvect:
      ((unsigned long *) SCM_UVECTOR_BASE (v))[pos] = scm_to_ulong (obj);
      break;
    case scm_tc7_ivect:
      ((long *) SCM_UVECTOR_BASE (v))[pos] = scm_to_long (obj);
      break;
    case scm_tc7_svect:
      ((short *) SCM_UVECTOR_BASE (v))[pos] = scm_to_short (obj);
      break;
#if SCM_SIZEOF_LONG_LONG != 0
    case scm_tc7_llvect:
      ((long long *) SCM_UVECTOR_BASE (v))[pos] = scm_to_long_long (obj);
      break;
#endif
    case scm_tc7_fvect:
      ((float *) SCM_UVECTOR_BASE (v))[pos] = scm_to_double (obj);
      break;
    case scm_tc7_dvect:
      ((double *) SCM_UVECTOR_BASE (v))[pos] = scm_to_double (obj);
      break;
    case scm_tc7_cvect:
      SCM_ASRTGO (SCM_INEXACTP (obj), badobj);
      if (SCM_REALP (obj)) {
	((double *) SCM_UVECTOR_BASE (v))[2 * pos] = SCM_REAL_VALUE (obj);
	((double *) SCM_UVECTOR_BASE (v))[2 * pos + 1] = 0.0;
      } else {
	((double *) SCM_UVECTOR_BASE (v))[2 * pos] = SCM_COMPLEX_REAL (obj);
	((double *) SCM_UVECTOR_BASE (v))[2 * pos + 1] = SCM_COMPLEX_IMAG (obj);
      }
      break;
    case scm_tc7_vector:
    case scm_tc7_wvect:
      SCM_VECTOR_SET (v, pos, obj);
      break;
    }
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

/* attempts to unroll an array into a one-dimensional array.
   returns the unrolled array or #f if it can't be done.  */
  /* if strict is not SCM_UNDEFINED, return #f if returned array
		     wouldn't have contiguous elements.  */
SCM_DEFINE (scm_array_contents, "array-contents", 1, 1, 0,
           (SCM ra, SCM strict),
	    "If @var{array} may be @dfn{unrolled} into a one dimensional shared array\n"
	    "without changing their order (last subscript changing fastest), then\n"
	    "@code{array-contents} returns that shared array, otherwise it returns\n"
	    "@code{#f}.  All arrays made by @var{make-array} and\n"
	    "@var{make-uniform-array} may be unrolled, some arrays made by\n"
	    "@var{make-shared-array} may not be.\n\n"
	    "If the optional argument @var{strict} is provided, a shared array will\n"
	    "be returned only if its elements are stored internally contiguous in\n"
	    "memory.")
#define FUNC_NAME s_scm_array_contents
{
  SCM sra;

  if (scm_is_uniform_vector (ra))
    return ra;

  if (SCM_IMP (ra))
    return SCM_BOOL_F;

  switch SCM_TYP7 (ra)
    {
    default:
      return SCM_BOOL_F;
    case scm_tc7_vector:
    case scm_tc7_wvect:
    case scm_tc7_string:
    case scm_tc7_bvect:
    case scm_tc7_uvect:
    case scm_tc7_ivect:
    case scm_tc7_fvect:
    case scm_tc7_dvect:
    case scm_tc7_cvect:
    case scm_tc7_svect:
#if SCM_SIZEOF_LONG_LONG != 0
    case scm_tc7_llvect:
#endif
      return ra;
    case scm_tc7_smob:
      {
	size_t k, ndim = SCM_ARRAY_NDIM (ra), len = 1;
	if (!SCM_ARRAYP (ra) || !SCM_ARRAY_CONTP (ra))
	  return SCM_BOOL_F;
	for (k = 0; k < ndim; k++)
	  len *= SCM_ARRAY_DIMS (ra)[k].ubnd - SCM_ARRAY_DIMS (ra)[k].lbnd + 1;
	if (!SCM_UNBNDP (strict))
	  {
	    if (ndim && (1 != SCM_ARRAY_DIMS (ra)[ndim - 1].inc))
	      return SCM_BOOL_F;
	    if (scm_tc7_bvect == SCM_TYP7 (SCM_ARRAY_V (ra)))
	      {
		if (len != SCM_BITVECTOR_LENGTH (SCM_ARRAY_V (ra)) ||
		    SCM_ARRAY_BASE (ra) % SCM_LONG_BIT ||
		    len % SCM_LONG_BIT)
		  return SCM_BOOL_F;
	      }
	  }

	{
	  SCM v = SCM_ARRAY_V (ra);
	  unsigned long length = scm_to_ulong (scm_uniform_vector_length (v));
	  if ((len == length) && 0 == SCM_ARRAY_BASE (ra) && SCM_ARRAY_DIMS (ra)->inc)
	    return v;
	}

	sra = scm_make_ra (1);
	SCM_ARRAY_DIMS (sra)->lbnd = 0;
	SCM_ARRAY_DIMS (sra)->ubnd = len - 1;
	SCM_ARRAY_V (sra) = SCM_ARRAY_V (ra);
	SCM_ARRAY_BASE (sra) = SCM_ARRAY_BASE (ra);
	SCM_ARRAY_DIMS (sra)->inc = (ndim ? SCM_ARRAY_DIMS (ra)[ndim - 1].inc : 1);
	return sra;
      }
    }
}
#undef FUNC_NAME


SCM 
scm_ra2contig (SCM ra, int copy)
{
  SCM ret;
  long inc = 1;
  size_t k, len = 1;
  for (k = SCM_ARRAY_NDIM (ra); k--;)
    len *= SCM_ARRAY_DIMS (ra)[k].ubnd - SCM_ARRAY_DIMS (ra)[k].lbnd + 1;
  k = SCM_ARRAY_NDIM (ra);
  if (SCM_ARRAY_CONTP (ra) && ((0 == k) || (1 == SCM_ARRAY_DIMS (ra)[k - 1].inc)))
    {
      if (scm_tc7_bvect != SCM_TYP7 (SCM_ARRAY_V (ra)))
	return ra;
      if ((len == SCM_BITVECTOR_LENGTH (SCM_ARRAY_V (ra)) &&
	   0 == SCM_ARRAY_BASE (ra) % SCM_LONG_BIT &&
	   0 == len % SCM_LONG_BIT))
	return ra;
    }
  ret = scm_make_ra (k);
  SCM_ARRAY_BASE (ret) = 0;
  while (k--)
    {
      SCM_ARRAY_DIMS (ret)[k].lbnd = SCM_ARRAY_DIMS (ra)[k].lbnd;
      SCM_ARRAY_DIMS (ret)[k].ubnd = SCM_ARRAY_DIMS (ra)[k].ubnd;
      SCM_ARRAY_DIMS (ret)[k].inc = inc;
      inc *= SCM_ARRAY_DIMS (ra)[k].ubnd - SCM_ARRAY_DIMS (ra)[k].lbnd + 1;
    }
  SCM_ARRAY_V (ret) = scm_make_uve (inc, scm_array_prototype (ra));
  if (copy)
    scm_array_copy_x (ra, ret);
  return ret;
}



SCM_DEFINE (scm_uniform_array_read_x, "uniform-array-read!", 1, 3, 0,
           (SCM ra, SCM port_or_fd, SCM start, SCM end),
	    "@deffnx {Scheme Procedure} uniform-vector-read! uve [port-or-fdes] [start] [end]\n"
	    "Attempt to read all elements of @var{ura}, in lexicographic order, as\n"
	    "binary objects from @var{port-or-fdes}.\n"
	    "If an end of file is encountered,\n"
	    "the objects up to that point are put into @var{ura}\n"
	    "(starting at the beginning) and the remainder of the array is\n"
	    "unchanged.\n\n"
	    "The optional arguments @var{start} and @var{end} allow\n"
	    "a specified region of a vector (or linearized array) to be read,\n"
	    "leaving the remainder of the vector unchanged.\n\n"
	    "@code{uniform-array-read!} returns the number of objects read.\n"
	    "@var{port-or-fdes} may be omitted, in which case it defaults to the value\n"
	    "returned by @code{(current-input-port)}.")
#define FUNC_NAME s_scm_uniform_array_read_x
{
  SCM cra = SCM_UNDEFINED, v = ra;
  long sz, vlen, ans;
  long cstart = 0;
  long cend;
  long offset = 0;
  char *base;

  SCM_ASRTGO (SCM_NIMP (v), badarg1);
  if (SCM_UNBNDP (port_or_fd))
    port_or_fd = scm_cur_inp;
  else
    SCM_ASSERT (scm_is_integer (port_or_fd)
		|| (SCM_OPINPORTP (port_or_fd)),
		port_or_fd, SCM_ARG2, FUNC_NAME);
  vlen = (SCM_TYP7 (v) == scm_tc7_smob
	  ? 0
	  : scm_to_long (scm_uniform_vector_length (v)));

loop:
  if (scm_is_uniform_vector (v))
    {
      base = scm_uniform_vector_elements (v);
      sz = scm_uniform_vector_element_size (v);
    }
  else
    switch SCM_TYP7 (v)
      {
      default:
      badarg1:SCM_WRONG_TYPE_ARG (SCM_ARG1, v);
      case scm_tc7_smob:
	SCM_ASRTGO (SCM_ARRAYP (v), badarg1);
	cra = scm_ra2contig (ra, 0);
	cstart += SCM_ARRAY_BASE (cra);
	vlen = SCM_ARRAY_DIMS (cra)->inc *
	  (SCM_ARRAY_DIMS (cra)->ubnd - SCM_ARRAY_DIMS (cra)->lbnd + 1);
	v = SCM_ARRAY_V (cra);
	goto loop;
      case scm_tc7_string:
	base = NULL;  /* writing to strings is special, see below. */
	sz = sizeof (char);
	break;
      case scm_tc7_bvect:
	base = (char *) SCM_BITVECTOR_BASE (v);
	vlen = (vlen + SCM_LONG_BIT - 1) / SCM_LONG_BIT;
	cstart /= SCM_LONG_BIT;
	sz = sizeof (long);
	break;
      case scm_tc7_uvect:
      case scm_tc7_ivect:
	base = (char *) SCM_UVECTOR_BASE (v);
	sz = sizeof (long);
	break;
      case scm_tc7_svect:
	base = (char *) SCM_UVECTOR_BASE (v);
	sz = sizeof (short);
	break;
#if SCM_SIZEOF_LONG_LONG != 0
      case scm_tc7_llvect:
	base = (char *) SCM_UVECTOR_BASE (v);
	sz = sizeof (long long);
	break;
#endif
      case scm_tc7_fvect:
	base = (char *) SCM_UVECTOR_BASE (v);
	sz = sizeof (float);
	break;
      case scm_tc7_dvect:
	base = (char *) SCM_UVECTOR_BASE (v);
	sz = sizeof (double);
	break;
      case scm_tc7_cvect:
	base = (char *) SCM_UVECTOR_BASE (v);
	sz = 2 * sizeof (double);
	break;
      }
  
  cend = vlen;
  if (!SCM_UNBNDP (start))
    {
      offset = 
	SCM_NUM2LONG (3, start);

      if (offset < 0 || offset >= cend)
	scm_out_of_range (FUNC_NAME, start);

      if (!SCM_UNBNDP (end))
	{
	  long tend =
	    SCM_NUM2LONG (4, end);
      
	  if (tend <= offset || tend > cend)
	    scm_out_of_range (FUNC_NAME, end);
	  cend = tend;
	}
    }

  if (SCM_NIMP (port_or_fd))
    {
      scm_t_port *pt = SCM_PTAB_ENTRY (port_or_fd);
      int remaining = (cend - offset) * sz;
      size_t off = (cstart + offset) * sz;

      if (pt->rw_active == SCM_PORT_WRITE)
	scm_flush (port_or_fd);

      ans = cend - offset;
      while (remaining > 0)
	{
	  if (pt->read_pos < pt->read_end)
	    {
	      int to_copy = min (pt->read_end - pt->read_pos,
				 remaining);

	      if (base == NULL)
		{
		  /* strings */
		  char *b = scm_i_string_writable_chars (v);
		  memcpy (b + off, pt->read_pos, to_copy);
		  scm_i_string_stop_writing ();
		}
	      else
		memcpy (base + off, pt->read_pos, to_copy);
	      pt->read_pos += to_copy;
	      remaining -= to_copy;
	      off += to_copy;
	    }
	  else
	    {
	      if (scm_fill_input (port_or_fd) == EOF)
		{
		  if (remaining % sz != 0)
		    {
		      SCM_MISC_ERROR ("unexpected EOF", SCM_EOL);
		    }
		  ans -= remaining / sz;
		  break;
		}
	    }
	}
      
      if (pt->rw_random)
	pt->rw_active = SCM_PORT_READ;
    }
  else /* file descriptor.  */
    {
      if (base == NULL)
	{
	  /* strings */
	  char *b = scm_i_string_writable_chars (v);
	  SCM_SYSCALL (ans = read (scm_to_int (port_or_fd),
				   b + (cstart + offset) * sz,
				   (sz * (cend - offset))));
	  scm_i_string_stop_writing ();
	}
      else
	SCM_SYSCALL (ans = read (scm_to_int (port_or_fd),
				 base + (cstart + offset) * sz,
				 (sz * (cend - offset))));
      if (ans == -1)
	SCM_SYSERROR;
    }
  if (SCM_TYP7 (v) == scm_tc7_bvect)
    ans *= SCM_LONG_BIT;

  if (!scm_is_eq (v, ra) && !scm_is_eq (cra, ra))
    scm_array_copy_x (cra, ra);

  return scm_from_long (ans);
}
#undef FUNC_NAME

SCM_DEFINE (scm_uniform_array_write, "uniform-array-write", 1, 3, 0,
           (SCM v, SCM port_or_fd, SCM start, SCM end),
	    "@deffnx {Scheme Procedure} uniform-vector-write uve [port-or-fdes] [start] [end]\n"
	    "Writes all elements of @var{ura} as binary objects to\n"
	    "@var{port-or-fdes}.\n\n"
	    "The optional arguments @var{start}\n"
	    "and @var{end} allow\n"
	    "a specified region of a vector (or linearized array) to be written.\n\n"
	    "The number of objects actually written is returned.\n"
	    "@var{port-or-fdes} may be\n"
	    "omitted, in which case it defaults to the value returned by\n"
	    "@code{(current-output-port)}.")
#define FUNC_NAME s_scm_uniform_array_write
{
  long sz, vlen, ans;
  long offset = 0;
  long cstart = 0;
  long cend;
  const char *base;

  port_or_fd = SCM_COERCE_OUTPORT (port_or_fd);

  SCM_ASRTGO (SCM_NIMP (v), badarg1);
  if (SCM_UNBNDP (port_or_fd))
    port_or_fd = scm_cur_outp;
  else
    SCM_ASSERT (scm_is_integer (port_or_fd)
		|| (SCM_OPOUTPORTP (port_or_fd)),
		port_or_fd, SCM_ARG2, FUNC_NAME);
  vlen = (SCM_TYP7 (v) == scm_tc7_smob
	  ? 0
	  : scm_to_long (scm_uniform_vector_length (v)));
  
loop:
  if (scm_is_uniform_vector (v))
    {
      base = scm_uniform_vector_elements (v);
      sz = scm_uniform_vector_element_size (v);
    }
  else
    switch SCM_TYP7 (v)
      {
      default:
      badarg1:SCM_WRONG_TYPE_ARG (1, v);
      case scm_tc7_smob:
	SCM_ASRTGO (SCM_ARRAYP (v), badarg1);
	v = scm_ra2contig (v, 1);
	cstart = SCM_ARRAY_BASE (v);
	vlen = (SCM_ARRAY_DIMS (v)->inc
		* (SCM_ARRAY_DIMS (v)->ubnd - SCM_ARRAY_DIMS (v)->lbnd + 1));
	v = SCM_ARRAY_V (v);
	goto loop;
      case scm_tc7_string:
	base = scm_i_string_chars (v);
	sz = sizeof (char);
	break;
      case scm_tc7_bvect:
	base = (char *) SCM_BITVECTOR_BASE (v);
	vlen = (vlen + SCM_LONG_BIT - 1) / SCM_LONG_BIT;
	cstart /= SCM_LONG_BIT;
	sz = sizeof (long);
	break;
      case scm_tc7_uvect:
      case scm_tc7_ivect:
	base = (char *) SCM_UVECTOR_BASE (v);
	sz = sizeof (long);
	break;
      case scm_tc7_svect:
	base = (char *) SCM_UVECTOR_BASE (v);
	sz = sizeof (short);
	break;
#if SCM_SIZEOF_LONG_LONG != 0
      case scm_tc7_llvect:
	base = (char *) SCM_UVECTOR_BASE (v);
	sz = sizeof (long long);
	break;
#endif
      case scm_tc7_fvect:
	base = (char *) SCM_UVECTOR_BASE (v);
	sz = sizeof (float);
	break;
      case scm_tc7_dvect:
	base = (char *) SCM_UVECTOR_BASE (v);
	sz = sizeof (double);
	break;
      case scm_tc7_cvect:
	base = (char *) SCM_UVECTOR_BASE (v);
	sz = 2 * sizeof (double);
	break;
      }
  
  cend = vlen;
  if (!SCM_UNBNDP (start))
    {
      offset = 
	SCM_NUM2LONG (3, start);

      if (offset < 0 || offset >= cend)
	scm_out_of_range (FUNC_NAME, start);

      if (!SCM_UNBNDP (end))
	{
	  long tend = 
	    SCM_NUM2LONG (4, end);
      
	  if (tend <= offset || tend > cend)
	    scm_out_of_range (FUNC_NAME, end);
	  cend = tend;
	}
    }

  if (SCM_NIMP (port_or_fd))
    {
      const char *source = base + (cstart + offset) * sz;

      ans = cend - offset;
      scm_lfwrite (source, ans * sz, port_or_fd);
    }
  else /* file descriptor.  */
    {
      SCM_SYSCALL (ans = write (scm_to_int (port_or_fd),
				base + (cstart + offset) * sz,
				(sz * (cend - offset))));
      if (ans == -1)
	SCM_SYSERROR;
    }
  if (SCM_TYP7 (v) == scm_tc7_bvect)
    ans *= SCM_LONG_BIT;

  return scm_from_long (ans);
}
#undef FUNC_NAME


static char cnt_tab[16] =
{0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

SCM_DEFINE (scm_bit_count, "bit-count", 2, 0, 0,
	    (SCM b, SCM bitvector),
	    "Return the number of occurrences of the boolean @var{b} in\n"
	    "@var{bitvector}.")
#define FUNC_NAME s_scm_bit_count
{
  SCM_VALIDATE_BOOL (1, b);
  SCM_ASSERT (SCM_BITVECTOR_P (bitvector), bitvector, 2, FUNC_NAME);
  if (SCM_BITVECTOR_LENGTH (bitvector) == 0) {
    return SCM_INUM0;
  } else {
    unsigned long int count = 0;
    unsigned long int i = (SCM_BITVECTOR_LENGTH (bitvector) - 1) / SCM_LONG_BIT;
    unsigned long int w = SCM_UNPACK (SCM_VELTS (bitvector)[i]);
    if (scm_is_false (b)) {
      w = ~w;
    };
    w <<= SCM_LONG_BIT - 1 - ((SCM_BITVECTOR_LENGTH (bitvector) - 1) % SCM_LONG_BIT);
    while (1) {
      while (w) {
	count += cnt_tab[w & 0x0f];
	w >>= 4;
      }
      if (i == 0) {
	return scm_from_ulong (count);
      } else {
	--i;
	w = SCM_UNPACK (SCM_VELTS (bitvector)[i]);
	if (scm_is_false (b)) {
	  w = ~w;
	}
      }
    }
  }
}
#undef FUNC_NAME


SCM_DEFINE (scm_bit_position, "bit-position", 3, 0, 0,
           (SCM item, SCM v, SCM k),
	    "Return the index of the first occurrance of @var{item} in bit\n"
	    "vector @var{v}, starting from @var{k}.  If there is no\n"
	    "@var{item} entry between @var{k} and the end of\n"
	    "@var{bitvector}, then return @code{#f}.  For example,\n"
	    "\n"
	    "@example\n"
	    "(bit-position #t #*000101 0)  @result{} 3\n"
	    "(bit-position #f #*0001111 3) @result{} #f\n"
	    "@end example")
#define FUNC_NAME s_scm_bit_position
{
  long i, lenw, xbits, pos;
  register unsigned long w;

  SCM_VALIDATE_BOOL (1, item);
  SCM_ASSERT (SCM_BITVECTOR_P (v), v, SCM_ARG2, FUNC_NAME);
  pos = scm_to_long (k);
  SCM_ASSERT_RANGE (3, k, (pos <= SCM_BITVECTOR_LENGTH (v)) && (pos >= 0));

  if (pos == SCM_BITVECTOR_LENGTH (v))
    return SCM_BOOL_F;

  lenw = (SCM_BITVECTOR_LENGTH (v) - 1) / SCM_LONG_BIT;   /* watch for part words */
  i = pos / SCM_LONG_BIT;
  w = SCM_UNPACK (SCM_VELTS (v)[i]);
  if (scm_is_false (item))
    w = ~w;
  xbits = (pos % SCM_LONG_BIT);
  pos -= xbits;
  w = ((w >> xbits) << xbits);
  xbits = SCM_LONG_BIT - 1 - (SCM_BITVECTOR_LENGTH (v) - 1) % SCM_LONG_BIT;
  while (!0)
    {
      if (w && (i == lenw))
	w = ((w << xbits) >> xbits);
      if (w)
	while (w)
	  switch (w & 0x0f)
	    {
	    default:
	      return scm_from_long (pos);
	    case 2:
	    case 6:
	    case 10:
	    case 14:
	      return scm_from_long (pos + 1);
	    case 4:
	    case 12:
	      return scm_from_long (pos + 2);
	    case 8:
	      return scm_from_long (pos + 3);
	    case 0:
	      pos += 4;
	      w >>= 4;
	    }
      if (++i > lenw)
	break;
      pos += SCM_LONG_BIT;
      w = SCM_UNPACK (SCM_VELTS (v)[i]);
      if (scm_is_false (item))
	w = ~w;
    }
  return SCM_BOOL_F;
}
#undef FUNC_NAME


SCM_DEFINE (scm_bit_set_star_x, "bit-set*!", 3, 0, 0,
	    (SCM v, SCM kv, SCM obj),
	    "Set entries of bit vector @var{v} to @var{obj}, with @var{kv}\n"
	    "selecting the entries to change.  The return value is\n"
	    "unspecified.\n"
	    "\n"
	    "If @var{kv} is a bit vector, then those entries where it has\n"
	    "@code{#t} are the ones in @var{v} which are set to @var{obj}.\n"
	    "@var{kv} and @var{v} must be the same length.  When @var{obj}\n"
	    "is @code{#t} it's like @var{kv} is OR'ed into @var{v}.  Or when\n"
	    "@var{obj} is @code{#f} it can be seen as an ANDNOT.\n"
	    "\n"
	    "@example\n"
	    "(define bv #*01000010)\n"
	    "(bit-set*! bv #*10010001 #t)\n"
	    "bv\n"
	    "@result{} #*11010011\n"
	    "@end example\n"
	    "\n"
	    "If @var{kv} is a uniform vector of unsigned long integers, then\n"
	    "they're indexes into @var{v} which are set to @var{obj}.\n"
	    "\n"
	    "@example\n"
	    "(define bv #*01000010)\n"
	    "(bit-set*! bv #u(5 2 7) #t)\n"
	    "bv\n"
	    "@result{} #*01100111\n"
	    "@end example")
#define FUNC_NAME s_scm_bit_set_star_x
{
  register long i, k, vlen;
  SCM_ASSERT (SCM_BITVECTOR_P (v), v, SCM_ARG1, FUNC_NAME);
  SCM_ASRTGO (SCM_NIMP (kv), badarg2);
  switch SCM_TYP7 (kv)
    {
    default:
    badarg2:SCM_WRONG_TYPE_ARG (2, kv);
    case scm_tc7_uvect:
      vlen = SCM_BITVECTOR_LENGTH (v);
      if (scm_is_false (obj))
	for (i = SCM_UVECTOR_LENGTH (kv); i;)
	  {
	    k = SCM_UNPACK (SCM_VELTS (kv)[--i]);
	    if (k >= vlen)
	      scm_out_of_range (FUNC_NAME, scm_from_long (k));
	    SCM_BITVEC_CLR(v, k);
	  }
      else if (scm_is_eq (obj, SCM_BOOL_T))
	for (i = SCM_UVECTOR_LENGTH (kv); i;)
	  {
	    k = SCM_UNPACK (SCM_VELTS (kv)[--i]);
	    if (k >= vlen)
	      scm_out_of_range (FUNC_NAME, scm_from_long (k));
	    SCM_BITVEC_SET(v, k);
	  }
      else
	badarg3:SCM_WRONG_TYPE_ARG (3, obj);
      break;
    case scm_tc7_bvect:
      SCM_ASSERT (SCM_BITVECTOR_LENGTH (v) == SCM_BITVECTOR_LENGTH (kv), v, SCM_ARG1, FUNC_NAME);
      if (scm_is_false (obj))
	for (k = (SCM_BITVECTOR_LENGTH (v) + SCM_LONG_BIT - 1) / SCM_LONG_BIT; k--;)
	  SCM_BITVECTOR_BASE (v) [k] &= ~SCM_BITVECTOR_BASE (kv) [k];
      else if (scm_is_eq (obj, SCM_BOOL_T))
	for (k = (SCM_BITVECTOR_LENGTH (v) + SCM_LONG_BIT - 1) / SCM_LONG_BIT; k--;)
	  SCM_BITVECTOR_BASE (v) [k] |= SCM_BITVECTOR_BASE (kv) [k];
      else
	goto badarg3;
      break;
    }
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME


SCM_DEFINE (scm_bit_count_star, "bit-count*", 3, 0, 0,
           (SCM v, SCM kv, SCM obj),
	    "Return a count of how many entries in bit vector @var{v} are\n"
	    "equal to @var{obj}, with @var{kv} selecting the entries to\n"
	    "consider.\n"
	    "\n"
	    "If @var{kv} is a bit vector, then those entries where it has\n"
	    "@code{#t} are the ones in @var{v} which are considered.\n"
	    "@var{kv} and @var{v} must be the same length.\n"
	    "\n"
	    "If @var{kv} is a uniform vector of unsigned long integers, then\n"
	    "it's the indexes in @var{v} to consider.\n"
	    "\n"
	    "For example,\n"
	    "\n"
	    "@example\n"
	    "(bit-count* #*01110111 #*11001101 #t) @result{} 3\n"
	    "(bit-count* #*01110111 #u(7 0 4) #f)  @result{} 2\n"
	    "@end example")
#define FUNC_NAME s_scm_bit_count_star
{
  register long i, vlen, count = 0;
  register unsigned long k;
  int fObj = 0;
  
  SCM_ASSERT (SCM_BITVECTOR_P (v), v, SCM_ARG1, FUNC_NAME);
  SCM_ASRTGO (SCM_NIMP (kv), badarg2);
  switch SCM_TYP7 (kv)
    {
    default:
    badarg2:
        SCM_WRONG_TYPE_ARG (2, kv);
    case scm_tc7_uvect:
      vlen = SCM_BITVECTOR_LENGTH (v);
      if (scm_is_false (obj))
	for (i = SCM_UVECTOR_LENGTH (kv); i;)
	  {
	    k = SCM_UNPACK (SCM_VELTS (kv)[--i]);
	    if (k >= vlen)
	      scm_out_of_range (FUNC_NAME, scm_from_long (k));
	    if (!SCM_BITVEC_REF(v, k))
	      count++;
	  }
      else if (scm_is_eq (obj, SCM_BOOL_T))
	for (i = SCM_UVECTOR_LENGTH (kv); i;)
	  {
	    k = SCM_UNPACK (SCM_VELTS (kv)[--i]);
	    if (k >= vlen)
	      scm_out_of_range (FUNC_NAME, scm_from_long (k));
	    if (SCM_BITVEC_REF (v, k))
	      count++;
	  }
      else
	badarg3:SCM_WRONG_TYPE_ARG (3, obj);
      break;
    case scm_tc7_bvect:
      SCM_ASSERT (SCM_BITVECTOR_LENGTH (v) == SCM_BITVECTOR_LENGTH (kv), v, SCM_ARG1, FUNC_NAME);
      if (0 == SCM_BITVECTOR_LENGTH (v))
	return SCM_INUM0;
      SCM_ASRTGO (scm_is_bool (obj), badarg3);
      fObj = scm_is_eq (obj, SCM_BOOL_T);
      i = (SCM_BITVECTOR_LENGTH (v) - 1) / SCM_LONG_BIT;
      k = SCM_UNPACK (SCM_VELTS (kv)[i]) & (fObj ? SCM_UNPACK (SCM_VELTS (v)[i]) : ~ SCM_UNPACK (SCM_VELTS (v)[i]));
      k <<= SCM_LONG_BIT - 1 - ((SCM_BITVECTOR_LENGTH (v) - 1) % SCM_LONG_BIT);
      while (1)
	{
	  for (; k; k >>= 4)
	    count += cnt_tab[k & 0x0f];
	  if (0 == i--)
	    return scm_from_long (count);

         /* urg. repetitive (see above.) */
	  k = SCM_UNPACK (SCM_VELTS (kv)[i]) & (fObj ? SCM_UNPACK(SCM_VELTS (v)[i]) : ~SCM_UNPACK (SCM_VELTS (v)[i]));
	}
    }
  return scm_from_long (count);
}
#undef FUNC_NAME


SCM_DEFINE (scm_bit_invert_x, "bit-invert!", 1, 0, 0, 
           (SCM v),
	    "Modify the bit vector @var{v} by replacing each element with\n"
	    "its negation.")
#define FUNC_NAME s_scm_bit_invert_x
{
  long int k;

  SCM_ASSERT (SCM_BITVECTOR_P (v), v, SCM_ARG1, FUNC_NAME);

  k = SCM_BITVECTOR_LENGTH (v);
  for (k = (k + SCM_LONG_BIT - 1) / SCM_LONG_BIT; k--;)
    SCM_BITVECTOR_BASE (v) [k] = ~SCM_BITVECTOR_BASE (v) [k];

  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME


SCM 
scm_istr2bve (SCM str)
{
  size_t len = scm_i_string_length (str);
  SCM v = scm_make_uve (len, SCM_BOOL_T);
  long *data = (long *) SCM_VELTS (v);
  register unsigned long mask;
  register long k;
  register long j;
  const char *c_str = scm_i_string_chars (str);

  for (k = 0; k < (len + SCM_LONG_BIT - 1) / SCM_LONG_BIT; k++)
    {
      data[k] = 0L;
      j = len - k * SCM_LONG_BIT;
      if (j > SCM_LONG_BIT)
	j = SCM_LONG_BIT;
      for (mask = 1L; j--; mask <<= 1)
	switch (*c_str++)
	  {
	  case '0':
	    break;
	  case '1':
	    data[k] |= mask;
	    break;
	  default:
	    return SCM_BOOL_F;
	  }
    }
  return v;
}



static SCM 
ra2l (SCM ra, unsigned long base, unsigned long k)
{
  register SCM res = SCM_EOL;
  register long inc = SCM_ARRAY_DIMS (ra)[k].inc;
  register size_t i;
  if (SCM_ARRAY_DIMS (ra)[k].ubnd < SCM_ARRAY_DIMS (ra)[k].lbnd)
    return SCM_EOL;
  i = base + (1 + SCM_ARRAY_DIMS (ra)[k].ubnd - SCM_ARRAY_DIMS (ra)[k].lbnd) * inc;
  if (k < SCM_ARRAY_NDIM (ra) - 1)
    {
      do
	{
	  i -= inc;
	  res = scm_cons (ra2l (ra, i, k + 1), res);
	}
      while (i != base);
    }
  else
    do
      {
	i -= inc;
	res = scm_cons (scm_uniform_vector_ref (SCM_ARRAY_V (ra), scm_from_size_t (i)), res);
      }
    while (i != base);
  return res;
}


SCM_DEFINE (scm_array_to_list, "array->list", 1, 0, 0, 
           (SCM v),
	    "Return a list consisting of all the elements, in order, of\n"
	    "@var{array}.")
#define FUNC_NAME s_scm_array_to_list
{
  SCM res = SCM_EOL;
  register long k;

  if (scm_is_uniform_vector (v))
    return scm_uniform_vector_to_list (v);

  SCM_ASRTGO (SCM_NIMP (v), badarg1);
  switch SCM_TYP7 (v)
    {
    default:
    badarg1:SCM_WRONG_TYPE_ARG (1, v);
    case scm_tc7_smob:
      SCM_ASRTGO (SCM_ARRAYP (v), badarg1);
      return ra2l (v, SCM_ARRAY_BASE (v), 0);
    case scm_tc7_vector:
    case scm_tc7_wvect:
      return scm_vector_to_list (v);
    case scm_tc7_string:
      return scm_string_to_list (v);
    case scm_tc7_bvect:
      {
	long *data = (long *) SCM_VELTS (v);
	register unsigned long mask;
	for (k = (SCM_BITVECTOR_LENGTH (v) - 1) / SCM_LONG_BIT; k > 0; k--)
	  for (mask = 1UL << (SCM_LONG_BIT - 1); mask; mask >>= 1)
	    res = scm_cons (scm_from_bool(((long *) data)[k] & mask), res);
	for (mask = 1L << ((SCM_BITVECTOR_LENGTH (v) % SCM_LONG_BIT) - 1); mask; mask >>= 1)
	  res = scm_cons (scm_from_bool(((long *) data)[k] & mask), res);
	return res;
      }
    case scm_tc7_uvect:
      {
	unsigned long *data = (unsigned long *)SCM_VELTS(v);
	for (k = SCM_UVECTOR_LENGTH(v) - 1; k >= 0; k--)
	  res = scm_cons(scm_from_ulong (data[k]), res);
	return res;
      }
    case scm_tc7_ivect:
      {
	long *data = (long *)SCM_VELTS(v);
	for (k = SCM_UVECTOR_LENGTH(v) - 1; k >= 0; k--)
	  res = scm_cons(scm_from_long (data[k]), res);
	return res;
      }
    case scm_tc7_svect:
      {
	short *data = (short *)SCM_VELTS(v);
	for (k = SCM_UVECTOR_LENGTH(v) - 1; k >= 0; k--)
	  res = scm_cons (scm_from_short (data[k]), res);
	return res;
      }
#if SCM_SIZEOF_LONG_LONG != 0
    case scm_tc7_llvect:
      {
	long long *data = (long long *)SCM_VELTS(v);
	for (k = SCM_UVECTOR_LENGTH(v) - 1; k >= 0; k--)
	  res = scm_cons(scm_from_long_long (data[k]), res);
	return res;
      }
#endif
    case scm_tc7_fvect:
      {
	float *data = (float *) SCM_VELTS (v);
	for (k = SCM_UVECTOR_LENGTH (v) - 1; k >= 0; k--)
	  res = scm_cons (scm_from_double (data[k]), res);
	return res;
      }
    case scm_tc7_dvect:
      {
	double *data = (double *) SCM_VELTS (v);
	for (k = SCM_UVECTOR_LENGTH (v) - 1; k >= 0; k--)
	  res = scm_cons (scm_from_double (data[k]), res);
	return res;
      }
    case scm_tc7_cvect:
      {
	double (*data)[2] = (double (*)[2]) SCM_VELTS (v);
	for (k = SCM_UVECTOR_LENGTH (v) - 1; k >= 0; k--)
	  res = scm_cons (scm_c_make_rectangular (data[k][0], data[k][1]),
			  res);
	return res;
      }
    }
}
#undef FUNC_NAME


static int l2ra(SCM lst, SCM ra, unsigned long base, unsigned long k);

SCM_DEFINE (scm_list_to_uniform_array, "list->uniform-array", 3, 0, 0,
           (SCM ndim, SCM prot, SCM lst),
	    "@deffnx {Scheme Procedure} list->uniform-vector prot lst\n"
	    "Return a uniform array of the type indicated by prototype\n"
	    "@var{prot} with elements the same as those of @var{lst}.\n"
	    "Elements must be of the appropriate type, no coercions are\n"
	    "done.\n"
	    "\n"
	    "The argument @var{ndim} determines the number of dimensions\n"
	    "of the array.  It is either an exact integer, giving the\n"
	    " number directly, or a list of exact integers, whose length\n"
	    "specifies the number of dimensions and each element is the\n"
	    "lower index bound of its dimension.")
#define FUNC_NAME s_scm_list_to_uniform_array
{
  SCM shape, row;
  SCM ra;
  unsigned long k;

  shape = SCM_EOL;
  row = lst;
  if (scm_is_integer (ndim))
    {
      size_t k = scm_to_size_t (ndim);
      while (k-- > 0)
	{
	  shape = scm_cons (scm_length (row), shape);
	  if (k > 0)
	    row = scm_car (row);
	}
    }
  else
    {
      while (1)
	{
	  shape = scm_cons (scm_list_2 (scm_car (ndim),
					scm_sum (scm_sum (scm_car (ndim),
							  scm_length (row)),
						 scm_from_int (-1))),
			    shape);
	  ndim = scm_cdr (ndim);
	  if (scm_is_pair (ndim))
	    row = scm_car (row);
	  else
	    break;
	}
    }

  ra = scm_dimensions_to_uniform_array (scm_reverse_x (shape, SCM_EOL), prot,
					SCM_UNDEFINED);
  if (scm_is_null (shape))
    {
      SCM_ASRTGO (1 == scm_ilength (lst), badlst);
      scm_array_set_x (ra, SCM_CAR (lst), SCM_EOL);
      return ra;
    }
  if (!SCM_ARRAYP (ra))
    {
      unsigned long length = scm_to_ulong (scm_uniform_vector_length (ra));
      for (k = 0; k < length; k++, lst = SCM_CDR (lst))
	scm_array_set_x (ra, SCM_CAR (lst), scm_from_ulong (k));
      return ra;
    }
  if (l2ra (lst, ra, SCM_ARRAY_BASE (ra), 0))
    return ra;
  else
    badlst:SCM_MISC_ERROR ("Bad scm_array contents list: ~S",
			   scm_list_1 (lst));
}
#undef FUNC_NAME

static int 
l2ra (SCM lst, SCM ra, unsigned long base, unsigned long k)
{
  register long inc = SCM_ARRAY_DIMS (ra)[k].inc;
  register long n = (1 + SCM_ARRAY_DIMS (ra)[k].ubnd - SCM_ARRAY_DIMS (ra)[k].lbnd);
  int ok = 1;
  if (n <= 0)
    return (scm_is_null (lst));
  if (k < SCM_ARRAY_NDIM (ra) - 1)
    {
      while (n--)
	{
	  if (!scm_is_pair (lst))
	    return 0;
	  ok = ok && l2ra (SCM_CAR (lst), ra, base, k + 1);
	  base += inc;
	  lst = SCM_CDR (lst);
	}
      if (!scm_is_null (lst))
 return 0;
    }
  else
    {
      while (n--)
	{
	  if (!scm_is_pair (lst))
	    return 0;
	  scm_array_set_x (SCM_ARRAY_V (ra), SCM_CAR (lst), scm_from_ulong (base));
	  base += inc;
	  lst = SCM_CDR (lst);
	}
      if (!scm_is_null (lst))
	return 0;
    }
  return ok;
}


static void 
rapr1 (SCM ra, unsigned long j, unsigned long k, SCM port, scm_print_state *pstate)
{
  long inc = 1;
  long n = (SCM_TYP7 (ra) == scm_tc7_smob
	    ? 0
	    : scm_to_long (scm_uniform_vector_length (ra)));
  int enclosed = 0;
tail:
  switch SCM_TYP7 (ra)
    {
    case scm_tc7_smob:
      if (enclosed++)
	{
	  SCM_ARRAY_BASE (ra) = j;
	  if (n-- > 0)
	    scm_iprin1 (ra, port, pstate);
	  for (j += inc; n-- > 0; j += inc)
	    {
	      scm_putc (' ', port);
	      SCM_ARRAY_BASE (ra) = j;
	      scm_iprin1 (ra, port, pstate);
	    }
	  break;
	}
      if (k + 1 < SCM_ARRAY_NDIM (ra))
	{
	  long i;
	  inc = SCM_ARRAY_DIMS (ra)[k].inc;
	  for (i = SCM_ARRAY_DIMS (ra)[k].lbnd; i < SCM_ARRAY_DIMS (ra)[k].ubnd; i++)
	    {
	      scm_putc ('(', port);
	      rapr1 (ra, j, k + 1, port, pstate);
	      scm_puts (") ", port);
	      j += inc;
	    }
	  if (i == SCM_ARRAY_DIMS (ra)[k].ubnd)
	    {			/* could be zero size. */
	      scm_putc ('(', port);
	      rapr1 (ra, j, k + 1, port, pstate);
	      scm_putc (')', port);
	    }
	  break;
	}
      if (SCM_ARRAY_NDIM (ra) > 0)
	{			/* Could be zero-dimensional */
	  inc = SCM_ARRAY_DIMS (ra)[k].inc;
	  n = (SCM_ARRAY_DIMS (ra)[k].ubnd - SCM_ARRAY_DIMS (ra)[k].lbnd + 1);
	}
      else
	n = 1;
      ra = SCM_ARRAY_V (ra);
      goto tail;
    default:
      /* scm_tc7_bvect and scm_tc7_llvect only?  */
      if (n-- > 0)
	scm_iprin1 (scm_cvref (ra, j, SCM_UNDEFINED), port, pstate);
      for (j += inc; n-- > 0; j += inc)
	{
	  scm_putc (' ', port);
	  scm_iprin1 (scm_cvref (ra, j, SCM_UNDEFINED), port, pstate);
	}
      break;
    case scm_tc7_string:
      {
	const char *src;
	src = scm_i_string_chars (ra);
	if (n-- > 0)
	  scm_iprin1 (SCM_MAKE_CHAR (src[j]), port, pstate);
	if (SCM_WRITINGP (pstate))
	  for (j += inc; n-- > 0; j += inc)
	    {
	      scm_putc (' ', port);
	      scm_iprin1 (SCM_MAKE_CHAR (src[j]), port, pstate);
	    }
	else
	  for (j += inc; n-- > 0; j += inc)
	    scm_putc (src[j], port);
	scm_remember_upto_here_1 (ra);
      }
      break;
      
    case scm_tc7_uvect:
      {
	char str[11];
	
	if (n-- > 0)
	  {
	    /* intprint can't handle >= 2^31.  */
	    sprintf (str, "%lu", ((unsigned long *) SCM_VELTS (ra))[j]);
	    scm_puts (str, port);
	  }
	for (j += inc; n-- > 0; j += inc)
	  {
	    scm_putc (' ', port);
	    sprintf (str, "%lu", ((unsigned long *) SCM_VELTS (ra))[j]);
	    scm_puts (str, port);
	  }
      }
    case scm_tc7_ivect:
      if (n-- > 0)
	scm_intprint (((signed long *) SCM_VELTS (ra))[j], 10, port);
      for (j += inc; n-- > 0; j += inc)
	{
	  scm_putc (' ', port);
	  scm_intprint (((signed long *) SCM_VELTS (ra))[j], 10, port);
	}
      break;
      
    case scm_tc7_svect:
      if (n-- > 0)
	scm_intprint (((short *) SCM_CELL_WORD_1 (ra))[j], 10, port);
      for (j += inc; n-- > 0; j += inc)
	{
	  scm_putc (' ', port);
	  scm_intprint (((short *) SCM_CELL_WORD_1 (ra))[j], 10, port);
	}
      break;
      
    case scm_tc7_fvect:
      if (n-- > 0)
	{
	  SCM z = scm_from_double (1.0);
	  SCM_REAL_VALUE (z) = ((float *) SCM_VELTS (ra))[j];
	  scm_print_real (z, port, pstate);
	  for (j += inc; n-- > 0; j += inc)
	    {
	      scm_putc (' ', port);
	      SCM_REAL_VALUE (z) = ((float *) SCM_VELTS (ra))[j];
	      scm_print_real (z, port, pstate);
	    }
	}
      break;
    case scm_tc7_dvect:
      if (n-- > 0)
	{
	  SCM z = scm_from_double (1.0 / 3.0);
	  SCM_REAL_VALUE (z) = ((double *) SCM_VELTS (ra))[j];
	  scm_print_real (z, port, pstate);
	  for (j += inc; n-- > 0; j += inc)
	    {
	      scm_putc (' ', port);
	      SCM_REAL_VALUE (z) = ((double *) SCM_VELTS (ra))[j];
	      scm_print_real (z, port, pstate);
	    }
	}
      break;
    case scm_tc7_cvect:
      if (n-- > 0)
	{
	  SCM cz = scm_c_make_rectangular (0.0, 1.0);
	  SCM z = scm_from_double (1.0/3.0);
	  SCM_REAL_VALUE (z) =
	    SCM_COMPLEX_REAL (cz) = ((double *) SCM_VELTS (ra))[2 * j];
	  SCM_COMPLEX_IMAG (cz) = ((double *) SCM_VELTS (ra))[2 * j + 1];
	  scm_print_complex ((0.0 == SCM_COMPLEX_IMAG (cz) ? z : cz),
			     port, pstate);
	  for (j += inc; n-- > 0; j += inc)
	    {
	      scm_putc (' ', port);
	      SCM_REAL_VALUE (z)
		= SCM_COMPLEX_REAL (cz) = ((double *) SCM_VELTS (ra))[2 * j];
	      SCM_COMPLEX_IMAG (cz) = ((double *) SCM_VELTS (ra))[2 * j + 1];
	      scm_print_complex ((0.0 == SCM_COMPLEX_IMAG (cz) ? z : cz),
				 port, pstate);
	    }
	}
      break;
    }
}

/* Print dimension DIM of ARRAY.
 */

static int
scm_i_print_array_dimension (SCM array, int dim, int base,
			     SCM port, scm_print_state *pstate)
{
  scm_t_array_dim *dim_spec = SCM_ARRAY_DIMS (array) + dim;
  long idx;

  scm_putc ('(', port);

#if 0
  scm_putc ('{', port);
  scm_intprint (dim_spec->lbnd, 10, port);
  scm_putc (':', port);
  scm_intprint (dim_spec->ubnd, 10, port);
  scm_putc (':', port);
  scm_intprint (dim_spec->inc, 10, port);
  scm_putc ('}', port);
#endif

  for (idx = dim_spec->lbnd; idx <= dim_spec->ubnd; idx++)
    {
      if (dim < SCM_ARRAY_NDIM(array)-1)
	scm_i_print_array_dimension (array, dim+1, base, port, pstate);
      else
	scm_iprin1 (scm_cvref (SCM_ARRAY_V (array), base, SCM_UNDEFINED), 
		    port, pstate);
      if (idx < dim_spec->ubnd)
	scm_putc (' ', port);
      base += dim_spec->inc;
    }

  scm_putc (')', port);
  return 1;
}

static const char *
scm_i_legacy_tag (SCM v)
{
  switch (SCM_TYP7 (v))
    {
    case scm_tc7_bvect:
      return "b";
    case scm_tc7_string:
      return "a";
    case scm_tc7_uvect:
      return "u";
    case scm_tc7_ivect:
      return "e";
    case scm_tc7_svect:
      return "h";
#if SCM_SIZEOF_LONG_LONG != 0
    case scm_tc7_llvect:
      return "l";
#endif
    case scm_tc7_fvect:
      return "s";
    case scm_tc7_dvect:
      return "i";
    case scm_tc7_cvect:
      return "c";
    case scm_tc7_vector:
    case scm_tc7_wvect:
      return "";
    default:
      return "?";
    }
}

/* Print a array.  (Only for strict arrays, not for strings, uniform
   vectors, vectors and other stuff that can masquerade as an array.)
*/

/* The array tag is generally of the form
 *
 *   #<rank><unif><@lower><@lower>...
 *
 * <rank> is a positive integer in decimal giving the rank of the
 * array.  It is omitted when the rank is 1 and the array is
 * non-shared and has zero-origin.  For shared arrays and for a
 * non-zero origin, the rank is always printed even when it is 1 to
 * dinstinguish them from ordinary vectors.
 *
 * <unif> is the tag for a uniform (or homogenous) numeric vector,
 * like u8, s16, etc, as defined by SRFI-4.  It is omitted when the
 * array is not uniform.
 *
 * <@lower> is a 'at' sign followed by a integer in decimal giving the
 * lower bound of a dimension.  There is one <@lower> for each
 * dimension.  When all lower bounds are zero, all <@lower> are
 * omitted.
 *
 * Thus, 
 *
 *   #(1 2 3) is non-uniform array of rank 1 with lower bound 0 in
 *   dimension 0.  (I.e., a regular vector.)
 *
 *   #@2(1 2 3) is non-uniform array of rank 1 with lower bound 2 in
 *   dimension 0.
 *
 *   #2((1 2 3) (4 5 6)) is a non-uniform array of rank 2; a 3x3
 *   matrix with index ranges 0..2 and 0..2.
 *
 *   #u32(0 1 2) is a uniform u8 array of rank 1.
 *
 *   #2u32@2@3((1 2) (2 3)) is a uniform u8 array of rank 2 with index
 *   ranges 2..3 and 3..4.
 */

static int
scm_i_print_array (SCM array, SCM port, scm_print_state *pstate)
{
  long ndim = SCM_ARRAY_NDIM (array);
  scm_t_array_dim *dim_specs = SCM_ARRAY_DIMS (array);
  unsigned long base = SCM_ARRAY_BASE (array);
  long i;

  scm_putc ('#', port);
  if (rank != 1 || dim_specs[0].lbnd != 0)
    scm_intprint (ndim, 10, port);
  if (scm_is_uniform_vector (SCM_ARRAY_V (array)))
    scm_puts (scm_i_uniform_vector_tag (SCM_ARRAY_V (array)), port);
  else 
    scm_puts (scm_i_legacy_tag (SCM_ARRAY_V (array)), port);
  for (i = 0; i < ndim; i++)
    if (dim_specs[i].lbnd != 0)
      {
	for (i = 0; i < ndim; i++)
	  {
	    scm_putc ('@', port);
	    scm_uintprint (dim_specs[i].lbnd, 10, port);
	  }
	break;
      }

#if 0
  scm_putc ('{', port);
  scm_uintprint (base, 10, port);
  scm_putc ('}', port);
#endif

  return scm_i_print_array_dimension (array, 0, base, port, pstate);
}

/* Read an array.  This function can also read vectors and uniform
   vectors.  Also, the conflict between '#f' and '#f32' and '#f64' is
   handled here.

   C is the first character read after the '#'.
*/

typedef struct {
  const char *tag;
  SCM *proto_var;
} tag_proto;

static SCM scm_i_proc_make_vector;

static tag_proto tag_proto_table[] = {
  { "", &scm_i_proc_make_vector },
  { "u8", &scm_i_proc_make_u8vector },
  { "s8", &scm_i_proc_make_s8vector },
  { "u16", &scm_i_proc_make_u16vector },
  { "s16", &scm_i_proc_make_s16vector },
  { "u32", &scm_i_proc_make_u32vector },
  { "s32", &scm_i_proc_make_s32vector },
  { "u64", &scm_i_proc_make_u64vector },
  { "s64", &scm_i_proc_make_s64vector },
  { "f32", &scm_i_proc_make_f32vector },
  { "f64", &scm_i_proc_make_f64vector },
  { NULL, NULL }
};

static SCM
scm_i_tag_to_prototype (const char *tag, SCM port)
{
  tag_proto *tp;

  for (tp = tag_proto_table; tp->tag; tp++)
    if (!strcmp (tp->tag, tag))
      return *(tp->proto_var);
  
#if SCM_ENABLE_DEPRECATED
  {
    /* Recognize the old syntax, producing the old prototypes.
     */
    SCM proto = SCM_EOL;
    const char *instead;
    switch (tag[0])
      {
      case 'a':
	proto = SCM_MAKE_CHAR ('a');
	instead = "???";
	break;
      case 'u':
	proto = scm_from_int (1);
	instead = "u32";
	break;
      case 'e':
	proto = scm_from_int (-1);
	instead = "s32";
	break;
      case 's':
	proto = scm_from_double (1.0);
	instead = "f32";
	break;
      case 'i':
	proto = scm_divide (scm_from_int (1), scm_from_int (3));
	instead = "f64";
	break;
      case 'y':
	proto = SCM_MAKE_CHAR (0);
	instead = "s8";
	break;
      case 'h':
	proto = scm_from_locale_symbol ("s");
	instead = "s16";
	break;
      case 'l':
	proto = scm_from_locale_symbol ("l");
	instead = "s64";
	break;
      case 'c':
	proto = scm_c_make_rectangular (0.0, 1.0);
	instead = "???";
	break;
      }
    if (!scm_is_eq (proto, SCM_EOL) && tag[1] == '\0')
      {
	scm_c_issue_deprecation_warning_fmt
	  ("The tag '%c' is deprecated for uniform vectors. "
	   "Use '%s' instead.", tag[0], instead);
	return proto;
      }
  }
#endif

  scm_i_input_error (NULL, port,
		     "unrecognized uniform array tag: ~a",
		     scm_list_1 (scm_from_locale_string (tag)));
  return SCM_BOOL_F;
}

SCM
scm_i_read_array (SCM port, int c)
{
  size_t rank;
  int got_rank;
  char tag[80];
  int tag_len;

  SCM lower_bounds, elements;

  /* XXX - shortcut for ordinary vectors.  Shouldn't be necessary but
     the array code can not deal with zero-length dimensions yet, and
     we want to allow zero-length vectors, of course.
  */
  if (c == '(')
    {
      scm_ungetc (c, port);
      return scm_vector (scm_read (port));
    }

  /* Disambiguate between '#f' and uniform floating point vectors.
   */
  if (c == 'f')
    {
      c = scm_getc (port);
      if (c != '3' && c != '6')
	{
	  if (c != EOF)
	    scm_ungetc (c, port);
	  return SCM_BOOL_F;
	}
      rank = 1;
      got_rank = 1;
      tag[0] = 'f';
      tag_len = 1;
      goto continue_reading_tag;
    }

  /* Read rank.  We disallow arrays of rank zero since they do not
     seem to work reliably yet. */
  rank = 0;
  got_rank = 0;
  while ('0' <= c && c <= '9')
    {
      rank = 10*rank + c-'0';
      got_rank = 1;
      c = scm_getc (port);
    }
  if (!got_rank)
    rank = 1;
  else if (rank == 0)
    scm_i_input_error (NULL, port,
		       "array rank must be positive", SCM_EOL);

  /* Read tag. */
  tag_len = 0;
 continue_reading_tag:
  while (c != EOF && c != '(' && c != '@' && tag_len < 80)
    {
      tag[tag_len++] = c;
      c = scm_getc (port);
    }
  tag[tag_len] = '\0';
  
  /* Read lower bounds. */
  lower_bounds = SCM_EOL;
  while (c == '@')
    {
      /* Yeah, right, we should use some ready-made integer parsing
	 routine for this...
       */

      long lbnd = 0;
      long sign = 1;

      c = scm_getc (port);
      if (c == '-')
	{
	  sign = -1;
	  c = scm_getc (port);
	}
      while ('0' <= c && c <= '9')
	{
	  lbnd = 10*lbnd + c-'0';
	  c = scm_getc (port);
	}
      lower_bounds = scm_cons (scm_from_long (sign*lbnd), lower_bounds);
    }

  /* Read nested lists of elements.
   */
  if (c != '(')
    scm_i_input_error (NULL, port,
		       "missing '(' in vector or array literal",
		       SCM_EOL);
  scm_ungetc (c, port);
  elements = scm_read (port);

  if (scm_is_null (lower_bounds))
    lower_bounds = scm_from_size_t (rank);
  else if (scm_ilength (lower_bounds) != rank)
    scm_i_input_error (NULL, port,
		       "the number of lower bounds must match the array rank",
		       SCM_EOL);

  /* Construct array. */
  return scm_list_to_uniform_array (lower_bounds,
				    scm_i_tag_to_prototype (tag, port),
				    elements);
}

int 
scm_raprin1 (SCM exp, SCM port, scm_print_state *pstate)
{
  SCM v = exp;
  unsigned long base = 0;

  if (SCM_ARRAYP (exp) && !SCM_ARRAYP (SCM_ARRAY_V (exp)))
    return scm_i_print_array (exp, port, pstate);

  scm_putc ('#', port);
tail:
  switch SCM_TYP7 (v)
    {
    case scm_tc7_smob:
      {
	long ndim = SCM_ARRAY_NDIM (v);
	base = SCM_ARRAY_BASE (v);
	v = SCM_ARRAY_V (v);
	if (SCM_ARRAYP (v))

	  {
	    scm_puts ("<enclosed-array ", port);
	    rapr1 (exp, base, 0, port, pstate);
	    scm_putc ('>', port);
	    return 1;
	  }
	else
	  {
	    scm_intprint (ndim, 10, port);
	    goto tail;
	  }
      }
    case scm_tc7_bvect:
      if (scm_is_eq (exp, v))
	{			/* a uve, not an scm_array */
	  register long i, j, w;
	  scm_putc ('*', port);
	  for (i = 0; i < (SCM_BITVECTOR_LENGTH (exp)) / SCM_LONG_BIT; i++)
	    {
	      scm_t_bits w = SCM_UNPACK (SCM_VELTS (exp)[i]);
	      for (j = SCM_LONG_BIT; j; j--)
		{
		  scm_putc (w & 1 ? '1' : '0', port);
		  w >>= 1;
		}
	    }
	  j = SCM_BITVECTOR_LENGTH (exp) % SCM_LONG_BIT;
	  if (j)
	    {
	      w = SCM_UNPACK (SCM_VELTS (exp)[SCM_BITVECTOR_LENGTH (exp) / SCM_LONG_BIT]);
	      for (; j; j--)
		{
		  scm_putc (w & 1 ? '1' : '0', port);
		  w >>= 1;
		}
	    }
	  return 1;
	}
      else
	scm_putc ('b', port);
      break;
    case scm_tc7_string:
      scm_putc ('a', port);
      break;
    case scm_tc7_uvect:
      scm_putc ('u', port);
      break;
    case scm_tc7_ivect:
      scm_putc ('e', port);
      break;
    case scm_tc7_svect:
      scm_putc ('h', port);
      break;
#if SCM_SIZEOF_LONG_LONG != 0
    case scm_tc7_llvect:
      scm_putc ('l', port);
      break;
#endif
    case scm_tc7_fvect:
      scm_putc ('s', port);
      break;
    case scm_tc7_dvect:
      scm_putc ('i', port);
      break;
    case scm_tc7_cvect:
      scm_putc ('c', port);
      break;
    }
  scm_putc ('(', port);
  rapr1 (exp, base, 0, port, pstate);
  scm_putc (')', port);
  return 1;
}

SCM_DEFINE (scm_array_prototype, "array-prototype", 1, 0, 0, 
           (SCM ra),
	    "Return an object that would produce an array of the same type\n"
	    "as @var{array}, if used as the @var{prototype} for\n"
	    "@code{make-uniform-array}.")
#define FUNC_NAME s_scm_array_prototype
{
  int enclosed = 0;
  SCM_ASRTGO (SCM_NIMP (ra), badarg);
loop:
  if (scm_is_uniform_vector (ra))
    return scm_i_uniform_vector_creator (ra);
  else if (scm_is_true (scm_vector_p (ra)))
    return scm_i_proc_make_vector;

  switch SCM_TYP7 (ra)
    {
    default:
    badarg:SCM_WRONG_TYPE_ARG (1, ra);
    case scm_tc7_smob:
      SCM_ASRTGO (SCM_ARRAYP (ra), badarg);
      if (enclosed++)
	return SCM_UNSPECIFIED;
      ra = SCM_ARRAY_V (ra);
      goto loop;
    case scm_tc7_vector:
    case scm_tc7_wvect:
      return SCM_EOL;
    case scm_tc7_bvect:
      return SCM_BOOL_T;
    case scm_tc7_string:
      return SCM_MAKE_CHAR ('a');
    case scm_tc7_uvect:
      return scm_from_int (1);
    case scm_tc7_ivect:
      return scm_from_int (-1);
    case scm_tc7_svect:
      return scm_from_locale_symbol ("s");
#if SCM_SIZEOF_LONG_LONG != 0
    case scm_tc7_llvect:
      return scm_from_locale_symbol ("l");
#endif
    case scm_tc7_fvect:
      return scm_from_double (1.0);
    case scm_tc7_dvect:
      return exactly_one_third;
    case scm_tc7_cvect:
      return scm_c_make_rectangular (0.0, 1.0);
    }
}
#undef FUNC_NAME


static SCM
array_mark (SCM ptr)
{
  return SCM_ARRAY_V (ptr);
}


static size_t
array_free (SCM ptr)
{
  scm_gc_free (SCM_ARRAY_MEM (ptr),
	       (sizeof (scm_t_array) 
		+ SCM_ARRAY_NDIM (ptr) * sizeof (scm_t_array_dim)),
	       "array");
  return 0;
}

void
scm_init_unif ()
{
  scm_tc16_array = scm_make_smob_type ("array", 0);
  scm_set_smob_mark (scm_tc16_array, array_mark);
  scm_set_smob_free (scm_tc16_array, array_free);
  scm_set_smob_print (scm_tc16_array, scm_raprin1);
  scm_set_smob_equalp (scm_tc16_array, scm_array_equal_p);
  exactly_one_third = scm_permanent_object (scm_divide (scm_from_int (1),
							scm_from_int (3)));
  scm_add_feature ("array");
#include "libguile/unif.x"

  scm_i_proc_make_vector = scm_variable_ref (scm_c_lookup ("make-vector"));
  scm_i_proc_make_string = scm_variable_ref (scm_c_lookup ("make-string"));
  scm_i_proc_make_u1vector = scm_variable_ref (scm_c_lookup ("make-u1vector"));
}

/*
  Local Variables:
  c-file-style: "gnu"
  End:
*/
