/*
 * This file is part of Connect4 Game Solver <http://connect4.gamesolver.org>
 * Copyright (C) 2017-2019 Pascal Pons <contact@gamesolver.org>
 *
 * Connect4 Game Solver is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Connect4 Game Solver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Connect4 Game Solver. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SOLVER_HPP
#define SOLVER_HPP

#ifdef __cplusplus 

#include <cstddef>

#include "Position.hpp"
#include "TranspositionTable.hpp"
#include "OpeningBook.hpp"
#include "OpeningBook12.hpp"

namespace GameSolver {
namespace Connect4 {

class Solver {
 private:
  static constexpr int TABLE_SIZE = 24; // store 2^TABLE_SIZE elements in the transpositiontbale
  TranspositionTable < uint_t < Position::WIDTH*(Position::HEIGHT + 1) - TABLE_SIZE >, Position::position_t, uint8_t, TABLE_SIZE > transTable;
  OpeningBook book{Position::WIDTH, Position::HEIGHT}; // opening book
  OpeningBook12 book12{Position::WIDTH, Position::HEIGHT}; // complete 12-move opening book
  unsigned long long nodeCount; // counter of explored nodes.
  int columnOrder[Position::WIDTH]; // column exploration order

  /**
   * Reccursively score connect 4 position using negamax variant of alpha-beta algorithm.
   * @param: position to evaluate, this function assumes nobody already won and
   *         current player cannot win next move. This has to be checked before
   * @param: alpha < beta, a score window within which we are evaluating the position.
   *
   * @return the exact score, an upper or lower bound score depending of the case:
   * - if actual score of position <= alpha then actual score <= return value <= alpha
   * - if actual score of position >= beta then beta <= return value <= actual score
   * - if alpha <= actual score <= beta then return value = actual score
   */
  int negamax(const Position &P, int alpha, int beta);

 public:

  int solve(const Position &P, bool weak = false);

  void resetNodeCount() {
    nodeCount = 0;
  }

  unsigned long long getNodeCount() const {
    return nodeCount;
  }

  void reset() {
    resetNodeCount();
    transTable.reset();
  }

  OpeningBook &getBook() { return book; }
  OpeningBook12 &getBook12() { return book12; }

  Solver(); // Constructor
};

} // namespace Connect4
} // namespace GameSolver


#else

void solver_init();
const char *solver_solve(const char *position, unsigned long long *nodeCount);

#endif


#endif
