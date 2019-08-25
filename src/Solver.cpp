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

#include "Solver.hpp"
#include "MoveSorter.hpp"
#include "utils.h"
#include "uart.h"

// act_led is defined in main.c
extern "C" void act_led(int on);

using namespace GameSolver::Connect4;

namespace GameSolver {
namespace Connect4 {

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
int Solver::negamax(const Position &P, int alpha, int beta) {
  nodeCount++; // increment counter of explored nodes
  if( (nodeCount&0x7fff)==0 ) act_led((nodeCount & 0x8000) ? 0 : 1);

  Position::position_t possible = P.possibleNonLosingMoves();
  if(possible == 0)     // if no possible non losing move, opponent wins next move
    return -(Position::WIDTH * Position::HEIGHT - P.nbMoves()) / 2;

  if(P.nbMoves() >= Position::WIDTH * Position::HEIGHT - 2) // check for draw game
    return 0;

  int min = -(Position::WIDTH * Position::HEIGHT - 2 - P.nbMoves()) / 2;	// lower bound of score as opponent cannot win next move
  if(alpha < min) {
    alpha = min;                     // there is no need to keep alpha below our max possible score.
    if(alpha >= beta) return alpha;  // prune the exploration if the [alpha;beta] window is empty.
  }

  int max = (Position::WIDTH * Position::HEIGHT - 1 - P.nbMoves()) / 2;	// upper bound of our score as we cannot win immediately
  if(beta > max) {
    beta = max;                     // there is no need to keep beta above our max possible score.
    if(alpha >= beta) return beta;  // prune the exploration if the [alpha;beta] window is empty.
  }

  const Position::position_t key = P.key();
  if(int val = transTable.get(key)) {
    if(val > Position::MAX_SCORE - Position::MIN_SCORE + 1) { // we have an lower bound
      min = val + 2 * Position::MIN_SCORE - Position::MAX_SCORE - 2;
      if(alpha < min) {
        alpha = min;                     // there is no need to keep beta above our max possible score.
        if(alpha >= beta) return alpha;  // prune the exploration if the [alpha;beta] window is empty.
      }
    } else { // we have an upper bound
      max = val + Position::MIN_SCORE - 1;
      if(beta > max) {
        beta = max;                     // there is no need to keep beta above our max possible score.
        if(alpha >= beta) return beta;  // prune the exploration if the [alpha;beta] window is empty.
      }
    }
    }

  // opening books only contain sequences 12 moves or less, 
  // save time by not looking for longer sequences
  if( P.nbMoves()<=12 )
    {
      if( P.nbMoves()==12 )
        {
          // find solution in dedicated (complete) 12-move opening book
          if(int val = book12.get(P)) return val + Position::MIN_SCORE - 1;
        }
      else 
        {
          // look for solutions stored in general opening book
          if(int val = book.get(P))  return val + Position::MIN_SCORE - 1;
        }
    }

  MoveSorter moves;
  for(int i = Position::WIDTH; i--;)
    if(Position::position_t move = possible & Position::column_mask(columnOrder[i]))
      moves.add(move, P.moveScore(move));

  while(Position::position_t next = moves.getNext()) {
    Position P2(P);
    P2.play(next);  // It's opponent turn in P2 position after current player plays x column.
    int score = -negamax(P2, -beta, -alpha); // explore opponent's score within [-beta;-alpha] windows:
    // no need to have good precision for score better than beta (opponent's score worse than -beta)
    // no need to check for score worse than alpha (opponent's score worse better than -alpha)

    if(score >= beta) {
      transTable.put(key, score + Position::MAX_SCORE - 2 * Position::MIN_SCORE + 2); // save the lower bound of the position
      return score;  // prune the exploration if we find a possible move better than what we were looking for.
    }
    if(score > alpha) alpha = score; // reduce the [alpha;beta] window for next exploration, as we only
    // need to search for a position that is better than the best so far.
  }

  transTable.put(key, alpha - Position::MIN_SCORE + 1); // save the upper bound of the position
  return alpha;
}

int Solver::solve(const Position &P, bool weak) {
  if(P.canWinNext()) // check if win in one move as the Negamax function does not support this case.
    return (Position::WIDTH * Position::HEIGHT + 1 - P.nbMoves()) / 2;
  int min = -(Position::WIDTH * Position::HEIGHT - P.nbMoves()) / 2;
  int max = (Position::WIDTH * Position::HEIGHT + 1 - P.nbMoves()) / 2;
  if(weak) {
    min = -1;
    max = 1;
  }

  while(min < max) {                    // iteratively narrow the min-max exploration window
    int med = min + (max - min) / 2;
    if(med <= 0 && min / 2 < med) med = min / 2;
    else if(med >= 0 && max / 2 > med) med = max / 2;
    int r = negamax(P, med, med + 1);   // use a null depth window to know if the actual score is greater or smaller than med
    if(r <= med) max = r;
    else min = r;
  }
  return min;
}

// Constructor
Solver::Solver() : nodeCount{0} {
  for(int i = 0; i < Position::WIDTH; i++) // initialize the column exploration order, starting with center columns
    columnOrder[i] = Position::WIDTH / 2 + (1 - 2 * (i % 2)) * (i + 1) / 2; // example for WIDTH=7: columnOrder = {3, 4, 2, 5, 1, 6, 0}
}


} // namespace Connect4
} // namespace GameSolver


// ---------------------------------------------------------------------------------------------------------------------------------
//                                                top-level solve functions
// ---------------------------------------------------------------------------------------------------------------------------------


#ifdef _X86
#include <stdlib.h>
#else
extern "C" unsigned int rand();
extern unsigned char G_BOOK;
extern unsigned char G_BOOK12;
extern unsigned char G_BOOK12_END;
#endif

static Solver *solver = NULL;


static int getBestMove(Solver &solver, Position P, int *score = NULL)
{
  int scores[7], bestScore = -100, bestColumn = -1;

  act_led(1);

  solver.resetNodeCount();
  for(int i=0; i<7; i++)
    {
      Position P2 = P;
      if( P2.canPlay(i) ) 
        {
          if( P2.isWinningMove(i) )
            scores[i] = 100;
          else
            {
              P2.playCol(i);
              scores[i] = -solver.solve(P2, false);
            }

          if( scores[i] > bestScore ) { bestScore = scores[i]; bestColumn = i; }
        }
      else
        scores[i] = -100;
    }

  int numBest = 0;
  for(int column=0; column<7; column++)
    if( scores[column] == bestScore )
      numBest++;
  
  numBest = ::rand() % numBest;
  for(int column=0; column<7; column++)
    if( scores[column] == bestScore )
      if( numBest-- == 0 )
        bestColumn = column;

  act_led(0);

  if( score!=NULL ) *score = bestScore;
  return bestColumn;
}


extern "C" void solver_init()
{
  if( solver==NULL ) 
    {
      solver = new Solver;
#ifdef _X86
      solver->getBook().loadFile("book.dat");
      solver->getBook12().loadFile("book12.dat");
#else
      solver->getBook().loadData(&G_BOOK, &G_BOOK12-&G_BOOK);
      solver->getBook12().setData(&G_BOOK12, &G_BOOK12_END-&G_BOOK);
#endif
    }
}

extern "C" const char *solver_solve(const char *position, unsigned long long *nodeCount)
{
  static char res[5];

  solver_init();
  Position P;
  if( P.play(position) )
    {
      int column, score;

      uart_write("!", 1);
      column = getBestMove(*solver, P, &score);

      res[0] = column + '1';
      if( P.isWinningMove(column) )
        {
          // will win in this move if played in column
          memcpy(res+1, "+00", 3);
        }
      else if( P.nbMoves()==41 )
        {
          // ties in this move if played in column
          memcpy(res+1, "=00", 3);
        }
      else if( score>0 )
        {
          // can win in "n" moves if played in column
          int n = 43 - score*2 - P.nbMoves() - (~P.nbMoves()&1);
          res[1] = '+';
          res[2] = '0' + n/10;
          res[3] = '0' + n%10;
        }
      else if( score<0 )
        {
          // will lose in no fewer than "n" moves if played in column
          int n = 43 + score*2 - P.nbMoves() - (P.nbMoves()&1);
          res[1] = '-';
          res[2] = '0' + n/10;
          res[3] = '0' + n%10;
        }
      else
        {
          // can tie in "n" moves if played in column
          int n = 41 - P.nbMoves();
          res[1] = '=';
          res[2] = '0' + n/10;
          res[3] = '0' + n%10;
        }

      res[4] = 0;
      if( nodeCount!=0 ) *nodeCount = solver->getNodeCount();
      return res;
    }
  else
    return NULL;
}
