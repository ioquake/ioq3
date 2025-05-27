/* Reed Solomon Coding for glyphs
 * Copyright Henry Minsky (hqm@alum.mit.edu) 1991-2009
 *
 * This software library is licensed under terms of the GNU GENERAL
 * PUBLIC LICENSE
 *
 * RSCODE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RSCODE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Rscode.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Source code is available at http://rscode.sourceforge.net
 *
 * Commercial licensing is available under a separate license, please
 * contact author for details.
 *
 */

/****************************************************************
  
  Below is NPAR, the only compile-time parameter you should have to
  modify.
  
  It is the number of parity bytes which will be appended to
  your data to create a codeword.

  Note that the maximum codeword size is 255, so the
  sum of your message length plus parity should be less than
  or equal to this maximum limit.

  In practice, you will get slooow error correction and decoding
  if you use more than a reasonably small number of parity bytes.
  (say, 10 or 20)

  ****************************************************************/

#define NPAR 4

/****************************************************************/




#define TRUE 1
#define FALSE 0

typedef unsigned long BIT32;
typedef unsigned short BIT16;

/* **************************************************************** */

/* Maximum degree of various polynomials. */
#define MAXDEG (NPAR*2)

/*************************************/
/* Encoder parity bytes */
extern int pBytes[MAXDEG];

/* Decoder syndrome bytes */
extern int synBytes[MAXDEG];

/* print debugging info */
extern int DEBUG;

/* Reed Solomon encode/decode routines */
void initialize_ecc (void);
int check_syndrome (void);
void decode_data (unsigned char data[], int nbytes);
void encode_data (unsigned char msg[], int nbytes, unsigned char dst[]);

/* CRC-CCITT checksum generator */
BIT16 crc_ccitt(unsigned char *msg, int len);

/* galois arithmetic tables */
extern int gexp[];
extern int glog[];

void init_galois_tables (void);
int ginv(int elt); 
int gmult(int a, int b);


/* Error location routines */
int correct_errors_erasures (unsigned char codeword[], int csize,int nerasures, int erasures[]);

/* polynomial arithmetic */
void add_polys(int dst[], int src[]) ;
void scale_poly(int k, int poly[]);
void mult_polys(int dst[], int p1[], int p2[]);

void copy_poly(int dst[], int src[]);
void zero_poly(int poly[]);
