// ------------------------------------------
// Copyright KAPSARC. Open Source MIT License
// ------------------------------------------
#ifndef AGENDA_DEMO_H
#define AGENDA_DEMO_H

#include "agenda.h"

// ------------------------------------------
namespace AgendaControl {
using KBase::KMatrix;

// These are the "Payoffs in Dollars in the Plott-Levine Experiment"
// from ""Liberalism Against Populism", W. H. Riker, chapter 7.B.
// There are only five distinct payoff structures for twenty-one
// subjects; the repetition was not explained. Note that, with
// traditional deterministic 1P1V, option 0 (PL's #1) is the deterministic
// Condorcet Winner, while option 4 (PL's #5) is defeated by all alternatives.
//
// I added actor 0 to represent the weak but agenda-manipulating chairperson

const double plpA[] = {
  // 0     1     2     3     4
  0.00, 0.00, 0.00, 0.00, 1.00, // 00

  6.00, 7.00, 5.00, 8.00, 0.50, // 01
  6.00, 7.00, 5.00, 8.00, 0.50, // 02
  6.00, 7.00, 5.00, 8.00, 0.50, // 03
  6.00, 7.00, 5.00, 8.00, 0.50, // 04
  6.00, 7.00, 5.00, 8.00, 0.50, // 05
  6.00, 7.00, 5.00, 8.00, 0.50, // 06

  7.50, 7.75, 6.75, 5.75, 0.25, // 07
  7.50, 7.75, 6.75, 5.75, 0.25, // 08
  7.50, 7.75, 6.75, 5.75, 0.25, // 09
  7.50, 7.75, 6.75, 5.75, 0.25, // 10

  7.50, 7.00, 6.00, 8.00, 0.50, // 11

  8.00, 7.50, 7.00, 6.00, 0.50, // 12
  8.00, 7.50, 7.00, 6.00, 0.50, // 13
  8.00, 7.50, 7.00, 6.00, 0.50, // 14

  7.00, 5.50, 7.50, 6.50, 0.25, // 15
  7.00, 5.50, 7.50, 6.50, 0.25, // 16
  7.00, 5.50, 7.50, 6.50, 0.25, // 17
  7.00, 5.50, 7.50, 6.50, 0.25, // 18
  7.00, 5.50, 7.50, 6.50, 0.25, // 19
  7.00, 5.50, 7.50, 6.50, 0.25, // 20
  7.00, 5.50, 7.50, 6.50, 0.25  // 21
};

const double plwA[] = {
  1.00, // 00, to be divided by 25 during setup
  1.00, // 01
  1.00, // 02
  1.00, // 03
  1.00, // 04
  1.00, // 05
  1.00, // 06
  1.00, // 07
  1.00, // 08
  1.00, // 09
  1.00, // 10
  1.00, // 11
  1.00, // 12
  1.00, // 13
  1.00, // 14
  1.00, // 15
  1.00, // 16
  1.00, // 17
  1.00, // 18
  1.00, // 19
  1.00, // 20
  1.00  // 21
};


}; // end of namespace


// ------------------------------------------
#endif
// ------------------------------------------
// Copyright KAPSARC. Open Source MIT License
// ------------------------------------------
