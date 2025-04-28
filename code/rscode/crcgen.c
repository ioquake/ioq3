/*****************************
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

 * Commercial licensing is available under a separate license, please
 * contact author for details.
 *
 * Source code is available at http://rscode.sourceforge.net
 *
 * CRC-CCITT generator simulator for byte wide data.
 *
 *
 * CRC-CCITT = x^16 + x^12 + x^5 + 1
 *
 *
 ******************************/
 
 
#include "ecc.h"

BIT16 crchware(BIT16 data, BIT16 genpoly, BIT16 accum);

/* Computes the CRC-CCITT checksum on array of byte data, length len
*/
BIT16 crc_ccitt(unsigned char *msg, int len)
{
	int i;
	BIT16 acc = 0;

	for (i = 0; i < len; i++) {
		acc = crchware((BIT16) msg[i], (BIT16) 0x1021, acc);
	}
	
	return(acc);
}
	
/* models crc hardware (minor variation on polynomial division algorithm) */
BIT16 crchware(BIT16 data, BIT16 genpoly, BIT16 accum)
{
	static BIT16 i;
	data <<= 8;
	for (i = 8; i > 0; i--) {
		if ((data ^ accum) & 0x8000)
			accum = ((accum << 1) ^ genpoly) & 0xFFFF;
		else
			accum = (accum<<1) & 0xFFFF;
		data = (data<<1) & 0xFFFF;
	}
	return (accum);
}
		
