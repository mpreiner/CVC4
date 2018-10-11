/*********************                                                        */
/*! \file eager_bitblaster.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Liana Hadarean, Mathias Preiner, Tim King
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2018 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief
 **
 ** Bitblaster for the eager bv solver.
 **/

#include "cvc4_private.h"

#include "theory/bv/bitblast/eager_bitblaster.h"

#include "options/bv_options.h"
#include "proof/bitvector_proof.h"
#include "prop/cnf_stream.h"
#include "prop/sat_solver_factory.h"
#include "smt/smt_statistics_registry.h"
#include "theory/bv/theory_bv.h"
#include "theory/theory_model.h"

namespace CVC4 {
namespace theory {
namespace bv {

EagerBitblaster::EagerBitblaster(TheoryBV* theory_bv,
                                 context::Context* c,
                                 bv::SatSolverMode sat_solver)
    : TBitblaster<Node>(),
      d_context(c),
      d_nullContext(new context::Context()),
      d_satSolver(),
      d_bitblastingRegistrar(new BitblastingRegistrar(this)),
      d_cnfStream(),
      d_bv(theory_bv),
      d_bbAtoms(),
      d_variables(),
      d_notify()
{
  prop::SatSolver *solver = nullptr;
  switch (sat_solver)
  {
    case SAT_SOLVER_MINISAT:
    {
      prop::BVSatSolverInterface* minisat =
          prop::SatSolverFactory::createMinisat(
              d_nullContext.get(), smtStatisticsRegistry(), "EagerBitblaster");
      d_notify.reset(new MinisatEmptyNotify());
      minisat->setNotify(d_notify.get());
      solver = minisat;
      break;
    }
    case SAT_SOLVER_CADICAL:
      solver = prop::SatSolverFactory::createCadical(smtStatisticsRegistry(),
                                                     "EagerBitblaster");
      break;
    case SAT_SOLVER_CRYPTOMINISAT:
      solver = prop::SatSolverFactory::createCryptoMinisat(
          smtStatisticsRegistry(), "EagerBitblaster");
      break;
    default: Unreachable("Unknown SAT solver type");
  }
  d_satSolver.reset(solver);
  d_cnfStream.reset(
      new prop::TseitinCnfStream(d_satSolver.get(),
                                 d_bitblastingRegistrar.get(),
                                 d_nullContext.get(),
                                 options::proof(),
                                 "EagerBitblaster"));
}

EagerBitblaster::~EagerBitblaster() {}

void EagerBitblaster::bbFormula(TNode node, bool assert_formula)
{
  if (assert_formula)
  {
    d_cnfStream->convertAndAssert(
        node, false, false, RULE_INVALID, TNode::null());
  }
  else
  {
    d_cnfStream->ensureLiteral(node);
  }
}

/**
 * Bitblasts the atom, assigns it a marker literal, adding it to the SAT solver
 * NOTE: duplicate clauses are not detected because of marker literal
 * @param node the atom to be bitblasted
 *
 */
void EagerBitblaster::bbAtom(TNode node)
{
  node = node.getKind() == kind::NOT ? node[0] : node;
  if (node.getKind() == kind::BITVECTOR_BITOF) return;
  if (hasBBAtom(node))
  {
    return;
  }

  Debug("bitvector-bitblast") << "Bitblasting node " << node << "\n";

  // the bitblasted definition of the atom
  Node normalized = Rewriter::rewrite(node);
  Node atom_bb =
      normalized.getKind() != kind::CONST_BOOLEAN
          ? d_atomBBStrategies[normalized.getKind()](normalized, this)
          : normalized;

  if (!options::proof())
  {
    atom_bb = Rewriter::rewrite(atom_bb);
  }

  // asserting that the atom is true iff the definition holds
  Node atom_definition =
      NodeManager::currentNM()->mkNode(kind::EQUAL, node, atom_bb);

  storeBBAtom(node, atom_bb);
  d_cnfStream->convertAndAssert(
      atom_definition, false, false, RULE_INVALID, TNode::null());
}

void EagerBitblaster::storeBBAtom(TNode atom, Node atom_bb) {
  if (d_bvp) {
    d_bvp->registerAtomBB(atom.toExpr(), atom_bb.toExpr());
  }
  d_bbAtoms.insert(atom);
}

void EagerBitblaster::storeBBTerm(TNode node, const Bits& bits) {
  if (d_bvp) {
    d_bvp->registerTermBB(node.toExpr());
  }
  d_termCache.insert(std::make_pair(node, bits));
}

bool EagerBitblaster::hasBBAtom(TNode atom) const {
  return d_bbAtoms.find(atom) != d_bbAtoms.end();
}

void EagerBitblaster::bbTerm(TNode node, Bits& bits) {
  Assert(node.getType().isBitVector());

  if (hasBBTerm(node)) {
    getBBTerm(node, bits);
    return;
  }

  d_bv->spendResource(options::bitblastStep());
  Debug("bitvector-bitblast") << "Bitblasting node " << node << "\n";

  d_termBBStrategies[node.getKind()](node, bits, this);

  Assert(bits.size() == utils::getSize(node));

  storeBBTerm(node, bits);
}

void EagerBitblaster::makeVariable(TNode var, Bits& bits) {
  Assert(bits.size() == 0);
  for (unsigned i = 0; i < utils::getSize(var); ++i) {
    bits.push_back(utils::mkBitOf(var, i));
  }
  d_variables.insert(var);
}

Node EagerBitblaster::getBBAtom(TNode node) const { return node; }

/**
 * Calls the solve method for the Sat Solver.
 *
 * @return true for sat, and false for unsat
 */

bool EagerBitblaster::solve() {
  if (Trace.isOn("bitvector")) {
    Trace("bitvector") << "EagerBitblaster::solve(). \n";
  }
  Debug("bitvector") << "EagerBitblaster::solve(). \n";
  // TODO: clear some memory
  // if (something) {
  //   NodeManager* nm= NodeManager::currentNM();
  //   Rewriter::garbageCollect();
  //   nm->reclaimZombiesUntil(options::zombieHuntThreshold());
  // }
  return prop::SAT_VALUE_TRUE == d_satSolver->solve();
}

bool EagerBitblaster::solve(const std::vector<Node>& assumptions)
{
  std::vector<prop::SatLiteral> assumpts;
  for (const Node& assumption : assumptions)
  {
    Assert(d_cnfStream->hasLiteral(assumption));
    assumpts.push_back(d_cnfStream->getLiteral(assumption));
  }
  return prop::SAT_VALUE_TRUE == d_satSolver->solve(assumpts);
}

std::vector<Node> EagerBitblaster::getUnsatAssumptions()
{
  std::vector<prop::SatLiteral> unsat_assumpts =
      d_satSolver->getUnsatAssumptions();
  std::vector<Node> result;

  for (const prop::SatLiteral& lit : unsat_assumpts)
  {
    result.push_back(d_cnfStream->getNode(lit));
  }
  return result;
}

/**
 * Returns the value a is currently assigned to in the SAT solver
 * or null if the value is completely unassigned.
 *
 * @param a
 * @param fullModel whether to create a "full model," i.e., add
 * constants to equivalence classes that don't already have them
 *
 * @return
 */
Node EagerBitblaster::getModelFromSatSolver(TNode a, bool fullModel) {
  if (!hasBBTerm(a)) {
    return fullModel ? utils::mkConst(utils::getSize(a), 0u) : Node();
  }

  Bits bits;
  getBBTerm(a, bits);
  Integer value(0);
  for (int i = bits.size() - 1; i >= 0; --i) {
    prop::SatValue bit_value;
    if (d_cnfStream->hasLiteral(bits[i])) {
      prop::SatLiteral bit = d_cnfStream->getLiteral(bits[i]);
      bit_value = d_satSolver->value(bit);
      Assert(bit_value != prop::SAT_VALUE_UNKNOWN);
    } else {
      if (!fullModel) return Node();
      // unconstrained bits default to false
      bit_value = prop::SAT_VALUE_FALSE;
    }
    Integer bit_int =
        bit_value == prop::SAT_VALUE_TRUE ? Integer(1) : Integer(0);
    value = value * 2 + bit_int;
  }
  return utils::mkConst(bits.size(), value);
}

bool EagerBitblaster::collectModelInfo(TheoryModel* m, bool fullModel)
{
  std::set<Node> termSet;
  d_bv->computeRelevantTerms(termSet);
  for (TNode var : termSet)
  {
    // not actually a leaf of the bit-vector theory
    if (d_variables.find(var) == d_variables.end()) continue;

    Assert(Theory::theoryOf(var) == theory::THEORY_BV || isSharedTerm(var));
    // only shared terms could not have been bit-blasted
    Assert(hasBBTerm(var) || isSharedTerm(var));

    Node const_value = getModelFromSatSolver(var, true);
    Assert(const_value.isNull() || const_value.isConst());
    if (const_value != Node())
    {
      Debug("bitvector-model")
          << "TLazyBitblaster::collectModelInfo (assert (= " << var << " "
          << const_value << "))\n";
      if (!m->assertEquality(var, const_value, true))
      {
        return false;
      }
    }
  }
  return true;
}

void EagerBitblaster::setProofLog(BitVectorProof* bvp) {
  d_bvp = bvp;
  d_satSolver->setProofLog(bvp);
  bvp->initCnfProof(d_cnfStream.get(), d_nullContext.get());
}

bool EagerBitblaster::isSharedTerm(TNode node) {
  return d_bv->d_sharedTermsSet.find(node) != d_bv->d_sharedTermsSet.end();
}


}  // namespace bv
}  // namespace theory
}  // namespace CVC4
