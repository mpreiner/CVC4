/*********************                                                        */
/*! \file cvc4_public.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Morgan Deters, Mathias Preiner
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2021 by the authors listed in the file AUTHORS
 ** in the top-level source directory and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Macros that should be defined everywhere during the building of
 ** the libraries and driver binary, and also exported to the user.
 **
 ** Macros that should be defined everywhere during the building of
 ** the libraries and driver binary, and also exported to the user.
 **/

#ifndef _H
#define _H

#include <stddef.h>
#include <stdint.h>

// CVC4_UNUSED is to mark something (e.g. local variable, function)
// as being _possibly_ unused, so that the compiler generates no
// warning about it.  This might be the case for e.g. a variable
// only used in DEBUG builds.

#ifdef __GNUC__
#  define CVC4_UNUSED __attribute__((__unused__))
#  define CVC4_NORETURN __attribute__ ((__noreturn__))
#  define CVC4_CONST_FUNCTION __attribute__ ((__const__))
#  define CVC4_PURE_FUNCTION __attribute__ ((__pure__))
#  define CVC4_DEPRECATED __attribute__ ((__deprecated__))
#  define CVC4_WARN_UNUSED_RESULT __attribute__ ((__warn_unused_result__))
#else /* ! __GNUC__ */
#  define CVC4_UNUSED
#  define CVC4_NORETURN
#  define CVC4_CONST_FUNCTION
#  define CVC4_PURE_FUNCTION
#  define CVC4_DEPRECATED
#  define CVC4_WARN_UNUSED_RESULT
#endif /* __GNUC__ */

#endif /* _H */
