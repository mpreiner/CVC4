/*********************                                                        */
/*! \file theory_bool.cpp
 ** \verbatim
 ** Original author: Morgan Deters <mdeters@cs.nyu.edu>
 ** Major contributors: Dejan Jovanović <dejan.jovanovic@gmail.com>
 ** Minor contributors (to current version): none
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2013  New York University and The University of Iowa
 ** See the file COPYING in the top-level source directory for licensing
 ** information.\endverbatim
 **
 ** \brief The theory of booleans.
 **
 ** The theory of booleans.
 **/

#include "theory/theory.h"
#include "theory/booleans/theory_bool.h"
#include "theory/booleans/circuit_propagator.h"
#include "theory/valuation.h"
#include "util/boolean_simplification.h"

#include <vector>
#include <stack>
#include "util/hash.h"

using namespace std;

namespace CVC4 {
namespace theory {
namespace booleans {

Theory::PPAssertStatus TheoryBool::ppAssert(TNode in, SubstitutionMap& outSubstitutions) {

  if (in.getKind() == kind::CONST_BOOLEAN && !in.getConst<bool>()) {
    // If we get a false literal, we're in conflict
    return PP_ASSERT_STATUS_CONFLICT;
  }

  // Add the substitution from the variable to it's value
  if (in.getKind() == kind::NOT) {
    if (in[0].getKind() == kind::VARIABLE) {
      outSubstitutions.addSubstitution(in[0], NodeManager::currentNM()->mkConst<bool>(false));
    } else {
      return PP_ASSERT_STATUS_UNSOLVED;
    }
  } else {
    if (in.getKind() == kind::VARIABLE) {
      outSubstitutions.addSubstitution(in, NodeManager::currentNM()->mkConst<bool>(true));
    } else {
      return PP_ASSERT_STATUS_UNSOLVED;
    }
  }

  return PP_ASSERT_STATUS_SOLVED;
}


}/* CVC4::theory::booleans namespace */
}/* CVC4::theory namespace */
}/* CVC4 namespace */
