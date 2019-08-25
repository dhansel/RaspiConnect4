#ifndef OPENING_BOOK12_HPP
#define OPENING_BOOK12_HPP

#ifdef _X86
#include <stdio.h>
#endif

#include "Position.hpp"

#define BOOKSIZE     4200899
#define MASKPOSITION 0xFFFFFFFC;

namespace GameSolver {
namespace Connect4 {

class OpeningBook12 {
  int  *book;
  signed char *vals;

 private:
  int getValue(int codedPos, int codedPosMirrored) const
  {
    int code = 0, code2 = 0, pos, pos2, step;
    pos = pos2 = step = BOOKSIZE - 1;

    // Binary Search
    while (step > 0) {
      step = (step != 1 ? (step + (step & 1)) >> 1 : 0);
      if (pos < BOOKSIZE && pos >= 0)
        code = book[pos] & MASKPOSITION;
      if (pos2 < BOOKSIZE && pos2 >= 0)
        code2 = book[pos2] & MASKPOSITION;
      
      if (codedPos < code)
        pos -= step;
      else if (codedPos > code)
        pos += step;
      else if (codedPos == code)
        return vals[pos];
      
      if (codedPosMirrored < code2)
        pos2 -= step;
      else if (codedPosMirrored > code2)
        pos2 += step;
      else if (codedPosMirrored == code2)
        return vals[pos2];
    }
    
    return 0;
  }

 public:
  OpeningBook12(int width, int height) { book=NULL; vals=NULL; } // Empty opening book
  ~OpeningBook12() {}
  
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
            setData(buf, len);
          }

        fclose(f);
      }
  }
#endif

  void setData(const unsigned char *data, size_t len)
  {
    // check data size
    if( len < BOOKSIZE*5 ) return;

    book = (int*) data;
    vals = (signed char  *) (data+(sizeof(int)*BOOKSIZE));
  }

  bool ok() const { return (book!=NULL) && (vals!=NULL); }

  int get(const Position &P) const 
  {
    if( book==NULL || vals==NULL || P.nbMoves() != 12 )
      return 0;
    else
      {
        // get huffman code for the position
        int huffman, huffmanM;
        P.getHuffman(huffman, huffmanM);

        // get distance value from huffman coded database:
        //  97: current player will win in two moves
        //  96: current player will win in three moves
        // ...
        //  0: either the game is a draw or current player will win in next move
        // ...
        // -96: other player will win in three moves
        // -97: other player will win in four moves
        int r, n = getValue(huffman, huffmanM);
        
        // convert distance to our representation (Position::MIN_SCORE=-18)
        //   1: other player will win with their 21st piece
        //   2: other player will win with their 20th piece
        //  ...
        //  18: other player will win with their 4th piece
        //  19: game is a draw
        //  20: current player will win with their 21st piece
        //  21: current player will win with their 20th piece
        //  ...
        //  37: current player will win with their 4th piece
        if( n<0 ) 
          r = -22 - ((-100 - n - P.nbMoves()) / 2);
        else if( n>0 )
          r =  22 - ( (101 - n + P.nbMoves()) / 2);
        else if( P.canWinNext() )
          r = 15; // current player wins in next move (not in database)
        else
          r = 0; // draw (not in database)

        return r - Position::MIN_SCORE + 1;
      }
  }
};

} // namespace Connect4
} // namespace GameSolver
#endif
