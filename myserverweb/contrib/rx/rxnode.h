/* classes: h_files */

#ifndef RXNODEH
#define RXNODEH
/*	Copyright (C) 1995, 1996 Tom Lord
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */


/*
 * Tom Lord (lord@cygnus.com, lord@gnu.ai.mit.edu)
 */

#include "rxbitset.h"
#include "rxcset.h"


enum rexp_rxnode_type
{
  r_cset = 0,			/* Match from a character set. `a' or `[a-z]'*/
  r_concat = 1,			/* Concat two subexpressions.   `ab' */
  r_alternate = 2,		/* Choose one of two subexpressions. `a\|b' */
  r_opt = 3,			/* Optional subexpression. `a?' */
  r_star = 4,			/* Repeated subexpression. `a*' */
  r_plus = 5,			/* Nontrivially repeated subexpression. `a+' */
  r_string = 6,			/* Shorthand for a concatenation of characters */
  r_cut = 7,			/* Generates a tagged, final nfa state. */

  /* see RX_regular_rxnode_type */

  r_interval = 8,		/* Counted subexpression.  `a{4, 1000}' */
  r_parens = 9,			/* Parenthesized subexpression */
  r_context = 10		/* Context-sensative operator such as "^" */
};

#define RX_regular_rxnode_type(T)  ((T) <= r_interval)



struct rx_string
{
  unsigned long len;
  unsigned long reallen;
  unsigned char *contents;
};

struct rexp_rxnode
{
  int refs;
  enum rexp_rxnode_type type;
  
  struct
  {
    int cset_size;
    rx_Bitset cset;
    long intval;
    long intval2;
    struct s_rxpair
    {
      struct rexp_rxnode *left;
      struct rexp_rxnode *right;
    }  rxpair;
    struct rx_string cstr;
  } params;
  int id;
  int len;
  int observed;
  struct rexp_rxnode * simplified;
  struct rx_cached_rexp * cr;
};


#ifdef __STDC__
extern int rx_adjoin_string (struct rx_string *str, char c);
extern struct rexp_rxnode * rexp_rxnode (int type);
extern struct rexp_rxnode * rx_mk_r_cset (int type, int size, rx_Bitset b);
extern struct rexp_rxnode * rx_mk_r_int (int type, int intval);
extern struct rexp_rxnode * rx_mk_r_str (int type, char c);
extern struct rexp_rxnode * rx_mk_r_binop (int type, struct rexp_rxnode * a, struct rexp_rxnode * b);
extern struct rexp_rxnode * rx_mk_r_monop (int type, struct rexp_rxnode * a);
extern void rx_free_rexp (struct rexp_rxnode * rxnode);
extern void rx_save_rexp (struct rexp_rxnode * rxnode);
extern struct rexp_rxnode * rx_copy_rexp (int cset_size, struct rexp_rxnode *rxnode);
extern struct rexp_rxnode * rx_shallow_copy_rexp (int cset_size, struct rexp_rxnode *rxnode);
extern int rx_rexp_equal (struct rexp_rxnode * a, struct rexp_rxnode * b);
extern unsigned long rx_rexp_hash (struct rexp_rxnode * rxnode, unsigned long seed);

#else /* STDC */
extern int rx_adjoin_string ();
extern struct rexp_rxnode * rexp_rxnode ();
extern struct rexp_rxnode * rx_mk_r_cset ();
extern struct rexp_rxnode * rx_mk_r_int ();
extern struct rexp_rxnode * rx_mk_r_str ();
extern struct rexp_rxnode * rx_mk_r_binop ();
extern struct rexp_rxnode * rx_mk_r_monop ();
extern void rx_free_rexp ();
extern void rx_save_rexp ();
extern struct rexp_rxnode * rx_copy_rexp ();
extern struct rexp_rxnode * rx_shallow_copy_rexp ();
extern int rx_rexp_equal ();
extern unsigned long rx_rexp_hash ();

#endif /* STDC */




#endif  /* RXNODEH */
