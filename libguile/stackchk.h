/* classes: h_files */

#ifndef SCM_STACKCHK_H
#define SCM_STACKCHK_H

/* Copyright (C) 1995,1996,1998,2000 Free Software Foundation, Inc.
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



#include "libguile/__scm.h"

#include "libguile/continuations.h"
#include "libguile/debug.h"



/* With debug options we have the possibility to disable stack checking.
 */
#define SCM_STACK_CHECKING_P SCM_STACK_LIMIT

#ifdef STACK_CHECKING
# if SCM_STACK_GROWS_UP
#  define SCM_STACK_OVERFLOW_P(s)\
   (s > ((SCM_STACKITEM *) SCM_BASE (scm_rootcont) + SCM_STACK_LIMIT))
# else
#  define SCM_STACK_OVERFLOW_P(s)\
   (s < ((SCM_STACKITEM *) SCM_BASE (scm_rootcont) - SCM_STACK_LIMIT))
# endif
# define SCM_CHECK_STACK\
    {\
       SCM_STACKITEM stack;\
       if (SCM_STACK_OVERFLOW_P (&stack) && scm_stack_checking_enabled_p)\
	 scm_report_stack_overflow ();\
    }
#else
# define SCM_CHECK_STACK /**/
#endif /* STACK_CHECKING */

SCM_API int scm_stack_checking_enabled_p;



SCM_API void scm_report_stack_overflow (void);
SCM_API long scm_stack_size (SCM_STACKITEM *start);
SCM_API void scm_stack_report (void);
SCM_API void scm_init_stackchk (void);

#endif  /* SCM_STACKCHK_H */

/*
  Local Variables:
  c-file-style: "gnu"
  End:
*/
