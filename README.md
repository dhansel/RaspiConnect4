# Raspberry Pi Connect Four Solver

For a school project, my son created a robot that plays Connect Four.
Controlled by an Arduino the robot played fairly well but nowhere close to perfect.

Since I knew that Connect Four is a solved game I set out to create an
addition to his project that could help it play a perfect game. It just needed
more computing power and a better algorithm.

In the end I settled for an implementation using a Raspberry Pi Zero that
can be sent the current position on the board and will reply with an optimal
move.

It occurred to me that this may be interesting to others trying to implement
a connect 4 robot so I am making the code available here. Note that very little
of this is of my own writing, I basically took three different projects and
glued them together (see acknowledgements below). 

As written here, this is meant for a Raspberry Pi Zero V1.3 (not W), but it
may be easy to port to others. I just chose the Zero because I had one lying 
around at home and it is nice and small, with a very small power draw.

To set up the solver:
- copy the contents of the "bin" directory to an SD card
- insert the SD card into the Raspberry Pi.
- connect serial lines to pins 8 (TX) and 10 (RX)
  (when connecting to an Arduino, make sure to use a level converter as Raspi pins are 3.3V)

The communication protocol is rather simple. To send a query, open a 
serial connection at 9600 baud 8N1 and send a "!", followed by up to 41 
digits (1-7), followed by a "?".  The digit sequence describes the moves
made so far. For example, sending "!427?" will request the best response
to a board where three moves have been taken: player one in column 4, player
two in column 2 and player one in column 7.

The solver will respond with either a "?" (meaning there was an error in the
query) or a "!" (meaning that it is calculating a solution). This response
is guaranteed within 0.1 seconds, unless the solver was already busy.

After responding "!", the solver will compute the solution. Computation
times for queries up to 12 moves long are almost zero due to the included 
opening books. Times for 13-move queries are the longest - I have seen
130 seconds for the worst. For 14-move queries I have not seen anything
longer than 30 seconds and times go down rapidly from there. Not the fastest
ever but quite reasonable, especially for games where the computer goes
second, i.e. the 13th move is a human player move and does not need to
be computed.

After computing the solution, the solver sends a 4-character response:
"NxMM", where "N" is the column (1-7) in which to play, "x" is a sign
indicating whether the player whose move it is now can win or not.
Specifically, "+" means win, "-" means lose and "=" means tie. "MM"
is the number of moves until the game will end. This assumes that both
players play perfectly. For example, a response of "4+23" means: "if
you play in column 4 then you can win in no more than 23 moves from now",
whereas "6-04" means "if you play in column 6 you will lose in no fewer
than 4 moves".

The green ACT LED on the Raspberry pi shows activity status. It is on 
during initialization after power-up (takes about 2 seconds) and 
flashes on/off while computing solutions.

# Acknowledgements

The Connect Four solver algorithm was taken and adapted from Pascal Pons'
code and tutorial at https://github.com/PascalPons/connect4, distributed
under the GPL-3 license.

The 12-ply opening book including distances (book12.dat) was taken from 
Markus Thill's work at https://github.com/MarkusThill/Connect-Four, 
distributed under the MIT license.

The Raspberry Pi bare-metal framework was adapted from Filippo Bergamasco's
bare-metal ANSI terminal at https://github.com/fbergama/pigfx, also under
the MIT license.
