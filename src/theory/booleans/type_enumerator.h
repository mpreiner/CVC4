/*********************                                                        */
/*! \file type_enumerator.h
 ** \verbatim
 ** Original author: Morgan Deters <mdeters@cs.nyu.edu>
 ** Major contributors: none
 ** Minor contributors (to current version): none
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2013  New York University and The University of Iowa
 ** See the file COPYING in the top-level source directory for licensing
 ** information.\endverbatim
 **
 ** \brief An enumerator for Booleans
 **
 ** An enumerator for Booleans.
 **/

#include "cvc4_private.h"

#ifndef __CVC4__THEORY__BOOLEANS__TYPE_ENUMERATOR_H
#define __CVC4__THEORY__BOOLEANS__TYPE_ENUMERATOR_H

#include "theory/type_enumerator.h"
#include "expr/type_node.h"
#include "expr/kind.h"

namespace CVC4 {
namespace theory {
namespace booleans {

class BooleanEnumerator : public TypeEnumeratorBase<BooleanEnumerator> {
  enum { FALSE, TRUE, DONE } d_value;

public:

  BooleanEnumerator(TypeNode type) :
    TypeEnumeratorBase<BooleanEnumerator>(type),
    d_value(FALSE) {
    Assert(type.getKind() == kind::TYPE_CONSTANT &&
           type.getConst<TypeConstant>() == BOOLEAN_TYPE);
  }

  Node operator*() throw(NoMoreValuesException) {
    switch(d_value) {
    case FALSE:
      return NodeManager::currentNM()->mkConst(false);
    case TRUE:
      return NodeManager::currentNM()->mkConst(true);
    default:
      throw NoMoreValuesException(getType());
    }
  }

  BooleanEnumerator& operator++() throw() {
    // sequence is FALSE, TRUE
    if(d_value == FALSE) {
      d_value = TRUE;
    } else {
      d_value = DONE;
    }
    return *this;
  }

  bool isFinished() throw() {
    return d_value == DONE;
  }

};/* class BooleanEnumerator */

}/* CVC4::theory::booleans namespace */
}/* CVC4::theory namespace */
}/* CVC4 namespace */

#endif /* __CVC4__THEORY__BOOLEANS__TYPE_ENUMERATOR_H */
