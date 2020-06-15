/*********************                                                        */
/*! \file theory_preprocessor.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Dejan Jovanovic, Morgan Deters, Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief The theory preprocessor
 **/

#include "theory/theory_preprocessor.h"

#include "expr/lazy_proof.h"
#include "expr/skolem_manager.h"
#include "theory/logic_info.h"
#include "theory/rewriter.h"
#include "theory/theory_engine.h"

using namespace std;

namespace CVC4 {
namespace theory {

TheoryPreprocessor::TheoryPreprocessor(TheoryEngine& engine,
                                       RemoveTermFormulas& tfr,
                                       ProofNodeManager* pnm)
    : d_engine(engine),
      d_logicInfo(engine.getLogicInfo()),
      d_ppCache(),
      d_tfr(tfr),
      d_tpg(pnm ? new TConvProofGenerator(pnm) : nullptr)
{
}

TheoryPreprocessor::~TheoryPreprocessor() {}

void TheoryPreprocessor::clearCache()
{
  d_ppCache.clear();
  // TODO: clear rewrites from d_tpg
}

TrustNode TheoryPreprocessor::preprocess(TNode node,
                       std::vector<TrustNode>& newLemmas,
                       std::vector<Node>& newSkolems,
                       bool doTheoryPreprocess)
{
  // Run theory preprocessing, maybe
  Node retNode = node;
  if (doTheoryPreprocess)
  {
    // run theory preprocessing
    TrustNode trn = theoryPreprocess(node);
    retNode = trn.getNode();
  }

  // Remove the ITEs
  Trace("te-tform-rm") << "Remove term formulas from " << retNode << std::endl;
  TrustNode tret = d_tfr.run(retNode, newLemmas, newSkolems, false);
  Trace("te-tform-rm") << "..done " << tret.getNode() << std::endl;

#if 0
  // justify the preprocessing step
  if (lp != nullptr)
  {
    // currently this is a trusted step that combines theory preprocessing and
    // term formula removal.
    if (!CDProof::isSame(node, lemmas[0]))
    {
      Node eq = node.eqNode(lemmas[0]);
      std::shared_ptr<ProofNode> ppf = d_tpg->getTranformProofFor(node,lp);
      Assert (ppf!=nullptr);
      Assert (ppf->getResult()==lemmas[0]);
      // trusted big step
      std::vector<Node> pfChildren;
      pfChildren.push_back(node);
      std::vector<Node> pfArgs;
      pfArgs.push_back(lemmas[0]);
      lp->addStep(lemmas[0], PfRule::THEORY_PREPROCESS, pfChildren, pfArgs);
    }
  }
#endif

  if (Debug.isOn("lemma-ites"))
  {
    Debug("lemma-ites") << "removed ITEs from lemma: " << tret.getNode() << endl;
    Debug("lemma-ites") << " + now have the following " << newLemmas.size()
                        << " lemma(s):" << endl;
  for (size_t i = 0, lsize = newLemmas.size(); i <= lsize; ++i)
  {
      Debug("lemma-ites") << " + " << newLemmas[i].getNode() << endl;
    }
    Debug("lemma-ites") << endl;
  }

  // now, rewrite the lemmas
  for (size_t i = 0, lsize = newLemmas.size(); i <= lsize; ++i)
  {
    // get the trust node to process
    TrustNode trn = i==lsize ? tret : newLemmas[i];
    Node assertion = trn.getNode();
    // rewrite
    Node rewritten = Rewriter::rewrite(assertion);
    if (assertion!=rewritten)
    {
      // update the trust node
      TrustNode trnRew = TrustNode::mkTrustLemma(rewritten, nullptr);
      if (i==lsize)
      {
        tret = trnRew;
      }
      else
      {
        newLemmas[i] = trnRew;
      }
    }
#if 0
    if (lp != nullptr)
    {
      if (!CDProof::isSame(rewritten, lemmas[i]))
      {
        std::vector<Node> pfChildren;
        pfChildren.push_back(lemmas[i]);
        std::vector<Node> pfArgs;
        pfArgs.push_back(rewritten);
        lp->addStep(
            rewritten, PfRule::MACRO_SR_PRED_TRANSFORM, pfChildren, pfArgs);
      }
    }
#endif
  }
  return tret;
}

struct preprocess_stack_element
{
  TNode node;
  bool children_added;
  preprocess_stack_element(TNode n) : node(n), children_added(false) {}
};

TrustNode TheoryPreprocessor::theoryPreprocess(TNode assertion)
{
  Trace("theory::preprocess")
      << "TheoryPreprocessor::theoryPreprocess(" << assertion << ")" << endl;
  // spendResource();

  // Do a topological sort of the subexpressions and substitute them
  vector<preprocess_stack_element> toVisit;
  toVisit.push_back(assertion);

  while (!toVisit.empty())
  {
    // The current node we are processing
    preprocess_stack_element& stackHead = toVisit.back();
    TNode current = stackHead.node;

    Debug("theory::internal")
        << "TheoryPreprocessor::theoryPreprocess(" << assertion
        << "): processing " << current << endl;

    // If node already in the cache we're done, pop from the stack
    NodeMap::iterator find = d_ppCache.find(current);
    if (find != d_ppCache.end())
    {
      toVisit.pop_back();
      continue;
    }

    if (!d_logicInfo.isTheoryEnabled(Theory::theoryOf(current))
        && Theory::theoryOf(current) != THEORY_SAT_SOLVER)
    {
      stringstream ss;
      ss << "The logic was specified as " << d_logicInfo.getLogicString()
         << ", which doesn't include " << Theory::theoryOf(current)
         << ", but got a preprocessing-time fact for that theory." << endl
         << "The fact:" << endl
         << current;
      throw LogicException(ss.str());
    }

    // If this is an atom, we preprocess its terms with the theory ppRewriter
    if (Theory::theoryOf(current) != THEORY_BOOL)
    {
      Node ppRewritten = ppTheoryRewrite(current);
      d_ppCache[current] = ppRewritten;
      Assert(Rewriter::rewrite(d_ppCache[current]) == d_ppCache[current]);
      continue;
    }

    // Not yet substituted, so process
    if (stackHead.children_added)
    {
      // Children have been processed, so substitute
      NodeBuilder<> builder(current.getKind());
      if (current.getMetaKind() == kind::metakind::PARAMETERIZED)
      {
        builder << current.getOperator();
      }
      for (unsigned i = 0; i < current.getNumChildren(); ++i)
      {
        Assert(d_ppCache.find(current[i]) != d_ppCache.end());
        builder << d_ppCache[current[i]];
      }
      // Mark the substitution and continue
      Node result = builder;
      if (result != current)
      {
        result = Rewriter::rewrite(result);
      }
      Debug("theory::internal")
          << "TheoryPreprocessor::theoryPreprocess(" << assertion
          << "): setting " << current << " -> " << result << endl;
      d_ppCache[current] = result;
      toVisit.pop_back();
    }
    else
    {
      // Mark that we have added the children if any
      if (current.getNumChildren() > 0)
      {
        stackHead.children_added = true;
        // We need to add the children
        for (TNode::iterator child_it = current.begin();
             child_it != current.end();
             ++child_it)
        {
          TNode childNode = *child_it;
          NodeMap::iterator childFind = d_ppCache.find(childNode);
          if (childFind == d_ppCache.end())
          {
            toVisit.push_back(childNode);
          }
        }
      }
      else
      {
        // No children, so we're done
        Debug("substitution::internal")
            << "SubstitutionMap::internalSubstitute(" << assertion
            << "): setting " << current << " -> " << current << endl;
        d_ppCache[current] = current;
        toVisit.pop_back();
      }
    }
  }
  // Return the substituted version
  Node res = d_ppCache[assertion];
  return TrustNode::mkTrustRewrite(assertion, res, d_tpg.get());
}

// Recursively traverse a term and call the theory rewriter on its sub-terms
Node TheoryPreprocessor::ppTheoryRewrite(TNode term)
{
  NodeMap::iterator find = d_ppCache.find(term);
  if (find != d_ppCache.end())
  {
    return (*find).second;
  }
  unsigned nc = term.getNumChildren();
  if (nc == 0)
  {
    return preprocessWithProof(term);
  }
  Trace("theory-pp") << "ppTheoryRewrite { " << term << endl;

  Node newTerm = term;
  // do not rewrite inside quantifiers
  if (!term.isClosure())
  {
    NodeBuilder<> newNode(term.getKind());
    if (term.getMetaKind() == kind::metakind::PARAMETERIZED)
    {
      newNode << term.getOperator();
    }
    unsigned i;
    for (i = 0; i < nc; ++i)
    {
      newNode << ppTheoryRewrite(term[i]);
    }
    newTerm = Node(newNode);
  }
  newTerm = rewriteWithProof(newTerm);
  newTerm = preprocessWithProof(newTerm);
  d_ppCache[term] = newTerm;
  Trace("theory-pp") << "ppTheoryRewrite returning " << newTerm << "}" << endl;
  return newTerm;
}

Node TheoryPreprocessor::rewriteWithProof(Node term)
{
  Node termr = Rewriter::rewrite(term);
  // store rewrite step if tracking proofs and it rewrites
  if (d_tpg != nullptr)
  {
    // may rewrite the same term more than once, thus check hasRewriteStep
    if (termr != term && !d_tpg->hasRewriteStep(term))
    {
      d_tpg->addRewriteStep(term, termr, PfRule::REWRITE, {}, {term});
    }
  }
  return termr;
}

Node TheoryPreprocessor::preprocessWithProof(Node term)
{
  // call ppRewrite for the given theory
  TrustNode trn = d_engine.theoryOf(term)->ppRewrite(term);
  if (trn.isNull())
  {
    // no change, return original term
    return term;
  }
  Node termr = trn.getNode();
  if (d_tpg != nullptr)
  {
    if (trn.getGenerator() != nullptr)
    {
      d_tpg->addRewriteStep(term, termr, trn.getGenerator());
    }
    else
    {
      // TODO: small step trust?
    }
  }
  termr = rewriteWithProof(termr);
  return ppTheoryRewrite(termr);
}

}  // namespace theory
}  // namespace CVC4
