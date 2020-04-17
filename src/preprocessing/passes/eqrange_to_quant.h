/*********************                                                        */
/*! \file real_to_int.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haniel Barbosa
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2018 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief The EqrangeToQuant preprocessing pass
 **
 ** Converts the eqrange predicate into its axiomatization
 **/

#include "cvc4_private.h"

#ifndef __CVC4__PREPROCESSING__PASSES__EQRANGE_TO_QUANT_H
#define __CVC4__PREPROCESSING__PASSES__EQRANGE_TO_QUANT_H

#include <unordered_map>
#include <vector>

#include "expr/node.h"
#include "preprocessing/preprocessing_pass.h"
#include "preprocessing/preprocessing_pass_context.h"

namespace CVC4 {
namespace preprocessing {
namespace passes {

using NodeMap = std::unordered_map<Node, Node, NodeHashFunction>;

class EqrangeToQuant : public PreprocessingPass
{
 public:
  EqrangeToQuant(PreprocessingPassContext* preprocContext);

 protected:
  PreprocessingPassResult applyInternal(
      AssertionPipeline* assertionsToPreprocess) override;

 private:
  Node eqrangeToQuantInternal(TNode n, NodeMap& cache);
};

}  // namespace passes
}  // namespace preprocessing
}  // namespace CVC4

#endif /* __CVC4__PREPROCESSING__PASSES__EQRANGE_TO_QUANT_H */
