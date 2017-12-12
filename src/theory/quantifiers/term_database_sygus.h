/*********************                                                        */
/*! \file term_database_sygus.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief term database sygus class
 **/

#include "cvc4_private.h"

#ifndef __CVC4__THEORY__QUANTIFIERS__TERM_DATABASE_SYGUS_H
#define __CVC4__THEORY__QUANTIFIERS__TERM_DATABASE_SYGUS_H

#include <unordered_set>

#include "theory/quantifiers/extended_rewrite.h"
#include "theory/quantifiers/sygus_explain.h"
#include "theory/quantifiers/term_database.h"

namespace CVC4 {
namespace theory {
namespace quantifiers {

class CegConjecture;

// TODO :issue #1235 split and document this class
class TermDbSygus {
 public:
  TermDbSygus(context::Context* c, QuantifiersEngine* qe);
  ~TermDbSygus() {}
  /** Reset this utility */
  bool reset(Theory::Effort e);
  /** Identify this utility */
  std::string identify() const { return "TermDbSygus"; }
  /** register the sygus type */
  void registerSygusType(TypeNode tn);
  /** register a variable e that we will do enumerative search on
   * conj is the conjecture that the enumeration of e is for.
   * f is the synth-fun that the enumeration of e is for.
   * mkActiveGuard is whether we want to make an active guard for e 
   * (see d_enum_to_active_guard).
   *
   * Notice that enumerator e may not be equivalent
   * to f in synthesis-through-unification approaches
   * (e.g. decision tree construction for PBE synthesis).
   */
  void registerEnumerator(Node e,
                          Node f,
                          CegConjecture* conj,
                          bool mkActiveGuard = false);
  /** is e an enumerator? */
  bool isEnumerator(Node e) const;
  /** return the conjecture e is associated with */
  CegConjecture* getConjectureForEnumerator(Node e);
  /** return the function-to-synthesize e is associated with */
  Node getSynthFunForEnumerator(Node e);
  /** get active guard for e */
  Node getActiveGuardForEnumerator(Node e);
  /** get all registered enumerators */
  void getEnumerators(std::vector<Node>& mts);
  /** get the explanation utility */
  SygusExplain* getExplain() { return d_syexp.get(); }
  /** get the extended rewrite utility */
  ExtendedRewriter* getExtRewriter() { return d_ext_rw.get(); }
 private:
  /** reference to the quantifiers engine */
  QuantifiersEngine* d_quantEngine;
  /** sygus explanation */
  std::unique_ptr<SygusExplain> d_syexp;
  /** sygus explanation */
  std::unique_ptr<ExtendedRewriter> d_ext_rw;
  /** mapping from enumerator terms to the conjecture they are associated with
   */
  std::map<Node, CegConjecture*> d_enum_to_conjecture;
  /** mapping from enumerator terms to the function-to-synthesize they are
   * associated with 
   */
  std::map<Node, Node> d_enum_to_synth_fun;
  /** mapping from enumerator terms to the guard they are associated with
   * The guard G for an enumerator e has the semantics
   *   if G is true, then there are more values of e to enumerate".
   */
  std::map<Node, Node> d_enum_to_active_guard;

  // TODO :issue #1235 : below here needs refactor

 public:
  Node d_true;
  Node d_false;

 private:
  std::map< TypeNode, std::vector< Node > > d_fv[2];
  std::map< Node, TypeNode > d_fv_stype;
  std::map< Node, int > d_fv_num;
  bool hasFreeVar( Node n, std::map< Node, bool >& visited );
public:
  TNode getFreeVar( TypeNode tn, int i, bool useSygusType = false );
  TNode getFreeVarInc( TypeNode tn, std::map< TypeNode, int >& var_count, bool useSygusType = false );
  bool isFreeVar( Node n ) { return d_fv_stype.find( n )!=d_fv_stype.end(); }
  int getVarNum( Node n ) { return d_fv_num[n]; }
  bool hasFreeVar( Node n );
private:
  std::map< TypeNode, std::map< int, Node > > d_generic_base;
  std::map< TypeNode, std::vector< Node > > d_generic_templ;
  bool getMatch( Node p, Node n, std::map< int, Node >& s );
  bool getMatch2( Node p, Node n, std::map< int, Node >& s, std::vector< int >& new_s );
public:
  bool getMatch( Node n, TypeNode st, int& index_found, std::vector< Node >& args, int index_exc = -1, int index_start = 0 );
private:
  void computeMinTypeDepthInternal( TypeNode root_tn, TypeNode tn, unsigned type_depth );
  bool involvesDivByZero( Node n, std::map< Node, bool >& visited );

 private:
  // information for sygus types
  std::map<TypeNode, TypeNode> d_register;  // stores sygus -> builtin type
  std::map<TypeNode, std::vector<Node> > d_var_list;
  std::map<TypeNode, std::map<int, Kind> > d_arg_kind;
  std::map<TypeNode, std::map<Kind, int> > d_kinds;
  std::map<TypeNode, std::map<int, Node> > d_arg_const;
  std::map<TypeNode, std::map<Node, int> > d_consts;
  std::map<TypeNode, std::map<Node, int> > d_ops;
  std::map<TypeNode, std::map<int, Node> > d_arg_ops;
  std::map<TypeNode, std::vector<int> > d_id_funcs;
  std::map<TypeNode, std::vector<Node> >
      d_const_list;  // sorted list of constants for type
  std::map<TypeNode, unsigned> d_const_list_pos;
  std::map<TypeNode, std::map<Node, Node> > d_semantic_skolem;
  // normalized map
  std::map<TypeNode, std::map<Node, Node> > d_normalized;
  std::map<TypeNode, std::map<Node, Node> > d_sygus_to_builtin;
  std::map<TypeNode, std::map<Node, Node> > d_builtin_const_to_sygus;
  // grammar information
  // root -> type -> _
  std::map<TypeNode, std::map<TypeNode, unsigned> > d_min_type_depth;
  // std::map< TypeNode, std::map< Node, std::map< std::map< int, bool > > >
  // d_consider_const;
  // type -> cons -> _
  std::map<TypeNode, unsigned> d_min_term_size;
  std::map<TypeNode, std::map<unsigned, unsigned> > d_min_cons_term_size;
  /** a cache for getSelectorWeight */
  std::map<TypeNode, std::map<Node, unsigned> > d_sel_weight;

 public:  // general sygus utilities
  bool isRegistered( TypeNode tn );
  // get the minimum depth of type in its parent grammar
  unsigned getMinTypeDepth( TypeNode root_tn, TypeNode tn );
  // get the minimum size for a constructor term
  unsigned getMinTermSize( TypeNode tn );
  unsigned getMinConsTermSize( TypeNode tn, unsigned cindex );
  /** get the weight of the selector, where tn is the domain of sel */
  unsigned getSelectorWeight(TypeNode tn, Node sel);

 public:
  TypeNode sygusToBuiltinType( TypeNode tn );
  int getKindConsNum( TypeNode tn, Kind k );
  int getConstConsNum( TypeNode tn, Node n );
  int getOpConsNum( TypeNode tn, Node n );
  bool hasKind( TypeNode tn, Kind k );
  bool hasConst( TypeNode tn, Node n );
  bool hasOp( TypeNode tn, Node n );
  Node getConsNumConst( TypeNode tn, int i );
  Node getConsNumOp( TypeNode tn, int i );
  Kind getConsNumKind( TypeNode tn, int i );
  bool isKindArg( TypeNode tn, int i );
  bool isConstArg( TypeNode tn, int i );
  unsigned getNumIdFuncs( TypeNode tn );
  unsigned getIdFuncIndex( TypeNode tn, unsigned i );
  /** get arg type */
  TypeNode getArgType( const DatatypeConstructor& c, int i );
  /** get first occurrence */
  int getFirstArgOccurrence( const DatatypeConstructor& c, TypeNode tn );
  /** is type match */
  bool isTypeMatch( const DatatypeConstructor& c1, const DatatypeConstructor& c2 );

  TypeNode getSygusTypeForVar( Node v );
  Node getGenericBase( TypeNode tn, const Datatype& dt, int c );
  Node mkGeneric( const Datatype& dt, int c, std::map< TypeNode, int >& var_count, std::map< int, Node >& pre );
  Node sygusToBuiltin( Node n, TypeNode tn );
  Node sygusToBuiltin( Node n ) { return sygusToBuiltin( n, n.getType() ); }
  Node sygusSubstituted( TypeNode tn, Node n, std::vector< Node >& args );
  Node builtinToSygusConst( Node c, TypeNode tn, int rcons_depth = 0 );
  Node getSygusNormalized( Node n, std::map< TypeNode, int >& var_count, std::map< Node, Node >& subs );
  Node getNormalized(TypeNode t, Node prog);
  unsigned getSygusTermSize( Node n );
  // returns size
  unsigned getSygusConstructors( Node n, std::vector< Node >& cons );
  /** given a term, construct an equivalent smaller one that respects syntax */
  Node minimizeBuiltinTerm( Node n );
  /** given a term, expand it into more basic components */
  Node expandBuiltinTerm( Node n );
  /** get comparison kind */
  Kind getComparisonKind( TypeNode tn );
  Kind getPlusKind( TypeNode tn, bool is_neg = false );
  bool doCompare( Node a, Node b, Kind k );
  // get semantic skolem for n (a sygus term whose builtin version is n)
  Node getSemanticSkolem( TypeNode tn, Node n, bool doMk = true );
  /** involves div-by-zero */
  bool involvesDivByZero( Node n );
  
  /** get operator kind */
  static Kind getOperatorKind( Node op );

  /** get anchor */
  static Node getAnchor( Node n );
  static unsigned getAnchorDepth( Node n );
  
public: // for symmetry breaking
  bool considerArgKind( TypeNode tn, TypeNode tnp, Kind k, Kind pk, int arg );
  bool considerConst( TypeNode tn, TypeNode tnp, Node c, Kind pk, int arg );
  bool considerConst( const Datatype& pdt, TypeNode tnp, Node c, Kind pk, int arg );
  int solveForArgument( TypeNode tnp, unsigned cindex, unsigned arg );
  
//for eager instantiation
  // TODO (as part of #1235) move some of these functions to sygus_explain.h
 private:
  /** the set of evaluation terms we have already processed */
  std::unordered_set<Node, NodeHashFunction> d_eval_processed;
  std::map< Node, std::map< Node, bool > > d_subterms;
  std::map< Node, std::vector< Node > > d_evals;
  std::map< Node, std::vector< std::vector< Node > > > d_eval_args;
  std::map< Node, std::vector< bool > > d_eval_args_const;
  std::map< Node, std::map< Node, unsigned > > d_node_mv_args_proc;

public:
  void registerEvalTerm( Node n );
  void registerModelValue( Node n, Node v, std::vector< Node >& exps, std::vector< Node >& terms, std::vector< Node >& vals );
  Node unfold( Node en, std::map< Node, Node >& vtm, std::vector< Node >& exp, bool track_exp = true );
  Node unfold( Node en ){
    std::map< Node, Node > vtm;
    std::vector< Node > exp;
    return unfold( en, vtm, exp, false );
  }
  Node getEagerUnfold( Node n, std::map< Node, Node >& visited );

  // builtin evaluation, returns rewrite( bn [ args / vars(tn) ] )
  Node evaluateBuiltin( TypeNode tn, Node bn, std::vector< Node >& args );
  // evaluate with unfolding
  Node evaluateWithUnfolding(
      Node n, std::unordered_map<Node, Node, NodeHashFunction>& visited);
  Node evaluateWithUnfolding( Node n );
//for calculating redundant operators
private:
  //whether each constructor is redundant
  // 0 : not redundant, 1 : redundant, 2 : partially redundant
  std::map< TypeNode, std::vector< int > > d_sygus_red_status;
  // type to (rewritten) to original
  std::map< TypeNode, std::map< Node, Node > > d_gen_terms;
  std::map< TypeNode, std::map< Node, bool > > d_gen_redundant;
  //compute generic redundant
  bool computeGenericRedundant( TypeNode tn, Node g );
public:
  bool isGenericRedundant( TypeNode tn, unsigned i );
};

}/* CVC4::theory::quantifiers namespace */
}/* CVC4::theory namespace */
}/* CVC4 namespace */

#endif /* __CVC4__THEORY__QUANTIFIERS__TERM_DATABASE_H */
