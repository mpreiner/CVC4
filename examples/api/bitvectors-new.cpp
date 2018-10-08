/*********************                                                        */
/*! \file bitvectors.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Aina Niemetz, Liana Hadarean, Morgan Deters
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2018 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief A simple demonstration of the solving capabilities of the CVC4
 ** bit-vector solver.
 **
 **/

#include <iostream>

//#include <cvc4/cvc4.h> // use this after CVC4 is properly installed
#include "api/cvc4cpp.h"

using namespace std;
using namespace CVC4::api;

int main()
{

  Solver slv;
  slv.setLogic("QF_BV"); // Set the logic

  // The following example has been adapted from the book A Hacker's Delight by
  // Henry S. Warren.
  //
  // Given a variable x that can only have two values, a or b. We want to
  // assign to x a value other than the current one. The straightforward code
  // to do that is:
  //
  //(0) if (x == a ) x = b;
  //    else x = a;
  //
  // Two more efficient yet equivalent methods are:
  //
  //(1) x = a ⊕ b ⊕ x;
  //
  //(2) x = a + b - x;
  //
  // We will use CVC4 to prove that the three pieces of code above are all
  // equivalent by encoding the problem in the bit-vector theory.

  // Creating a bit-vector type of width 32
  Sort bitvector32 = slv.mkBitVectorSort(32);

  // Variables
  Term x = slv.mkVar("x", bitvector32);
  Term a = slv.mkVar("a", bitvector32);
  Term b = slv.mkVar("b", bitvector32);

  // First encode the assumption that x must be equal to a or b
  Term x_eq_a = slv.mkTerm(EQUAL, x, a);
  Term x_eq_b = slv.mkTerm(EQUAL, x, b);
  Term assumption = slv.mkTerm(OR, x_eq_a, x_eq_b);

  // Assert the assumption
  slv.assertFormula(assumption);

  // Introduce a new variable for the new value of x after assignment.
  Term new_x = slv.mkVar("new_x", bitvector32); // x after executing code (0)
  Term new_x_ = slv.mkVar("new_x_", bitvector32); // x after executing code (1) or (2)

  // Encoding code (0)
  // new_x = x == a ? b : a;
  Term ite = slv.mkTerm(ITE, x_eq_a, b, a);
  Term assignment0 = slv.mkTerm(EQUAL, new_x, ite);

  // Assert the encoding of code (0)
  cout << "Asserting " << assignment0 << " to CVC4 " << endl;
  slv.assertFormula(assignment0);
  cout << "Pushing a new context." << endl;
  slv.push();

  // Encoding code (1)
  // new_x_ = a xor b xor x
  Term a_xor_b_xor_x = slv.mkTerm(BITVECTOR_XOR, a, b, x);
  Term assignment1 = slv.mkTerm(EQUAL, new_x_, a_xor_b_xor_x);

  // Assert encoding to CVC4 in current context;
  cout << "Asserting " << assignment1 << " to CVC4 " << endl;
  slv.assertFormula(assignment1);
  Term new_x_eq_new_x_ = slv.mkTerm(EQUAL, new_x, new_x_);

  cout << " Check validity assuming: " << new_x_eq_new_x_ << endl;
  cout << " Expect valid. " << endl;
  cout << " CVC4: " << slv.checkValidAssuming(new_x_eq_new_x_) << endl;
  cout << " Popping context. " << endl;
  slv.pop();

  // Encoding code (2)
  // new_x_ = a + b - x
  Term a_plus_b = slv.mkTerm(BITVECTOR_PLUS, a, b);
  Term a_plus_b_minus_x = slv.mkTerm(BITVECTOR_SUB, a_plus_b, x);
  Term assignment2 = slv.mkTerm(EQUAL, new_x_, a_plus_b_minus_x);

  // Assert encoding to CVC4 in current context;
  cout << "Asserting " << assignment2 << " to CVC4 " << endl;
  slv.assertFormula(assignment2);

  cout << " Check validity assuming: " << new_x_eq_new_x_ << endl;
  cout << " Expect valid. " << endl;
  cout << " CVC4: " << slv.checkValidAssuming(new_x_eq_new_x_) << endl;

  Term x_neq_x = slv.mkTerm(EQUAL, x, x).notTerm();
  std::vector<Term> v{new_x_eq_new_x_, x_neq_x};
  cout << " Check Validity Assuming: " << v << endl;
  cout << " Expect invalid. " << endl;
  cout << " CVC4: " << slv.checkValidAssuming(v) << endl;

  // Assert that a is odd 
  OpTerm extract_op = slv.mkOpTerm(BITVECTOR_EXTRACT_OP, 0, 0);
  Term  lsb_of_a = slv.mkTerm(extract_op, a);
  cout << "Sort of " << lsb_of_a << " is " << lsb_of_a.getSort() << endl;
  Term a_odd = slv.mkTerm(EQUAL, lsb_of_a, slv.mkBitVector(1u, 1u));
  cout << "Assert " << a_odd << endl;
  cout << "Check satisfiability." << endl;
  slv.assertFormula(a_odd);
  cout << " Expect sat. " << endl;
  cout << " CVC4: " << slv.checkSat() << endl;
  return 0;
}
