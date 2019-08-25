// -----------------------------------------------------------------------------
// Connect 4 solver on bare metal Raspberry Pi Zero
// Copyright (C) 2019 David Hansel
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
// -----------------------------------------------------------------------------


#include "uart.h"
#include "utils.h"
#include "Memory.hpp"
#include "Solver.hpp"

#ifdef _X86  // ----------------------- X86 for testing

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

long long timeInMilliseconds(void) 
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

void uart_write(const char *s, unsigned int n)
{
  for(unsigned int i=0; i<n; i++) printf("%c", s[i]);
  fflush(stdout);
}

void uart_write_str(const char *s)
{
  while(*s) uart_write((char *) s++, 1);
}


void act_led(int on) {}


#define HEAPSIZE 200000000

int main(int argc, char **argv)
{
  unsigned long long n;
  void *heap = malloc(HEAPSIZE);
  if( heap )
    memory_set_area(heap, HEAPSIZE);
  else
    {
      printf("can't allocate heap\n");
      exit(0);
    }

  srand(time(NULL));
  long long t1 = timeInMilliseconds();
  const char *s = solver_solve(argc>1 ? argv[1] : "", &n);
  long long t2 = timeInMilliseconds();
  uart_write_str(s==NULL ? "?" : s);
  printf("\nnodes: %I64u, time: %I64i milliseconds\n", n, t2-t1);
  return 0;
}


#else // -------------------- ARM

#include "postman.h"

extern unsigned int pheap_space;
extern unsigned int heap_sz;

void __attribute__((interrupt("IRQ"))) irq_handler_(void) {}


#define GPIO_BASE ((volatile unsigned int*) 0x20200000)
#define MODE_REGISTER_ADDR (GPIO_BASE + 0x0)
#define SET_REGISTER_ADDR  (GPIO_BASE + 0x7)
#define CLR_REGISTER_ADDR  (GPIO_BASE + 0xA)

const char *u2s(unsigned int u)
{
  unsigned int i = 1, j = 0;
  static char buffer[20];

  if( u>=1000000000 )
    i = 1000000000;
  else
    {
      while( u>=i ) i *= 10;
      i /= 10;
    }

  if( i==0 )
    buffer[j++] = '0';
  else
    while( i>0 )
      {
        unsigned int d = u/i;
        buffer[j++] = 48+d;
        u -= d*i;
        i = i/10;
      }
  
  buffer[j] = 0;
  return buffer;
}


const char *u2h(unsigned int u)
{
  static const char hex[16] = "0123456789abcdef";
  static char buffer[9];

  for(int i=0; i<8; i++)
    {
      buffer[7-i] = hex[u & 15];
      u = u >> 4;
    }

  buffer[8] = 0;
  return buffer;
}


void gpio_setup_output(unsigned int gpio)
{
  if( gpio < 54 )
    {
      unsigned int bit = ((gpio % 10) * 3);
      unsigned int mem = MODE_REGISTER_ADDR[gpio / 10];
      mem &= ~(7 << bit);
      mem |= (1 << bit);
      MODE_REGISTER_ADDR[gpio / 10] = mem;
    }
}


void gpio_output(unsigned int gpio, int on) 
{
  if (gpio < 54)
    {
      unsigned int regnum = gpio / 32;
      unsigned int bit = 1 << (gpio % 32);
      if( on )
        SET_REGISTER_ADDR[regnum] = bit;
      else 
        CLR_REGISTER_ADDR[regnum] = bit;
    }
}


void act_led(int on)
{
  // ACT LED on Raspi Zero is GPIO 47 (inverted)
  gpio_output(47, !on);
}


void print_buffer(volatile unsigned int *b)
{
  unsigned int i;
  uart_write(" ", 1);
  uart_write_str(u2h(b[0]));
  uart_write(" ", 1);
  
  for(i=1; i<(b[0]/4) && i<32; i++)
    {
      usleep(20000);
      uart_write_str(u2h(b[i]));
      uart_write(" ", 1);
    }
}

unsigned int get_temp()
{
  unsigned int response;
  volatile unsigned int buffer[8] __attribute__((aligned (16)));

  // construct get_temp request
  buffer[0] = 8*4;        // buffer size in bytes
  buffer[1] = 0;          // bit 31 clear: this is a request
  buffer[2] = 0x00030006; // get_temp tag
  buffer[3] = 2*4;        // value buffer size in bytes
  buffer[4] = 0;          // bit 31 clear: request
  buffer[5] = 0;          // id
  buffer[6] = 0;          // reserved for response value
  buffer[7] = 0;          // end tag

  if( POSTMAN_SUCCESS == postman_send( 8, (unsigned int) buffer) )
    if( POSTMAN_SUCCESS == postman_recv( 8, &response ) )
      {
        // response is within buffer provided during request
        return buffer[6]; // core temperature in 1/1000 degrees C
      }
  
  return 0xFFFFFFFF;
}


int set_turbo(unsigned int on)
{
  unsigned int response;
  volatile unsigned int buffer[8] __attribute__((aligned (16)));

  // construct set_turbo request
  buffer[0] = 8*4;        // buffer size in bytes
  buffer[1] = 0;          // bit 31 clear: this is a request
  buffer[2] = 0x00038009; // set_turbo tag
  buffer[3] = 2*4;        // value buffer size in bytes
  buffer[4] = 0;          // bit 31 clear: request
  buffer[5] = 0;          // id
  buffer[6] = on ? 1 : 0; // level
  buffer[7] = 0;          // end tag

  if( POSTMAN_SUCCESS == postman_send( 8, (unsigned int) buffer) )
    if( POSTMAN_SUCCESS == postman_recv( 8, &response ) )
      return 1;

  return 0;
}


// our initial starting seed is 5323
static unsigned int nSeed = 5323;

void srand(unsigned int n)
{
  nSeed = n;
}

unsigned int rand()
{
  // Take the current seed and generate a new value from it
  // Due to our use of large constants and overflow, it would be
  // very hard for someone to predict what the next number is
  // going to be from the previous one.
  nSeed = (8253729 * nSeed + 2396403); 
  
  // Take the seed and return a value between 0 and 32767
  return nSeed  % 32767;
}

void entry_point()
{
  int first = 1;

  // turn on turbo mode during initialization
  set_turbo(1);

  // configure ACT LED GPIO for output and turn it on during initialization
  gpio_setup_output(47); // ACT LED
  act_led(1);

  // configure "advantage" GPIO outputs (connect to LEDs)
  gpio_setup_output(23);
  gpio_setup_output(24);

  // initialize heap
  memory_set_area( (unsigned char*)( pheap_space ), heap_sz );

  // initialize UART
  uart_init();

  // initialize solver
  solver_init();

  // turn off ACT LED
  act_led(0);

  // player 1 has advantage before first move
  gpio_output(23, 0);
  gpio_output(24, 1);

  while(1)
    {
      int n;
      char c, pos[50], ok = 0;
      const char *result;

      uart_purge();
      set_turbo(0); // turbo off (low power) while waiting for request
      while( uart_read_byte() != '!' );
      set_turbo(1); // turbo back on

      n = 0;
      ok = 1;
      while( ok && (c=uart_read_byte()) != '?' ) 
        { 
          if( c>='1' && c<='7' && n<42 )
            pos[n++] = c; 
          else if( !isspace(c) )
            ok = 0;
        }

      if( ok )
        {
          //unsigned int t;
          //unsigned int t1, t2;
          pos[n] = 0;
          if( first ) srand(time_microsec());
          //t1 = time_microsec() / 1000;
          //t = get_temp(); uart_write_str(" "); uart_write_str(u2s(t)); 
          result = solver_solve(pos, NULL);
          //t = get_temp(); uart_write_str(" "); uart_write_str(u2s(t)); uart_write_str(" "); 
          //t2 = time_microsec() / 1000;
          if( result )
            {
              // GPIO24 on: player 1 has advantage
              // GPIO23 on: player 2 has advantage
              int isWin  = result[1]=='+';
              int isLose = result[1]=='-';
              int p1     = !(n&1);
              gpio_output(24, ( p1 && isWin) || (!p1 && isLose));
              gpio_output(23, (!p1 && isWin) || ( p1 && isLose));
            }

          uart_write_str(result ? result : "?");
          //uart_write_str(" "); uart_write_str(u2s(t2-t1));
        }
      else
        uart_write_str("?");
    }
}

#endif
