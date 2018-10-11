/*********************                                                        */
/*! \file eager_bitblaster.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Mathias Preiner, Andres Noetzli
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2018 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Bitblaster for eager BV solver.
 **
 ** Bitblaster for the eager BV solver.
 **/

#include "cvc4_private.h"

#ifndef __CVC4__THEORY__BV__BITBLAST__EAGER_BITBLASTER_H
#define __CVC4__THEORY__BV__BITBLAST__EAGER_BITBLASTER_H

#include <unordered_set>

#include "options/bv_bitblast_mode.h"
#include "prop/cnf_stream.h"
#include "prop/sat_solver.h"
#include "theory/bv/bitblast/bitblaster.h"

namespace CVC4 {
namespace theory {
namespace bv {

class BitblastingRegistrar;
class TheoryBV;

class EagerBitblaster : public TBitblaster<Node>
{
 public:
  EagerBitblaster(TheoryBV* theory_bv,
                  context::Context* context,
                  bv::SatSolverMode sat_solver);
  ~EagerBitblaster();

  void addAtom(TNode atom);
  void makeVariable(TNode node, Bits& bits) override;
  void bbTerm(TNode node, Bits& bits) override;
  void bbAtom(TNode node) override;
  Node getBBAtom(TNode node) const override;
  bool hasBBAtom(TNode atom) const override;
  void bbFormula(TNode formula, bool assert_formula = true);
  void storeBBAtom(TNode atom, Node atom_bb) override;
  void storeBBTerm(TNode node, const Bits& bits) override;

  bool assertToSat(TNode node, bool propagate = true);
  bool solve();
  bool solve(const std::vector<Node>& assumptions);
  std::vector<Node> getUnsatAssumptions(void);
  bool collectModelInfo(TheoryModel* m, bool fullModel);
  void setProofLog(BitVectorProof* bvp);

 private:
  context::Context* d_context;
  std::unique_ptr<context::Context> d_nullContext;

  typedef std::unordered_set<TNode, TNodeHashFunction> TNodeSet;
  // sat solver used for bitblasting and associated CnfStream
  std::unique_ptr<prop::SatSolver> d_satSolver;
  std::unique_ptr<BitblastingRegistrar> d_bitblastingRegistrar;
  std::unique_ptr<prop::CnfStream> d_cnfStream;

  TheoryBV* d_bv;
  TNodeSet d_bbAtoms;
  TNodeSet d_variables;

  // This is either an MinisatEmptyNotify or NULL.
  std::unique_ptr<MinisatEmptyNotify> d_notify;

  Node getModelFromSatSolver(TNode a, bool fullModel) override;
  bool isSharedTerm(TNode node);
};

class BitblastingRegistrar : public prop::Registrar
{
 public:
  BitblastingRegistrar(EagerBitblaster* bb) : d_bitblaster(bb) {}
  void preRegister(Node n) override { d_bitblaster->bbAtom(n); }

 private:
  EagerBitblaster* d_bitblaster;
};

}  // namespace bv
}  // namespace theory
}  // namespace CVC4
#endif  //  __CVC4__THEORY__BV__BITBLAST__EAGER_BITBLASTER_H
