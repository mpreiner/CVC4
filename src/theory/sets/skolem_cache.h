/******************************************************************************
 * Top contributors (to current version):
 *   Andrew Reynolds
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2021 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * A cache of skolems for theory of sets.
 */

#include "cvc5_private.h"

#ifndef CVC5__THEORY__SETS__SKOLEM_CACHE_H
#define CVC5__THEORY__SETS__SKOLEM_CACHE_H

#include <map>
#include <unordered_set>

#include "expr/node.h"

namespace cvc5 {
namespace theory {
namespace sets {

/**
 * A cache of all set skolems generated by the TheorySets class. This
 * cache is used to ensure that duplicate skolems are not generated when
 * possible, and helps identify what skolems were allocated in the current run.
 */
class SkolemCache
{
 public:
  SkolemCache();
  /** Identifiers for skolem types
   *
   * The comments below document the properties of each skolem introduced by
   * inference in the sets solver, where by skolem we mean the fresh
   * set variable that witnesses each of "exists k".
   */
  enum SkolemId
  {
    // exists k. k = a
    SK_PURIFY,
    // a != b => exists k. ( k in a != k in b )
    SK_DISEQUAL,
    // a in tclosure(b) => exists k1 k2. ( a.1, k1 ) in b ^ ( k2, a.2 ) in b ^
    //                                   ( k1 = k2 V ( k1, k2 ) in tclosure(b) )
    SK_TCLOSURE_DOWN1,
    SK_TCLOSURE_DOWN2,
    // (a,b) in join(A,B) => exists k. (a,k) in A ^ (k,b) in B
    // This is cached by the nodes corresponding to (a,b) and join(A,B).
    SK_JOIN,
  };

  /**
   * Makes a skolem of type tn that is cached based on the key (a,b,id).
   * Argument c is the variable name of the skolem.
   */
  Node mkTypedSkolemCached(
      TypeNode tn, Node a, Node b, SkolemId id, const char* c);
  /** same as above, cached based on key (a,null,id) */
  Node mkTypedSkolemCached(TypeNode tn, Node a, SkolemId id, const char* c);
  /** Same as above, but without caching. */
  Node mkTypedSkolem(TypeNode tn, const char* c);
  /** Returns true if n is a skolem allocated by this class */
  bool isSkolem(Node n) const;

 private:
  /** map from node pairs and identifiers to skolems */
  std::map<Node, std::map<Node, std::map<SkolemId, Node> > > d_skolemCache;
  /** the set of all skolems we have generated */
  std::unordered_set<Node> d_allSkolems;
};

}  // namespace sets
}  // namespace theory
}  // namespace cvc5

#endif /* CVC5__THEORY__STRINGS__SKOLEM_CACHE_H */
