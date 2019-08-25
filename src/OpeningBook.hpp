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

#ifndef OPENING_BOOK_HPP
#define OPENING_BOOK_HPP

#ifdef _X86
#include <stdio.h>
#endif

#include "Position.hpp"
#include "TranspositionTable.hpp"

namespace GameSolver {
namespace Connect4 {

class OpeningBook {
  TableGetter<Position::position_t, uint8_t> *T;
  const int width;
  const int height;
  int depth;

  template<class partial_key_t>
  TableGetter<Position::position_t, uint8_t>* initTranspositionTable(int log_size) {
    switch(log_size) {
    case 18:
      return new TranspositionTable<partial_key_t, Position::position_t, uint8_t, 18>();
    case 21:
      return new TranspositionTable<partial_key_t, Position::position_t, uint8_t, 21>();
    case 22:
      return new TranspositionTable<partial_key_t, Position::position_t, uint8_t, 22>();
    case 23:
      return new TranspositionTable<partial_key_t, Position::position_t, uint8_t, 23>();
    case 24:
      return new TranspositionTable<partial_key_t, Position::position_t, uint8_t, 24>();
    case 25:
      return new TranspositionTable<partial_key_t, Position::position_t, uint8_t, 25>();
    case 26:
      return new TranspositionTable<partial_key_t, Position::position_t, uint8_t, 26>();
    case 27:
      return new TranspositionTable<partial_key_t, Position::position_t, uint8_t, 27>();
    default:
      //std::cerr << "Unimplemented OpeningBook size: " << log_size << std::endl;
      return 0;
    }
  }

  TableGetter<Position::position_t, uint8_t>* initTranspositionTable(int partial_key_bytes, int log_size) {
    switch(partial_key_bytes) {
    case 1:
      return initTranspositionTable<uint8_t>(log_size);
    case 2:
      return initTranspositionTable<uint16_t>(log_size);
    case 4:
      return initTranspositionTable<uint32_t>(log_size);
    default:
      //std::cerr << "Invalid internal key size: " << partial_key_bytes << " bytes" << std::endl;
      return 0;
    }
  }

 public:
  OpeningBook(int width, int height) : T{0}, width{width}, height{height}, depth{ -1} {} // Empty opening book

  OpeningBook(int width, int height, int depth, TableGetter<Position::position_t, uint8_t>* T) : T{T}, width{width}, height{height}, depth{depth} {} // Empty opening book

#ifdef _X86
  void loadFile(const char *filename)
  {
    FILE *f = fopen(filename, "rb");
    if( f )
      {
        fseek(f, 0, SEEK_END);
        size_t len = ftell(f);
        unsigned char *buf = new unsigned char[len];
        if( buf )
          {
            fseek(f, 0, SEEK_SET);
            fread(buf, 1, len, f);
            loadData(buf, len);
            delete[] buf;
          }

        fclose(f);
      }
  }
#endif

  /**
    * Opening book file format:
    * - 1 byte: board width
    * - 1 byte: board height
    * - 1 byte: max stored position depth
    * - 1 byte: key size in bits
    * - 1 byte: value size in bits
    * - 1 byte: log_size = log2(size). number of stored elements (size) is smallest prime number above 2^(log_size)
    * - size key elements
    * - size value elements
    */
  void loadData(const unsigned char *data, size_t len)
  {
    size_t n;
    char _width, _height, _depth, value_bytes, partial_key_bytes, log_size;

    depth = -1;
    delete T; 

    if( len<6 ) {
      //std::cerr << "data size too small (header): " << len << std::endl;
      return;
    }

    _width = *data++; len--;
    if(_width != width) {
      //std::cerr << "Unable to load opening book: invalid width (found: " << int(_width) << ", expected: " << width << ")" << std::endl;
      return;
    }

    _height = *data++; len--;
    if(_height != height) {
      //std::cerr << "Unable to load opening book: invalid height(found: " << int(_height) << ", expected: " << height << ")"  << std::endl;
      return;
    }

    _depth = *data++; len--;
    if(_depth > width * height) {
      //std::cerr << "Unable to load opening book: invalid depth (found: " << int(_depth) << ")"  << std::endl;
      return;
    }

    partial_key_bytes = *data++; len--;
    if(partial_key_bytes > 8) {
      //std::cerr << "Unable to load opening book: invalid internal key size(found: " << int(partial_key_bytes) << ")"  << std::endl;
      return;
    }

    value_bytes = *data++; len--;
    if(value_bytes != 1) {
      //std::cerr << "Unable to load opening book: invalid value size (found: " << int(value_bytes) << ", expected: 1)"  << std::endl;
      return;
    }

    log_size = *data++; len--;
    if(log_size > 40) {
      //std::cerr << "Unable to load opening book: invalid log2(size)(found: " << int(log_size) << ")"  << std::endl;
      return;
    }

    if((T = initTranspositionTable(partial_key_bytes, log_size)))
      {
        n = T->getSize() * partial_key_bytes;
        if( len<n ) {
          //std::cerr << "data size too small (keys): " << len << std::endl;
          return;
        }
        memcpy(reinterpret_cast<char *>(T->getKeys()), data, n);

        data += n; len -= n;
        n = T->getSize() * value_bytes;
        if( len<n ) {
          //std::cerr << "data size too small (values): " << len << std::endl;
          return;
        }
        memcpy(reinterpret_cast<char *>(T->getValues()), data, n);
      }

    depth = _depth; // set it in case of success only, keep -1 in case of failure
  }

  int get(const Position &P) const {
    if(P.nbMoves() > depth) 
      return 0;
    else 
      return T->get(P.key3());
  }

  bool ok() const { return (depth>0); }

  ~OpeningBook() {
    delete T;
  }
};

} // namespace Connect4
} // namespace GameSolver
#endif
