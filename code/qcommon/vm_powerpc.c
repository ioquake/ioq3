/*
===========================================================================
Copyright (C) 2008 Przemyslaw Iskra <sparky@pld-linux.org>

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/


#if defined(powerc) || defined(powerpc) || defined(__powerpc__) || \
	defined(ppc) || defined(__ppc) || defined(__ppc__) || \
	defined(__powerpc64__) || defined(__ppc64__)

#include <sys/types.h> /* needed by sys/mman.h on OSX */
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif

#include "vm_local.h"

/*
 * Register information according to:
 * http://refspecs.freestandards.org/elf/elfspec_ppc.pdf
 */

#define r0	0	// volatile
#define r1	1	// caller safe ( stack pointer )
#define r2	2	// reserved
#define r3	3	// callee safe
#define r4	4	// callee safe
#define r5	5	// callee safe
#define r6	6	// callee safe
#define r7	7	// callee safe
#define r8	8	// callee safe
#define r9	9	// callee safe
#define r10	10	// callee safe
#define r11	11	// volatile
#define r12	12	// volatile
#define r13	13	// reserved ( small data area )
#define r14	14	// caller safe
#define r15	15	// caller safe
#define r16	16	// caller safe
#define r17	17	// caller safe
#define r18	18	// caller safe
#define r19	19	// caller safe
#define r20	20	// caller safe
#define r21	21	// caller safe
#define r22	22	// caller safe
#define r23	23	// caller safe
#define r24	24	// caller safe
#define r25	25	// caller safe
#define r26	26	// caller safe
#define r27	27	// caller safe
#define r28	28	// caller safe
#define r29	29	// caller safe
#define r30	30	// caller safe
#define r31	31	// caller safe ( environment pointers )

#define f0	0	// callee safe
#define f1	1	// callee safe
#define f2	2	// callee safe
#define f3	3	// callee safe
#define f4	4	// callee safe
#define f5	5	// callee safe
#define f6	6	// callee safe
#define f7	7	// callee safe
#define f8	8	// callee safe
#define f9	9	// callee safe
#define f10	10	// callee safe
#define f11	11	// callee safe
#define f12	12	// callee safe
#define f13	13	// callee safe
#define f14	14	// caller safe
#define f15	15	// caller safe
#define f16	16	// caller safe
#define f17	17	// caller safe
#define f18	18	// caller safe
#define f19	19	// caller safe
#define f20	20	// caller safe
#define f21	21	// caller safe
#define f22	22	// caller safe
#define f23	23	// caller safe
#define f24	24	// caller safe
#define f25	25	// caller safe
#define f26	26	// caller safe
#define f27	27	// caller safe
#define f28	28	// caller safe
#define f29	29	// caller safe
#define f30	30	// caller safe
#define f31	31	// caller safe

#define cr0	0	// volatile
#define cr1	1	// volatile
#define cr2	2	// caller safe
#define cr3	3	// caller safe
#define cr4	4	// caller safe
#define cr5	5	// volatile
#define cr6	6	// volatile
#define cr7	7	// volatile

#define lt	0
#define gt	1
#define eq	2
#define so	3

// branch bo field values
#define branchLikely	1
#define branchFalse	4
#define branchTrue	12
#define branchAlways	20

// branch extensions (change branch type)
#define branchExtLink	0x0001


/*
 * This list must match exactly the powerpc_opcodes list from vm_powerpc_asm.c
 * If you're changing the original list remember to regenerate this one. You
 * may do so using this perl script:
   perl -p -e 'BEGIN{%t=("-"=>m=>"+"=>p=>"."=>d=>);$l=""}$o=0 if/^}/;
	if($o && s/^{ "(.*?)([\.+-])?".+/i\U$1\E$t{$2}/s){$_.="_" while$l{$_};
	$l{$_}=1;if(length $l.$_ > 70){$s=$_;$_="\t$l\n";$l="$s,"}else
	{$l.=" $_,";$_=undef}}else{$o=1 if/powerpc_opcodes.*=/;$_=undef};
	END{print "\t$l\n"}' < vm_powerpc_asm.c
 */

typedef enum powerpc_iname {
	iCMPLWI, iCMPWI, iCMPW, iCMPLW, iFCMPU, iLI, iLIS, iADDI, iADDIS,
	iBLTm, iBC, iBCL, iB, iBL, iBLR, iBCTR, iBCTRL, iRLWINM, iNOP, iORI,
	iXORIS, iLDX, iLWZX, iSLW, iAND, iSUB, iLBZX, iNEG, iNOT, iSTWX, iSTBX,
	iMULLW, iADD, iLHZX, iXOR, iMFLR, iSTHX, iMR, iOR, iDIVWU, iMTLR,
	iMTCTR, iDIVW, iLFSX, iSRW, iSTFSX, iSRAW, iEXTSH, iEXTSB, iLWZ, iLBZ,
	iSTW, iSTWU, iSTB, iLHZ, iSTH, iLFS, iLFD, iSTFS, iSTFD, iLD, iFDIVS,
	iFSUBS, iFADDS, iFMULS, iSTD, iSTDU, iFRSP, iFCTIWZ, iFSUB, iFNEG,
} powerpc_iname_t;

#include <stdint.h>

typedef uint32_t ppc_instruction_t;

extern ppc_instruction_t
asm_instruction( powerpc_iname_t, const int, const long int * );

#define IN( inst, args... ) \
({\
	const long int argv[] = { args };\
	const int argc = sizeof( argv ) / sizeof( argv[0] ); \
	asm_instruction( inst, argc, argv );\
})

/* return nop on error */
#define ASM_ERROR_OPC (0x60000000)

/*
 * BEGIN OF ppc.h
 */

#define ppc_cpu_t int

struct powerpc_opcode
{
	const char *name;
	unsigned long opcode;
	unsigned long mask;
	ppc_cpu_t flags;
	unsigned char operands[8];
};

static const struct powerpc_opcode powerpc_opcodes[];

#define PPC_OPCODE_PPC			 1
#define PPC_OPCODE_POWER		 2
#define PPC_OPCODE_POWER2		 4
#define PPC_OPCODE_32			 8
#define PPC_OPCODE_64		      0x10
#define PPC_OPCODE_601		      0x20
#define PPC_OPCODE_COMMON	      0x40
#define PPC_OPCODE_ANY		      0x80
#define PPC_OPCODE_64_BRIDGE	     0x100
#define PPC_OPCODE_ALTIVEC	     0x200
#define PPC_OPCODE_403		     0x400
#define PPC_OPCODE_BOOKE	     0x800
#define PPC_OPCODE_BOOKE64	    0x1000
#define PPC_OPCODE_440		    0x2000
#define PPC_OPCODE_POWER4	    0x4000
#define PPC_OPCODE_NOPOWER4	    0x8000
#define PPC_OPCODE_CLASSIC	   0x10000
#define PPC_OPCODE_SPE		   0x20000
#define PPC_OPCODE_ISEL		   0x40000
#define PPC_OPCODE_EFS		   0x80000
#define PPC_OPCODE_BRLOCK	  0x100000
#define PPC_OPCODE_PMR		  0x200000
#define PPC_OPCODE_CACHELCK	  0x400000
#define PPC_OPCODE_RFMCI	  0x800000
#define PPC_OPCODE_POWER5	 0x1000000
#define PPC_OPCODE_E300		 0x2000000
#define PPC_OPCODE_POWER6	 0x4000000
#define PPC_OPCODE_CELL		 0x8000000
#define PPC_OPCODE_PPCPS	0x10000000
#define PPC_OPCODE_E500MC	0x20000000
#define PPC_OPCODE_405		0x40000000
#define PPC_OPCODE_VSX		0x80000000

#define PPC_OP(i) (((i) >> 26) & 0x3f)

struct powerpc_operand
{
	unsigned int bitm;
	int shift;
	unsigned long (*insert)
		(unsigned long, long, int, const char **);
	unsigned long flags;
};

static const struct powerpc_operand powerpc_operands[];

#define PPC_OPERAND_SIGNED (0x1)
#define PPC_OPERAND_SIGNOPT (0x2)
#define PPC_OPERAND_FAKE (0x4)
#define PPC_OPERAND_PARENS (0x8)
#define PPC_OPERAND_CR (0x10)
#define PPC_OPERAND_GPR (0x20)
#define PPC_OPERAND_GPR_0 (0x40)
#define PPC_OPERAND_FPR (0x80)
#define PPC_OPERAND_RELATIVE (0x100)
#define PPC_OPERAND_ABSOLUTE (0x200)
#define PPC_OPERAND_OPTIONAL (0x400)
#define PPC_OPERAND_NEXT (0x800)
#define PPC_OPERAND_NEGATIVE (0x1000)
#define PPC_OPERAND_VR (0x2000)
#define PPC_OPERAND_DS (0x4000)
#define PPC_OPERAND_DQ (0x8000)
#define PPC_OPERAND_PLUS1 (0x10000)
#define PPC_OPERAND_FSL (0x20000)
#define PPC_OPERAND_FCR (0x40000)
#define PPC_OPERAND_UDI (0x80000)
#define PPC_OPERAND_VSR (0x100000)

/*
 * END OF ppc.h
 */

#define PPC_DEST_ARCH PPC_OPCODE_PPC

ppc_instruction_t
asm_instruction( powerpc_iname_t sname, const int argc, const long int *argv )
{
	const char *errmsg = NULL;
	const char *name;
	unsigned long int ret;
	const struct powerpc_opcode *opcode = NULL;
	int argi, argj;

	opcode = &powerpc_opcodes[ sname ];
	name = opcode->name;

	if ( ! opcode ) {
		printf( "Can't find opcode %d\n", sname );
		return ASM_ERROR_OPC;
	}
	if ( ( opcode->flags & PPC_DEST_ARCH ) != PPC_DEST_ARCH ) {
		printf( "opcode %s not defined for this arch\n", name );
		return ASM_ERROR_OPC;
	}

	ret = opcode->opcode;

	argi = argj = 0;
	while ( opcode->operands[ argi ] != 0 ) {
		long int op = 0;
		const struct powerpc_operand *operand = &powerpc_operands[ opcode->operands[ argi ] ];

		if ( ! (operand->flags & PPC_OPERAND_FAKE) ) {
			if ( argj >= argc ) {
				printf( "Not enough arguments for %s, got %d\n", name, argc );
				return ASM_ERROR_OPC;
			}

			op = argv[ argj++ ];
		}

		if ( operand->insert ) {
			errmsg = NULL;
			ret = operand->insert( ret, op, PPC_DEST_ARCH, &errmsg );
			if ( errmsg ) {
				printf( "%s: error while inserting operand %d (0x%.2lx): %s\n",
					name, argi, op, errmsg );
			}
		} else {
			unsigned long int opu = *(unsigned long int *)&op;
			unsigned long int bitm = operand->bitm;
			unsigned long int bitm_full = bitm | ( bitm & 1 ? 0 : 0xf );

			if ( operand->flags & PPC_OPERAND_SIGNED ) {
				bitm_full >>= 1;

				if ( ( opu & ~bitm_full ) != 0 && ( opu | bitm_full ) != -1 )
					printf( "%s: signed operand nr.%d to wide. op: %.8lx, mask: %.8lx\n",
						name, argi, opu, bitm );
			} else {
				if ( ( opu & ~bitm_full ) != 0 )
					printf( "%s: unsigned operand nr.%d to wide. op: %.8lx, mask: %.8lx\n",
						name, argi, opu, bitm );
			}
			if ( (bitm & 1) == 0 ) {
				if ( opu & 0xf & ~bitm )
					printf( "%s: operand nr.%d not aligned correctly. op: %.8lx, mask: %.8lx\n",
						name, argi, opu, bitm );
			}

			ret |= ( op & operand->bitm ) << operand->shift;
		}
		argi++;
	}
	if ( argc > argj ) {
		printf( "Too many arguments for %s, got %d\n", name, argc );
		return ASM_ERROR_OPC;
	}

	return ret;
}


/*
 * BEGIN OF ppc-opc.c
 */

#define ATTRIBUTE_UNUSED
#define _(x) (x)

/* Local insertion and extraction functions. */

static unsigned long insert_bdm (unsigned long, long, int, const char **);
static unsigned long insert_bo (unsigned long, long, int, const char **);
static unsigned long insert_ras (unsigned long, long, int, const char **);
static unsigned long insert_rbs (unsigned long, long, int, const char **);

/* The operands table.

   The fields are bitm, shift, insert, extract, flags.
   */

static const struct powerpc_operand powerpc_operands[] =
{
  /* The zero index is used to indicate the end of the list of
     operands.  */
#define UNUSED 0
  { 0, 0, NULL, 0 },

  /* The BA field in an XL form instruction.  */
#define BA UNUSED + 1
  /* The BI field in a B form or XL form instruction.  */
#define BI BA
#define BI_MASK (0x1f << 16)
  { 0x1f, 16, NULL, PPC_OPERAND_CR },

  /* The BD field in a B form instruction.  The lower two bits are
     forced to zero.  */
#define BD BA + 1
  { 0xfffc, 0, NULL, PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },

  /* The BD field in a B form instruction when the - modifier is used.
     This sets the y bit of the BO field appropriately.  */
#define BDM BD + 1
  { 0xfffc, 0, insert_bdm,
      PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },

  /* The BF field in an X or XL form instruction.  */
#define BF BDM + 1
  /* The CRFD field in an X form instruction.  */
#define CRFD BF
  { 0x7, 23, NULL, PPC_OPERAND_CR },

  /* An optional BF field.  This is used for comparison instructions,
     in which an omitted BF field is taken as zero.  */
#define OBF BF + 1
  { 0x7, 23, NULL, PPC_OPERAND_CR | PPC_OPERAND_OPTIONAL },

  /* The BO field in a B form instruction.  Certain values are
     illegal.  */
#define BO OBF + 1
#define BO_MASK (0x1f << 21)
  { 0x1f, 21, insert_bo, 0 },

  /* The condition register number portion of the BI field in a B form
     or XL form instruction.  This is used for the extended
     conditional branch mnemonics, which set the lower two bits of the
     BI field.  This field is optional.  */
#define CR BO + 1
  { 0x7, 18, NULL, PPC_OPERAND_CR | PPC_OPERAND_OPTIONAL },

  /* The D field in a D form instruction.  This is a displacement off
     a register, and implies that the next operand is a register in
     parentheses.  */
#define D CR + 1
  { 0xffff, 0, NULL, PPC_OPERAND_PARENS | PPC_OPERAND_SIGNED },

  /* The DS field in a DS form instruction.  This is like D, but the
     lower two bits are forced to zero.  */
#define DS D + 1
  { 0xfffc, 0, NULL,
    PPC_OPERAND_PARENS | PPC_OPERAND_SIGNED | PPC_OPERAND_DS },

  /* The FRA field in an X or A form instruction.  */
#define FRA DS + 1
#define FRA_MASK (0x1f << 16)
  { 0x1f, 16, NULL, PPC_OPERAND_FPR },

  /* The FRB field in an X or A form instruction.  */
#define FRB FRA + 1
#define FRB_MASK (0x1f << 11)
  { 0x1f, 11, NULL, PPC_OPERAND_FPR },

  /* The FRC field in an A form instruction.  */
#define FRC FRB + 1
#define FRC_MASK (0x1f << 6)
  { 0x1f, 6, NULL, PPC_OPERAND_FPR },

  /* The FRS field in an X form instruction or the FRT field in a D, X
     or A form instruction.  */
#define FRS FRC + 1
#define FRT FRS
  { 0x1f, 21, NULL, PPC_OPERAND_FPR },

  /* The LI field in an I form instruction.  The lower two bits are
     forced to zero.  */
#define LI FRS + 1
  { 0x3fffffc, 0, NULL, PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },

  /* The ME field in an M form instruction.  */
#define ME LI + 1
#define ME_MASK (0x1f << 1)
  { 0x1f, 1, NULL, 0 },

  /* The MB and ME fields in an M form instruction expressed a single
     operand which is a bitmask indicating which bits to select.  This
     is a two operand form using PPC_OPERAND_NEXT.  See the
     description in opcode/ppc.h for what this means.  */
#define MBE ME + 1
  { 0x1f, 6, NULL, PPC_OPERAND_OPTIONAL | PPC_OPERAND_NEXT },

  /* The RA field in an D, DS, DQ, X, XO, M, or MDS form instruction.  */
#define RA MBE + 1
#define RA_MASK (0x1f << 16)
  { 0x1f, 16, NULL, PPC_OPERAND_GPR },

  /* As above, but 0 in the RA field means zero, not r0.  */
#define RA0 RA + 1
  { 0x1f, 16, NULL, PPC_OPERAND_GPR_0 },

  /* The RA field in a D or X form instruction which is an updating
     store or an updating floating point load, which means that the RA
     field may not be zero.  */
#define RAS RA0 + 1
  { 0x1f, 16, insert_ras, PPC_OPERAND_GPR_0 },

  /* The RB field in an X, XO, M, or MDS form instruction.  */
#define RB RAS + 1
#define RB_MASK (0x1f << 11)
  { 0x1f, 11, NULL, PPC_OPERAND_GPR },

  /* The RB field in an X form instruction when it must be the same as
     the RS field in the instruction.  This is used for extended
     mnemonics like mr.  */
#define RBS RB + 1
  { 0x1f, 11, insert_rbs, PPC_OPERAND_FAKE },

  /* The RS field in a D, DS, X, XFX, XS, M, MD or MDS form
     instruction or the RT field in a D, DS, X, XFX or XO form
     instruction.  */
#define RS RBS + 1
#define RT RS
#define RT_MASK (0x1f << 21)
  { 0x1f, 21, NULL, PPC_OPERAND_GPR },

  /* The SH field in an X or M form instruction.  */
#define SH RS + 1
#define SH_MASK (0x1f << 11)
  /* The other UIMM field in an EVX form instruction.  */
#define EVUIMM SH
  { 0x1f, 11, NULL, 0 },

  /* The SI field in a D form instruction.  */
#define SI SH + 1
  { 0xffff, 0, NULL, PPC_OPERAND_SIGNED },

  /* The UI field in a D form instruction.  */
#define UI SI + 1
  { 0xffff, 0, NULL, 0 },

};


/* The functions used to insert and extract complicated operands.  */

/* The BD field in a B form instruction when the - modifier is used.
   This modifier means that the branch is not expected to be taken.
   For chips built to versions of the architecture prior to version 2
   (ie. not Power4 compatible), we set the y bit of the BO field to 1
   if the offset is negative.  When extracting, we require that the y
   bit be 1 and that the offset be positive, since if the y bit is 0
   we just want to print the normal form of the instruction.
   Power4 compatible targets use two bits, "a", and "t", instead of
   the "y" bit.  "at" == 00 => no hint, "at" == 01 => unpredictable,
   "at" == 10 => not taken, "at" == 11 => taken.  The "t" bit is 00001
   in BO field, the "a" bit is 00010 for branch on CR(BI) and 01000
   for branch on CTR.  We only handle the taken/not-taken hint here.
   Note that we don't relax the conditions tested here when
   disassembling with -Many because insns using extract_bdm and
   extract_bdp always occur in pairs.  One or the other will always
   be valid.  */

static unsigned long
insert_bdm (unsigned long insn,
	    long value,
	    int dialect,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  if ((dialect & PPC_OPCODE_POWER4) == 0)
    {
      if ((value & 0x8000) != 0)
	insn |= 1 << 21;
    }
  else
    {
      if ((insn & (0x14 << 21)) == (0x04 << 21))
	insn |= 0x02 << 21;
      else if ((insn & (0x14 << 21)) == (0x10 << 21))
	insn |= 0x08 << 21;
    }
  return insn | (value & 0xfffc);
}


/* Check for legal values of a BO field.  */

static int
valid_bo (long value, int dialect, int extract)
{
  if ((dialect & PPC_OPCODE_POWER4) == 0)
    {
      int valid;
      /* Certain encodings have bits that are required to be zero.
	 These are (z must be zero, y may be anything):
	     001zy
	     011zy
	     1z00y
	     1z01y
	     1z1zz
      */
      switch (value & 0x14)
	{
	default:
	case 0:
	  valid = 1;
	  break;
	case 0x4:
	  valid = (value & 0x2) == 0;
	  break;
	case 0x10:
	  valid = (value & 0x8) == 0;
	  break;
	case 0x14:
	  valid = value == 0x14;
	  break;
	}
      /* When disassembling with -Many, accept power4 encodings too.  */
      if (valid
	  || (dialect & PPC_OPCODE_ANY) == 0
	  || !extract)
	return valid;
    }

  /* Certain encodings have bits that are required to be zero.
     These are (z must be zero, a & t may be anything):
	 0000z
	 0001z
	 0100z
	 0101z
	 001at
	 011at
	 1a00t
	 1a01t
	 1z1zz
  */
  if ((value & 0x14) == 0)
    return (value & 0x1) == 0;
  else if ((value & 0x14) == 0x14)
    return value == 0x14;
  else
    return 1;
}

/* The BO field in a B form instruction.  Warn about attempts to set
   the field to an illegal value.  */

static unsigned long
insert_bo (unsigned long insn,
	   long value,
	   int dialect,
	   const char **errmsg)
{
  if (!valid_bo (value, dialect, 0))
    *errmsg = _("invalid conditional option");
  return insn | ((value & 0x1f) << 21);
}

/* The RA field in a D or X form instruction which is an updating
   store or an updating floating point load, which means that the RA
   field may not be zero.  */

static unsigned long
insert_ras (unsigned long insn,
	    long value,
	    int dialect ATTRIBUTE_UNUSED,
	    const char **errmsg)
{
  if (value == 0)
    *errmsg = _("invalid register operand when updating");
  return insn | ((value & 0x1f) << 16);
}

/* The RB field in an X form instruction when it must be the same as
   the RS field in the instruction.  This is used for extended
   mnemonics like mr.  This operand is marked FAKE.  The insertion
   function just copies the BT field into the BA field, and the
   extraction function just checks that the fields are the same.  */

static unsigned long
insert_rbs (unsigned long insn,
	    long value ATTRIBUTE_UNUSED,
	    int dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | (((insn >> 21) & 0x1f) << 11);
}


/* Macros used to form opcodes.  */

/* The main opcode.  */
#define OP(x) ((((unsigned long)(x)) & 0x3f) << 26)
#define OP_MASK OP (0x3f)

/* The main opcode combined with a trap code in the TO field of a D
   form instruction.  Used for extended mnemonics for the trap
   instructions.  */
#define OPTO(x,to) (OP (x) | ((((unsigned long)(to)) & 0x1f) << 21))
#define OPTO_MASK (OP_MASK | TO_MASK)

/* The main opcode combined with a comparison size bit in the L field
   of a D form or X form instruction.  Used for extended mnemonics for
   the comparison instructions.  */
#define OPL(x,l) (OP (x) | ((((unsigned long)(l)) & 1) << 21))
#define OPL_MASK OPL (0x3f,1)

/* An A form instruction.  */
#define A(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0x1f) << 1) | (((unsigned long)(rc)) & 1))
#define A_MASK A (0x3f, 0x1f, 1)

/* An A_MASK with the FRB field fixed.  */
#define AFRB_MASK (A_MASK | FRB_MASK)

/* An A_MASK with the FRC field fixed.  */
#define AFRC_MASK (A_MASK | FRC_MASK)

/* An A_MASK with the FRA and FRC fields fixed.  */
#define AFRAFRC_MASK (A_MASK | FRA_MASK | FRC_MASK)

/* An AFRAFRC_MASK, but with L bit clear.  */
#define AFRALFRC_MASK (AFRAFRC_MASK & ~((unsigned long) 1 << 16))

/* A B form instruction.  */
#define B(op, aa, lk) (OP (op) | ((((unsigned long)(aa)) & 1) << 1) | ((lk) & 1))
#define B_MASK B (0x3f, 1, 1)

/* A B form instruction setting the BO field.  */
#define BBO(op, bo, aa, lk) (B ((op), (aa), (lk)) | ((((unsigned long)(bo)) & 0x1f) << 21))
#define BBO_MASK BBO (0x3f, 0x1f, 1, 1)

/* A BBO_MASK with the y bit of the BO field removed.  This permits
   matching a conditional branch regardless of the setting of the y
   bit.  Similarly for the 'at' bits used for power4 branch hints.  */
#define Y_MASK   (((unsigned long) 1) << 21)
#define AT1_MASK (((unsigned long) 3) << 21)
#define AT2_MASK (((unsigned long) 9) << 21)
#define BBOY_MASK  (BBO_MASK &~ Y_MASK)
#define BBOAT_MASK (BBO_MASK &~ AT1_MASK)

/* A B form instruction setting the BO field and the condition bits of
   the BI field.  */
#define BBOCB(op, bo, cb, aa, lk) \
  (BBO ((op), (bo), (aa), (lk)) | ((((unsigned long)(cb)) & 0x3) << 16))
#define BBOCB_MASK BBOCB (0x3f, 0x1f, 0x3, 1, 1)

/* A BBOCB_MASK with the y bit of the BO field removed.  */
#define BBOYCB_MASK (BBOCB_MASK &~ Y_MASK)
#define BBOATCB_MASK (BBOCB_MASK &~ AT1_MASK)
#define BBOAT2CB_MASK (BBOCB_MASK &~ AT2_MASK)

/* A BBOYCB_MASK in which the BI field is fixed.  */
#define BBOYBI_MASK (BBOYCB_MASK | BI_MASK)
#define BBOATBI_MASK (BBOAT2CB_MASK | BI_MASK)

/* An Context form instruction.  */
#define CTX(op, xop)   (OP (op) | (((unsigned long)(xop)) & 0x7))
#define CTX_MASK CTX(0x3f, 0x7)

/* An User Context form instruction.  */
#define UCTX(op, xop)  (OP (op) | (((unsigned long)(xop)) & 0x1f))
#define UCTX_MASK UCTX(0x3f, 0x1f)

/* The main opcode mask with the RA field clear.  */
#define DRA_MASK (OP_MASK | RA_MASK)

/* A DS form instruction.  */
#define DSO(op, xop) (OP (op) | ((xop) & 0x3))
#define DS_MASK DSO (0x3f, 3)

/* A DE form instruction.  */
#define DEO(op, xop) (OP (op) | ((xop) & 0xf))
#define DE_MASK DEO (0x3e, 0xf)

/* An EVSEL form instruction.  */
#define EVSEL(op, xop) (OP (op) | (((unsigned long)(xop)) & 0xff) << 3)
#define EVSEL_MASK EVSEL(0x3f, 0xff)

/* An M form instruction.  */
#define M(op, rc) (OP (op) | ((rc) & 1))
#define M_MASK M (0x3f, 1)

/* An M form instruction with the ME field specified.  */
#define MME(op, me, rc) (M ((op), (rc)) | ((((unsigned long)(me)) & 0x1f) << 1))

/* An M_MASK with the MB and ME fields fixed.  */
#define MMBME_MASK (M_MASK | MB_MASK | ME_MASK)

/* An M_MASK with the SH and ME fields fixed.  */
#define MSHME_MASK (M_MASK | SH_MASK | ME_MASK)

/* An MD form instruction.  */
#define MD(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0x7) << 2) | ((rc) & 1))
#define MD_MASK MD (0x3f, 0x7, 1)

/* An MD_MASK with the MB field fixed.  */
#define MDMB_MASK (MD_MASK | MB6_MASK)

/* An MD_MASK with the SH field fixed.  */
#define MDSH_MASK (MD_MASK | SH6_MASK)

/* An MDS form instruction.  */
#define MDS(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0xf) << 1) | ((rc) & 1))
#define MDS_MASK MDS (0x3f, 0xf, 1)

/* An MDS_MASK with the MB field fixed.  */
#define MDSMB_MASK (MDS_MASK | MB6_MASK)

/* An SC form instruction.  */
#define SC(op, sa, lk) (OP (op) | ((((unsigned long)(sa)) & 1) << 1) | ((lk) & 1))
#define SC_MASK (OP_MASK | (((unsigned long)0x3ff) << 16) | (((unsigned long)1) << 1) | 1)

/* A VX form instruction.  */
#define VX(op, xop) (OP (op) | (((unsigned long)(xop)) & 0x7ff))

/* The mask for a VX form instruction.  */
#define VX_MASK	VX(0x3f, 0x7ff)

/* A VA form instruction.  */
#define VXA(op, xop) (OP (op) | (((unsigned long)(xop)) & 0x03f))

/* The mask for a VA form instruction.  */
#define VXA_MASK VXA(0x3f, 0x3f)

/* A VXR form instruction.  */
#define VXR(op, xop, rc) (OP (op) | (((rc) & 1) << 10) | (((unsigned long)(xop)) & 0x3ff))

/* The mask for a VXR form instruction.  */
#define VXR_MASK VXR(0x3f, 0x3ff, 1)

/* An X form instruction.  */
#define X(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x3ff) << 1))

/* A Z form instruction.  */
#define Z(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x1ff) << 1))

/* An X form instruction with the RC bit specified.  */
#define XRC(op, xop, rc) (X ((op), (xop)) | ((rc) & 1))

/* A Z form instruction with the RC bit specified.  */
#define ZRC(op, xop, rc) (Z ((op), (xop)) | ((rc) & 1))

/* The mask for an X form instruction.  */
#define X_MASK XRC (0x3f, 0x3ff, 1)

/* The mask for a Z form instruction.  */
#define Z_MASK ZRC (0x3f, 0x1ff, 1)
#define Z2_MASK ZRC (0x3f, 0xff, 1)

/* An X_MASK with the RA field fixed.  */
#define XRA_MASK (X_MASK | RA_MASK)

/* An XRA_MASK with the W field clear.  */
#define XWRA_MASK (XRA_MASK & ~((unsigned long) 1 << 16))

/* An X_MASK with the RB field fixed.  */
#define XRB_MASK (X_MASK | RB_MASK)

/* An X_MASK with the RT field fixed.  */
#define XRT_MASK (X_MASK | RT_MASK)

/* An XRT_MASK mask with the L bits clear.  */
#define XLRT_MASK (XRT_MASK & ~((unsigned long) 0x3 << 21))

/* An X_MASK with the RA and RB fields fixed.  */
#define XRARB_MASK (X_MASK | RA_MASK | RB_MASK)

/* An XRARB_MASK, but with the L bit clear.  */
#define XRLARB_MASK (XRARB_MASK & ~((unsigned long) 1 << 16))

/* An X_MASK with the RT and RA fields fixed.  */
#define XRTRA_MASK (X_MASK | RT_MASK | RA_MASK)

/* An XRTRA_MASK, but with L bit clear.  */
#define XRTLRA_MASK (XRTRA_MASK & ~((unsigned long) 1 << 21))

/* An X form instruction with the L bit specified.  */
#define XOPL(op, xop, l) (X ((op), (xop)) | ((((unsigned long)(l)) & 1) << 21))

/* The mask for an X form comparison instruction.  */
#define XCMP_MASK (X_MASK | (((unsigned long)1) << 22))

/* The mask for an X form comparison instruction with the L field
   fixed.  */
#define XCMPL_MASK (XCMP_MASK | (((unsigned long)1) << 21))

/* An X form trap instruction with the TO field specified.  */
#define XTO(op, xop, to) (X ((op), (xop)) | ((((unsigned long)(to)) & 0x1f) << 21))
#define XTO_MASK (X_MASK | TO_MASK)

/* An X form tlb instruction with the SH field specified.  */
#define XTLB(op, xop, sh) (X ((op), (xop)) | ((((unsigned long)(sh)) & 0x1f) << 11))
#define XTLB_MASK (X_MASK | SH_MASK)

/* An X form sync instruction.  */
#define XSYNC(op, xop, l) (X ((op), (xop)) | ((((unsigned long)(l)) & 3) << 21))

/* An X form sync instruction with everything filled in except the LS field.  */
#define XSYNC_MASK (0xff9fffff)

/* An X_MASK, but with the EH bit clear.  */
#define XEH_MASK (X_MASK & ~((unsigned long )1))

/* An X form AltiVec dss instruction.  */
#define XDSS(op, xop, a) (X ((op), (xop)) | ((((unsigned long)(a)) & 1) << 25))
#define XDSS_MASK XDSS(0x3f, 0x3ff, 1)

/* An XFL form instruction.  */
#define XFL(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0x3ff) << 1) | (((unsigned long)(rc)) & 1))
#define XFL_MASK XFL (0x3f, 0x3ff, 1)

/* An X form isel instruction.  */
#define XISEL(op, xop)  (OP (op) | ((((unsigned long)(xop)) & 0x1f) << 1))
#define XISEL_MASK      XISEL(0x3f, 0x1f)

/* An XL form instruction with the LK field set to 0.  */
#define XL(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x3ff) << 1))

/* An XL form instruction which uses the LK field.  */
#define XLLK(op, xop, lk) (XL ((op), (xop)) | ((lk) & 1))

/* The mask for an XL form instruction.  */
#define XL_MASK XLLK (0x3f, 0x3ff, 1)

/* An XL form instruction which explicitly sets the BO field.  */
#define XLO(op, bo, xop, lk) \
  (XLLK ((op), (xop), (lk)) | ((((unsigned long)(bo)) & 0x1f) << 21))
#define XLO_MASK (XL_MASK | BO_MASK)

/* An XL form instruction which explicitly sets the y bit of the BO
   field.  */
#define XLYLK(op, xop, y, lk) (XLLK ((op), (xop), (lk)) | ((((unsigned long)(y)) & 1) << 21))
#define XLYLK_MASK (XL_MASK | Y_MASK)

/* An XL form instruction which sets the BO field and the condition
   bits of the BI field.  */
#define XLOCB(op, bo, cb, xop, lk) \
  (XLO ((op), (bo), (xop), (lk)) | ((((unsigned long)(cb)) & 3) << 16))
#define XLOCB_MASK XLOCB (0x3f, 0x1f, 0x3, 0x3ff, 1)

#define BB_MASK (0x1f << 11)
/* An XL_MASK or XLYLK_MASK or XLOCB_MASK with the BB field fixed.  */
#define XLBB_MASK (XL_MASK | BB_MASK)
#define XLYBB_MASK (XLYLK_MASK | BB_MASK)
#define XLBOCBBB_MASK (XLOCB_MASK | BB_MASK)

/* A mask for branch instructions using the BH field.  */
#define XLBH_MASK (XL_MASK | (0x1c << 11))

/* An XL_MASK with the BO and BB fields fixed.  */
#define XLBOBB_MASK (XL_MASK | BO_MASK | BB_MASK)

/* An XL_MASK with the BO, BI and BB fields fixed.  */
#define XLBOBIBB_MASK (XL_MASK | BO_MASK | BI_MASK | BB_MASK)

/* An XO form instruction.  */
#define XO(op, xop, oe, rc) \
  (OP (op) | ((((unsigned long)(xop)) & 0x1ff) << 1) | ((((unsigned long)(oe)) & 1) << 10) | (((unsigned long)(rc)) & 1))
#define XO_MASK XO (0x3f, 0x1ff, 1, 1)

/* An XO_MASK with the RB field fixed.  */
#define XORB_MASK (XO_MASK | RB_MASK)

/* An XS form instruction.  */
#define XS(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0x1ff) << 2) | (((unsigned long)(rc)) & 1))
#define XS_MASK XS (0x3f, 0x1ff, 1)

/* A mask for the FXM version of an XFX form instruction.  */
#define XFXFXM_MASK (X_MASK | (1 << 11) | (1 << 20))

/* An XFX form instruction with the FXM field filled in.  */
#define XFXM(op, xop, fxm, p4) \
  (X ((op), (xop)) | ((((unsigned long)(fxm)) & 0xff) << 12) \
   | ((unsigned long)(p4) << 20))

#define SPR_MASK (0x3ff << 11)
/* An XFX form instruction with the SPR field filled in.  */
#define XSPR(op, xop, spr) \
  (X ((op), (xop)) | ((((unsigned long)(spr)) & 0x1f) << 16) | ((((unsigned long)(spr)) & 0x3e0) << 6))
#define XSPR_MASK (X_MASK | SPR_MASK)

/* An XFX form instruction with the SPR field filled in except for the
   SPRBAT field.  */
#define XSPRBAT_MASK (XSPR_MASK &~ SPRBAT_MASK)

/* An XFX form instruction with the SPR field filled in except for the
   SPRG field.  */
#define XSPRG_MASK (XSPR_MASK & ~(0x1f << 16))

/* An X form instruction with everything filled in except the E field.  */
#define XE_MASK (0xffff7fff)

/* An X form user context instruction.  */
#define XUC(op, xop)  (OP (op) | (((unsigned long)(xop)) & 0x1f))
#define XUC_MASK      XUC(0x3f, 0x1f)

/* The BO encodings used in extended conditional branch mnemonics.  */
#define BODNZF	(0x0)
#define BODNZFP	(0x1)
#define BODZF	(0x2)
#define BODZFP	(0x3)
#define BODNZT	(0x8)
#define BODNZTP	(0x9)
#define BODZT	(0xa)
#define BODZTP	(0xb)

#define BOF	(0x4)
#define BOFP	(0x5)
#define BOFM4	(0x6)
#define BOFP4	(0x7)
#define BOT	(0xc)
#define BOTP	(0xd)
#define BOTM4	(0xe)
#define BOTP4	(0xf)

#define BODNZ	(0x10)
#define BODNZP	(0x11)
#define BODZ	(0x12)
#define BODZP	(0x13)
#define BODNZM4 (0x18)
#define BODNZP4 (0x19)
#define BODZM4	(0x1a)
#define BODZP4	(0x1b)

#define BOU	(0x14)

/* The BI condition bit encodings used in extended conditional branch
   mnemonics.  */
#define CBLT	(0)
#define CBGT	(1)
#define CBEQ	(2)
#define CBSO	(3)

/* The TO encodings used in extended trap mnemonics.  */
#define TOLGT	(0x1)
#define TOLLT	(0x2)
#define TOEQ	(0x4)
#define TOLGE	(0x5)
#define TOLNL	(0x5)
#define TOLLE	(0x6)
#define TOLNG	(0x6)
#define TOGT	(0x8)
#define TOGE	(0xc)
#define TONL	(0xc)
#define TOLT	(0x10)
#define TOLE	(0x14)
#define TONG	(0x14)
#define TONE	(0x18)
#define TOU	(0x1f)

/* Smaller names for the flags so each entry in the opcodes table will
   fit on a single line.  */
#undef	PPC
#define PPC     PPC_OPCODE_PPC
#define PPCCOM	PPC_OPCODE_PPC | PPC_OPCODE_COMMON
#define PPC64   PPC_OPCODE_64 | PPC_OPCODE_PPC
#define	COM     PPC_OPCODE_POWER | PPC_OPCODE_PPC | PPC_OPCODE_COMMON
#define	COM32   PPC_OPCODE_POWER | PPC_OPCODE_PPC | PPC_OPCODE_COMMON | PPC_OPCODE_32

/* The opcode table.

   The format of the opcode table is:

   NAME	     OPCODE	MASK		FLAGS		{ OPERANDS }

   NAME is the name of the instruction.
   OPCODE is the instruction opcode.
   MASK is the opcode mask; this is used to tell the disassembler
     which bits in the actual opcode must match OPCODE.
   FLAGS are flags indicated what processors support the instruction.
   OPERANDS is the list of operands.

   The disassembler reads the table in order and prints the first
   instruction which matches, so this table is sorted to put more
   specific instructions before more general instructions.  It is also
   sorted by major opcode.  */

static const struct powerpc_opcode powerpc_opcodes[] = {

{ "cmplwi",  OPL(10,0),	OPL_MASK,	PPCCOM,		{ OBF, RA, UI } },
{ "cmpwi",   OPL(11,0),	OPL_MASK,	PPCCOM,		{ OBF, RA, SI } },
{ "cmpw",    XOPL(31,0,0), XCMPL_MASK,	PPCCOM,		{ OBF, RA, RB } },
{ "cmplw",   XOPL(31,32,0), XCMPL_MASK, PPCCOM,	{ OBF, RA, RB } },
{ "fcmpu",   X(63,0),	X_MASK|(3<<21),	COM,		{ BF, FRA, FRB } },

{ "li",	     OP(14),	DRA_MASK,	PPCCOM,		{ RT, SI } },
{ "lis",     OP(15),	DRA_MASK,	PPCCOM,		{ RT, SI } },

{ "addi",    OP(14),	OP_MASK,	PPCCOM,		{ RT, RA0, SI } },
{ "addis",   OP(15),	OP_MASK,	PPCCOM,		{ RT,RA0,SI } },
{ "blt-",    BBOCB(16,BOT,CBLT,0,0), BBOATCB_MASK, PPCCOM,	{ CR, BDM } },
{ "bc",	     B(16,0,0),	B_MASK,		COM,		{ BO, BI, BD } },
{ "bcl",     B(16,0,1),	B_MASK,		COM,		{ BO, BI, BD } },
{ "b",	     B(18,0,0),	B_MASK,		COM,		{ LI } },
{ "bl",      B(18,0,1),	B_MASK,		COM,		{ LI } },
{ "blr",     XLO(19,BOU,16,0), XLBOBIBB_MASK, PPCCOM,	{ 0 } },
{ "bctr",    XLO(19,BOU,528,0), XLBOBIBB_MASK, COM,	{ 0 } },
{ "bctrl",   XLO(19,BOU,528,1), XLBOBIBB_MASK, COM,	{ 0 } },

{ "rlwinm",  M(21,0),	M_MASK,		PPCCOM,		{ RA,RS,SH,MBE,ME } },
{ "nop",     OP(24),	0xffffffff,	PPCCOM,		{ 0 } },
{ "ori",     OP(24),	OP_MASK,	PPCCOM,		{ RA, RS, UI } },
{ "xoris",   OP(27),	OP_MASK,	PPCCOM,		{ RA, RS, UI } },
{ "ldx",     X(31,21),	X_MASK,		PPC64,		{ RT, RA0, RB } },
{ "lwzx",    X(31,23),	X_MASK,		PPCCOM,		{ RT, RA0, RB } },
{ "slw",     XRC(31,24,0), X_MASK,	PPCCOM,		{ RA, RS, RB } },
{ "and",     XRC(31,28,0), X_MASK,	COM,		{ RA, RS, RB } },
{ "sub",     XO(31,40,0,0), XO_MASK,	PPC,		{ RT, RB, RA } },
{ "lbzx",    X(31,87),	X_MASK,		COM,		{ RT, RA0, RB } },
{ "neg",     XO(31,104,0,0), XORB_MASK,	COM,		{ RT, RA } },
{ "not",     XRC(31,124,0), X_MASK,	COM,		{ RA, RS, RBS } },
{ "stwx",    X(31,151), X_MASK,		PPCCOM,		{ RS, RA0, RB } },
{ "stbx",    X(31,215),	X_MASK,		COM,		{ RS, RA0, RB } },
{ "mullw",   XO(31,235,0,0), XO_MASK,	PPCCOM,		{ RT, RA, RB } },
{ "add",     XO(31,266,0,0), XO_MASK,	PPCCOM,		{ RT, RA, RB } },
{ "lhzx",    X(31,279),	X_MASK,		COM,		{ RT, RA0, RB } },
{ "xor",     XRC(31,316,0), X_MASK,	COM,		{ RA, RS, RB } },
{ "mflr",    XSPR(31,339,8), XSPR_MASK, COM,		{ RT } },
{ "sthx",    X(31,407),	X_MASK,		COM,		{ RS, RA0, RB } },
{ "mr",	     XRC(31,444,0), X_MASK,	COM,		{ RA, RS, RBS } },
{ "or",      XRC(31,444,0), X_MASK,	COM,		{ RA, RS, RB } },
{ "divwu",   XO(31,459,0,0), XO_MASK,	PPC,		{ RT, RA, RB } },
{ "mtlr",    XSPR(31,467,8), XSPR_MASK, COM,		{ RS } },
{ "mtctr",   XSPR(31,467,9), XSPR_MASK, COM,		{ RS } },
{ "divw",    XO(31,491,0,0), XO_MASK,	PPC,		{ RT, RA, RB } },
{ "lfsx",    X(31,535),	X_MASK,		COM,		{ FRT, RA0, RB } },
{ "srw",     XRC(31,536,0), X_MASK,	PPCCOM,		{ RA, RS, RB } },
{ "stfsx",   X(31,663), X_MASK,		COM,		{ FRS, RA0, RB } },
{ "sraw",    XRC(31,792,0), X_MASK,	PPCCOM,		{ RA, RS, RB } },
{ "extsh",   XRC(31,922,0), XRB_MASK,	PPCCOM,		{ RA, RS } },
{ "extsb",   XRC(31,954,0), XRB_MASK,	PPC,		{ RA, RS} },

{ "lwz",     OP(32),	OP_MASK,	PPCCOM,		{ RT, D, RA0 } },
{ "lbz",     OP(34),	OP_MASK,	COM,		{ RT, D, RA0 } },
{ "stw",     OP(36),	OP_MASK,	PPCCOM,		{ RS, D, RA0 } },
{ "stwu",    OP(37),	OP_MASK,	PPCCOM,		{ RS, D, RAS } },
{ "stb",     OP(38),	OP_MASK,	COM,		{ RS, D, RA0 } },
{ "lhz",     OP(40),	OP_MASK,	COM,		{ RT, D, RA0 } },
{ "sth",     OP(44),	OP_MASK,	COM,		{ RS, D, RA0 } },
{ "lfs",     OP(48),	OP_MASK,	COM,		{ FRT, D, RA0 } },
{ "lfd",     OP(50),	OP_MASK,	COM,		{ FRT, D, RA0 } },
{ "stfs",    OP(52),	OP_MASK,	COM,		{ FRS, D, RA0 } },
{ "stfd",    OP(54),	OP_MASK,	COM,		{ FRS, D, RA0 } },
{ "ld",      DSO(58,0),	DS_MASK,	PPC64,		{ RT, DS, RA0 } },

{ "fdivs",   A(59,18,0), AFRC_MASK,	PPC,		{ FRT, FRA, FRB } },
{ "fsubs",   A(59,20,0), AFRC_MASK,	PPC,		{ FRT, FRA, FRB } },
{ "fadds",   A(59,21,0), AFRC_MASK,	PPC,		{ FRT, FRA, FRB } },
{ "fmuls",   A(59,25,0), AFRB_MASK,	PPC,		{ FRT, FRA, FRC } },
{ "std",     DSO(62,0),	DS_MASK,	PPC64,		{ RS, DS, RA0 } },
{ "stdu",    DSO(62,1),	DS_MASK,	PPC64,		{ RS, DS, RAS } },
{ "frsp",    XRC(63,12,0), XRA_MASK,	COM,		{ FRT, FRB } },
{ "fctiwz",  XRC(63,15,0), XRA_MASK,	PPCCOM,		{ FRT, FRB } },
{ "fsub",    A(63,20,0), AFRC_MASK,	PPCCOM,		{ FRT, FRA, FRB } },
{ "fneg",    XRC(63,40,0), XRA_MASK,	COM,		{ FRT, FRB } },
};

/*
 * VM_TIMES enables showing information about time spent inside
 * and outside generated code
 */
//#define VM_TIMES
#ifdef VM_TIMES
#include <sys/times.h>
static clock_t time_outside_vm = 0;
static clock_t time_total_vm = 0;
#endif

/* exit() won't be called but use it because it is marked with noreturn */
#define DIE( reason ) Com_Error( ERR_DROP, "vm_powerpc compiler error: " reason )

/*
 * vm_powerpc uses large quantities of memory during compilation,
 * Z_Malloc memory may not be enough for some big qvm files
 */

//#define VM_SYSTEM_MALLOC
#ifdef VM_SYSTEM_MALLOC
static inline void *
PPC_Malloc( size_t size )
{
	void *mem = malloc( size );
	if ( ! mem )
		DIE( "Not enough memory" );

	return mem;
}
# define PPC_Free free
#else
# define PPC_Malloc Z_Malloc
# define PPC_Free Z_Free
#endif

/*
 * optimizations:
 * - hole: bubble optimization (OP_CONST+instruction)
 * - copy: inline OP_BLOCK_COPY for lengths under 16/32 bytes
 * - mask: use rlwinm instruction as dataMask
 */

#ifdef __OPTIMIZE__
# define OPTIMIZE_HOLE 1
# define OPTIMIZE_COPY 1
# define OPTIMIZE_MASK 1
#else
# define OPTIMIZE_HOLE 0
# define OPTIMIZE_COPY 0
# define OPTIMIZE_MASK 0
#endif

/*
 * SUPPORTED TARGETS:
 * - Linux 32 bits
 *   ( http://refspecs.freestandards.org/elf/elfspec_ppc.pdf )
 *   * LR at r0 + 4
 *   * Local variable space not needed
 *     -> store caller safe regs at 16+
 *
 * - Linux 64 bits (not fully conformant)
 *   ( http://www.ibm.com/developerworks/linux/library/l-powasm4.html )
 *   * needs "official procedure descriptors" (only first function has one)
 *   * LR at r0 + 16
 *   * local variable space required, min 64 bytes, starts at 48
 *     -> store caller safe regs at 128+
 *
 * - OS X 32 bits
 *   ( http://developer.apple.com/documentation/DeveloperTools/Conceptual/LowLevelABI/Articles/32bitPowerPC.html )
 *   * LR at r0 + 8
 *   * local variable space required, min 32 bytes (?), starts at 24
 *     -> store caller safe regs at 64+
 *
 * - OS X 64 bits (completely untested)
 *   ( http://developer.apple.com/documentation/DeveloperTools/Conceptual/LowLevelABI/Articles/64bitPowerPC.html )
 *   * LR at r0 + 16
 *   * local variable space required, min 64 bytes (?), starts at 48
 *     -> store caller safe regs at 128+
 */

/* Select Length - first value on 32 bits, second on 64 */
#ifdef __PPC64__
#  define SL( a, b ) (b)
#else
#  define SL( a, b ) (a)
#endif

/* Select ABI - first for ELF, second for OS X */
#ifdef __ELF__
#  define SA( a, b ) (a)
#else
#  define SA( a, b ) (b)
#endif

/* Select Endian - first for big endian, second for little endian */
#ifdef __BIG_ENDIAN__
#  define SE( a, b ) (a)
#else
#  define SE( a, b ) (b)
#endif

/* indices for accessing high/low 16-bit halves of a 32-bit int via
 * a union of short[2]; on big-endian [0] is high, on little-endian [1] is */
#define HI16	SE( 0, 1 )
#define LO16	SE( 1, 0 )

/* offset of high/low 32-bit word within a 64-bit double stored in memory */
#define FPRHI	SE( 0, 4 )
#define FPRLO	SE( 4, 0 )

#define ELF32	SL( SA( 1, 0 ), 0 )
#define ELF64	SL( 0, SA( 1, 0 ) )
#define OSX32	SL( SA( 0, 1 ), 0 )
#define OSX64	SL( 0, SA( 0, 1 ) )

/* Distinguish ELFv1 (ppc64be) from ELFv2 (ppc64le) ABI.
 * ELFv1 uses Official Procedure Descriptors (OPD) for function pointers;
 * ELFv2 uses direct code addresses. */
#if ELF64 && defined(_CALL_ELF) && _CALL_ELF == 2
#  define ELFV2	1
#  define ELFV1	0
#else
#  define ELFV2	0
#  define ELFV1	ELF64
#endif

/* native length load/store instructions ( L stands for long ) */
#define iSTLU	SL( iSTWU, iSTDU )
#define iSTL	SL( iSTW, iSTD )
#define iLL	SL( iLWZ, iLD )
#define iLLX	SL( iLWZX, iLDX )

/* register length */
#define GPRLEN	SL( 4, 8 )
#define FPRLEN	(8)
/* shift that many bits to obtain value miltiplied by GPRLEN */
#define GPRLEN_SHIFT	SL( 2, 3 )

/* Link register position */
#define STACK_LR	SL( SA( 4, 8 ), 16 )
/* register save position */
#define STACK_SAVE	SL( SA( 16, 64 ), 128 )
/* temporary space, for float<->int exchange */
#define STACK_TEMP	SL( SA( 8, 24 ), 48 )
/* red zone temporary space, used instead of STACK_TEMP if stack isn't
 * prepared properly */
#define STACK_RTEMP	(-16)

#if ELFV1
/*
 * Official Procedure Descriptor (ELFv1 only)
 *  we need to prepare one for generated code if we want to call it
 * as function.  ELFv2 (ppc64le) uses direct code addresses instead.
 */
typedef struct {
	void *function;
	void *toc;
	void *env;
} opd_t;
#endif


/*
 * opcode information table:
 * - length of immediate value
 * - returned register type
 * - required register(s) type
 */
#define opImm0	0x0000 /* no immediate */
#define opImm1	0x0001 /* 1 byte immadiate value after opcode */
#define opImm4	0x0002 /* 4 bytes immediate value after opcode */

#define opRet0	0x0000 /* returns nothing */
#define opRetI	0x0004 /* returns integer */
#define opRetF	0x0008 /* returns float */
#define opRetIF	(opRetI | opRetF) /* returns integer or float */

#define opArg0	0x0000 /* requires nothing */
#define opArgI	0x0010 /* requires integer(s) */
#define opArgF	0x0020 /* requires float(s) */
#define opArgIF	(opArgI | opArgF) /* requires integer or float */

#define opArg2I	0x0040 /* requires second argument, integer */
#define opArg2F	0x0080 /* requires second argument, float */
#define opArg2IF (opArg2I | opArg2F) /* requires second argument, integer or float */

static const unsigned char vm_opInfo[256] =
{
	[OP_UNDEF]	= opImm0,
	[OP_IGNORE]	= opImm0,
	[OP_BREAK]	= opImm0,
	[OP_ENTER]	= opImm4,
			/* OP_LEAVE has to accept floats, they will be converted to ints */
	[OP_LEAVE]	= opImm4 | opRet0 | opArgIF,
			/* only STORE4 and POP use values from OP_CALL,
			 * no need to convert floats back */
	[OP_CALL]	= opImm0 | opRetI | opArgI,
	[OP_PUSH]	= opImm0 | opRetIF,
	[OP_POP]	= opImm0 | opRet0 | opArgIF,
	[OP_CONST]	= opImm4 | opRetIF,
	[OP_LOCAL]	= opImm4 | opRetI,
	[OP_JUMP]	= opImm0 | opRet0 | opArgI,

	[OP_EQ]		= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_NE]		= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_LTI]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_LEI]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_GTI]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_GEI]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_LTU]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_LEU]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_GTU]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_GEU]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_EQF]	= opImm4 | opRet0 | opArgF | opArg2F,
	[OP_NEF]	= opImm4 | opRet0 | opArgF | opArg2F,
	[OP_LTF]	= opImm4 | opRet0 | opArgF | opArg2F,
	[OP_LEF]	= opImm4 | opRet0 | opArgF | opArg2F,
	[OP_GTF]	= opImm4 | opRet0 | opArgF | opArg2F,
	[OP_GEF]	= opImm4 | opRet0 | opArgF | opArg2F,

	[OP_LOAD1]	= opImm0 | opRetI | opArgI,
	[OP_LOAD2]	= opImm0 | opRetI | opArgI,
	[OP_LOAD4]	= opImm0 | opRetIF| opArgI,
	[OP_STORE1]	= opImm0 | opRet0 | opArgI | opArg2I,
	[OP_STORE2]	= opImm0 | opRet0 | opArgI | opArg2I,
	[OP_STORE4]	= opImm0 | opRet0 | opArgIF| opArg2I,
	[OP_ARG]	= opImm1 | opRet0 | opArgIF,
	[OP_BLOCK_COPY]	= opImm4 | opRet0 | opArgI | opArg2I,

	[OP_SEX8]	= opImm0 | opRetI | opArgI,
	[OP_SEX16]	= opImm0 | opRetI | opArgI,
	[OP_NEGI]	= opImm0 | opRetI | opArgI,
	[OP_ADD]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_SUB]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_DIVI]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_DIVU]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_MODI]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_MODU]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_MULI]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_MULU]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_BAND]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_BOR]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_BXOR]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_BCOM]	= opImm0 | opRetI | opArgI,
	[OP_LSH]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_RSHI]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_RSHU]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_NEGF]	= opImm0 | opRetF | opArgF,
	[OP_ADDF]	= opImm0 | opRetF | opArgF | opArg2F,
	[OP_SUBF]	= opImm0 | opRetF | opArgF | opArg2F,
	[OP_DIVF]	= opImm0 | opRetF | opArgF | opArg2F,
	[OP_MULF]	= opImm0 | opRetF | opArgF | opArg2F,
	[OP_CVIF]	= opImm0 | opRetF | opArgI,
	[OP_CVFI]	= opImm0 | opRetI | opArgF,
};

/*
 * source instruction data
 */
typedef struct source_instruction_s source_instruction_t;
struct source_instruction_s {
	// opcode
	unsigned long int op;

	// number of instruction
	unsigned long int i_count;

	// immediate value (if any)
	union {
		unsigned int i;
		signed int si;
		signed short ss[2];
		unsigned short us[2];
		unsigned char b;
	} arg;

	// required and returned registers
	unsigned char regA1;
	unsigned char regA2;
	unsigned char regR;
	unsigned char regPos;

	// next instruction
	source_instruction_t *next;
};



/*
 * read-only data needed by the generated code
 */
typedef struct VM_Data {
	// length of this struct + data
	size_t dataLength;
	// compiled code size (in bytes)
	// it only is code size, without the data
	size_t codeLength;

	// function pointers, no use to waste registers for them
	long int (* AsmCall)( int, int );
	void (* BlockCopy )( unsigned int, unsigned int, size_t );

	// instruction pointers, rarely used so don't waste register
	ppc_instruction_t *iPointers;

	// data mask for load and store, not used if optimized
	unsigned int dataMask;

	// fixed number used to convert from integer to float
	unsigned int floatBase; // 0x59800004

#if ELFV1
	// official procedure descriptor (ELFv1 only)
	opd_t opd;
#endif

	// additional constants, for floating point OP_CONST
	// this data has dynamic length, thus '0' here
	unsigned int data[0];
} vm_data_t;

#ifdef offsetof
# define VM_Data_Offset( field )	offsetof( vm_data_t, field )
#else
# define OFFSET( structName, field ) \
	( (void *)&(((structName *)NULL)->field) - NULL )
# define VM_Data_Offset( field )	OFFSET( vm_data_t, field )
#endif


/*
 * functions used by generated code
 */
static long int
VM_AsmCall( int callSyscallInvNum, int callProgramStack )
{
	vm_t *savedVM = currentVM;
	long int i, ret;
#ifdef VM_TIMES
	struct tms start_time, stop_time;
	clock_t saved_time = time_outside_vm;
	times( &start_time );
#endif

	// save the stack to allow recursive VM entry
	currentVM->programStack = callProgramStack - 4;

	// we need to convert ints to longs on 64bit powerpcs
	if ( sizeof( intptr_t ) == sizeof( int ) ) {
		intptr_t *argPosition = (intptr_t *)((byte *)currentVM->dataBase + callProgramStack + 4);

		// generated code does not invert syscall number
		argPosition[ 0 ] = -1 - callSyscallInvNum;

		ret = currentVM->systemCall( argPosition );
	} else {
		intptr_t args[MAX_VMSYSCALL_ARGS];

		// generated code does not invert syscall number
		args[0] = -1 - callSyscallInvNum;

		int *argPosition = (int *)((byte *)currentVM->dataBase + callProgramStack + 4);
		for( i = 1; i < ARRAY_LEN(args); i++ )
			args[ i ] = argPosition[ i ];

		ret = currentVM->systemCall( args );
	}

	currentVM = savedVM;

#ifdef VM_TIMES
	times( &stop_time );
	time_outside_vm = saved_time + ( stop_time.tms_utime - start_time.tms_utime );
#endif

	return ret;
}

/*
 * code-block descriptors
 */
typedef struct dest_instruction dest_instruction_t;
typedef struct symbolic_jump symbolic_jump_t;

struct symbolic_jump {
	// number of source instruction it has to jump to
	unsigned long int jump_to;

	// jump condition true/false, (4*cr7+(eq|gt..))
	long int bo, bi;

	// extensions / modifiers (branch-link)
	unsigned long ext;

	// dest_instruction referring to this jump
	dest_instruction_t *parent;

	// next jump
	symbolic_jump_t *nextJump;
};

struct dest_instruction {
	// position in the output chain
	unsigned long int count;

	// source instruction number
	unsigned long int i_count;

	// exact (for instructins), or maximum (for jump) length
	unsigned short length;

	dest_instruction_t *next;

	// if the instruction is a jump than jump will be non NULL
	symbolic_jump_t *jump;

	// if jump is NULL than all the instructions will be here
	ppc_instruction_t code[0];
};

// first and last instruction,
// di_first is a dummy instruction
static dest_instruction_t *di_first = NULL, *di_last = NULL;
// number of instructions
static unsigned long int di_count = 0;
// pointers needed to compute local jumps, those aren't pointers to
// actual instructions, just used to check how long the jump is going
// to be and whether it is positive or negative
static dest_instruction_t **di_pointers = NULL;

// output instructions which does not come from source code
// use false i_count value
#define FALSE_ICOUNT 0xffffffff


/*
 * append specified instructions at the end of instruction chain
 */
static void
PPC_Append(
		dest_instruction_t *di_now,
		unsigned long int i_count
  	  )
{
	di_now->count = di_count++;
	di_now->i_count = i_count;
	di_now->next = NULL;

	di_last->next = di_now;
	di_last = di_now;

	if ( i_count != FALSE_ICOUNT ) {
		if ( ! di_pointers[ i_count ] )
			di_pointers[ i_count ] = di_now;
	}
}

/*
 * make space for instructions and append
 */
static void
PPC_AppendInstructions(
		unsigned long int i_count,
		size_t num_instructions,
		const ppc_instruction_t *is
	)
{
	if ( num_instructions < 0 )
		num_instructions = 0;
	size_t iBytes = sizeof( ppc_instruction_t ) * num_instructions;
	dest_instruction_t *di_now = PPC_Malloc( sizeof( dest_instruction_t ) + iBytes );

	di_now->length = num_instructions;
	di_now->jump = NULL;

	if ( iBytes > 0 )
		memcpy( &(di_now->code[0]), is, iBytes );

	PPC_Append( di_now, i_count );
}

/*
 * create symbolic jump and append
 */
static symbolic_jump_t *sj_first = NULL, *sj_last = NULL;
static void
PPC_PrepareJump(
		unsigned long int i_count,
		unsigned long int dest,
		long int bo,
		long int bi,
		unsigned long int ext
	)
{
	dest_instruction_t *di_now = PPC_Malloc( sizeof( dest_instruction_t ) );
	symbolic_jump_t *sj = PPC_Malloc( sizeof( symbolic_jump_t ) );

	sj->jump_to = dest;
	sj->bo = bo;
	sj->bi = bi;
	sj->ext = ext;
	sj->parent = di_now;
	sj->nextJump = NULL;

	sj_last->nextJump = sj;
	sj_last = sj;

	di_now->length = (bo == branchAlways ? 1 : 2);
	di_now->jump = sj;

	PPC_Append( di_now, i_count );
}

/*
 * simplyfy instruction emission
 */
#define emitStart( i_cnt ) \
	unsigned long int i_count = i_cnt; \
	size_t num_instructions = 0; \
	long int force_emit = 0; \
	ppc_instruction_t instructions[50];

#define pushIn( inst ) \
	(instructions[ num_instructions++ ] = inst)
#define in( inst, args... ) pushIn( IN( inst, args ) )

#define emitEnd() \
	do{ \
		if ( num_instructions || force_emit ) \
			PPC_AppendInstructions( i_count, num_instructions, instructions );\
		num_instructions = 0; \
	} while(0)

#define emitJump( dest, bo, bi, ext ) \
	do { \
		emitEnd(); \
		PPC_PrepareJump( i_count, dest, bo, bi, ext ); \
	} while(0)


/*
 * definitions for creating .data section,
 * used in cases where constant float is needed
 */
#define LOCAL_DATA_CHUNK 50
typedef struct local_data_s local_data_t;
struct local_data_s {
	// number of data in this structure
	long int count;

	// data placeholder
	unsigned int data[ LOCAL_DATA_CHUNK ];

	// next chunk, if this one wasn't enough
	local_data_t *next;
};

// first data chunk
static local_data_t *data_first = NULL;
// total number of data
static long int data_acc = 0;

/*
 * append the data and return its offset
 */
static size_t
PPC_PushData( unsigned int datum )
{
	local_data_t *d_now = data_first;
	long int accumulated = 0;

	// check whether we have this one already
	do {
		long int i;
		for ( i = 0; i < d_now->count; i++ ) {
			if ( d_now->data[ i ] == datum ) {
				accumulated += i;
				return VM_Data_Offset( data[ accumulated ] );
			}
		}
		if ( !d_now->next )
			break;

		accumulated += d_now->count;
		d_now = d_now->next;
	} while (1);

	// not found, need to append
	accumulated += d_now->count;

	// last chunk is full, create new one
	if ( d_now->count >= LOCAL_DATA_CHUNK ) {
		d_now->next = PPC_Malloc( sizeof( local_data_t ) );
		d_now = d_now->next;
		d_now->count = 0;
		d_now->next = NULL;
	}

	d_now->data[ d_now->count ] = datum;
	d_now->count += 1;

	data_acc = accumulated + 1;

	return VM_Data_Offset( data[ accumulated ] );
}

/*
 * find leading zeros in dataMask to implement it with
 * "rotate and mask" instruction
 */
static long int fastMaskHi = 0, fastMaskLo = 31;
static void
PPC_MakeFastMask( int mask )
{
#if defined( __GNUC__ ) && ( __GNUC__ >= 4 || ( __GNUC__ == 3 && __GNUC_MINOR__ >= 4 ) )
	/* count leading zeros */
	fastMaskHi = __builtin_clz( mask );

	/* count trailing zeros */
	fastMaskLo = 31 - __builtin_ctz( mask );
#else
	fastMaskHi = 0;
	while ( ( mask & ( 0x80000000 >> fastMaskHi ) ) == 0 )
		fastMaskHi++;

	fastMaskLo = 31;
	while ( ( mask & ( 0x80000000 >> fastMaskLo ) ) == 0 )
		fastMaskLo--;
#endif
}


/*
 * register definitions
 */

/* registers which are global for generated code */

// pointer to VM_Data (constant)
#define rVMDATA r14
// vm->dataBase (constant)
#define rDATABASE r15
// programStack (variable)
#define rPSTACK r16

/*
 * function local registers,
 *
 * normally only volatile registers are used, but if there aren't enough
 * or function has to preserve some value while calling another one
 * then caller safe registers are used as well
 */
static const long int gpr_list[] = {
	/* caller safe registers, normally only one is used */
	r24, r23, r22, r21,
	r20, r19, r18, r17,
	/* volatile registers (preferred),
	 * normally no more than 5 is used */
	r3, r4, r5, r6,
	r7, r8, r9, r10,
};
static const long int gpr_vstart = 8; /* position of first volatile register */
static const long int gpr_total = ARRAY_LEN( gpr_list );

static const long int fpr_list[] = {
	/* static registers, normally none is used */
	f20, f21, f19, f18,
	f17, f16, f15, f14,
	/* volatile registers (preferred),
	 * normally no more than 7 is used */
	f0, f1, f2, f3,
	f4, f5, f6, f7,
	f8, f9, f10, f11,
	f12, f13,
};
static const long int fpr_vstart = 8;
static const long int fpr_total = ARRAY_LEN( fpr_list );

/*
 * prepare some dummy structures and emit init code
 */
static void
PPC_CompileInit( void )
{
	di_first = di_last = PPC_Malloc( sizeof( dest_instruction_t ) );
	di_first->count = 0;
	di_first->next = NULL;
	di_first->jump = NULL;

	sj_first = sj_last = PPC_Malloc( sizeof( symbolic_jump_t ) );
	sj_first->nextJump = NULL;

	data_first = PPC_Malloc( sizeof( local_data_t ) );
	data_first->count = 0;
	data_first->next = NULL;

	/*
	 * init function:
	 * saves old values of global registers and sets our values
	 * function prototype is:
	 *  int begin( void *data, int programStack, void *vm->dataBase )
	 */

	/* first instruction must not be placed on instruction list */
	emitStart( FALSE_ICOUNT );

	long int stack = STACK_SAVE + 4 * GPRLEN;

	in( iMFLR, r0 );
	in( iSTLU, r1, -stack, r1 );
	in( iSTL, rVMDATA, STACK_SAVE + 0 * GPRLEN, r1 );
	in( iSTL, rPSTACK, STACK_SAVE + 1 * GPRLEN, r1 );
	in( iSTL, rDATABASE, STACK_SAVE + 2 * GPRLEN, r1 );
	in( iSTL, r0, stack + STACK_LR, r1 );
	in( iMR, rVMDATA, r3 );
	in( iMR, rPSTACK, r4 );
	in( iMR, rDATABASE, r5 );
	in( iBL, +4*8 ); // LINK JUMP: first generated instruction | XXX jump !
	in( iLL, rVMDATA, STACK_SAVE + 0 * GPRLEN, r1 );
	in( iLL, rPSTACK, STACK_SAVE + 1 * GPRLEN, r1 );
	in( iLL, rDATABASE, STACK_SAVE + 2 * GPRLEN, r1 );
	in( iLL, r0, stack + STACK_LR, r1 );
	in( iMTLR, r0 );
	in( iADDI, r1, r1, stack );
	in( iBLR );

	emitEnd();
}

// rFIRST is the copy of the top value on the opstack
#define rFIRST		(gpr_list[ gpr_pos - 1])
// second value on the opstack
#define rSECOND		(gpr_list[ gpr_pos - 2 ])
// temporary registers, not on the opstack
#define rTEMP(x)	(gpr_list[ gpr_pos + x ])
#define rTMP		rTEMP(0)

#define fFIRST		(fpr_list[ fpr_pos - 1 ])
#define fSECOND		(fpr_list[ fpr_pos - 2 ])
#define fTEMP(x)	(fpr_list[ fpr_pos + x ])
#define fTMP		fTEMP(0)

// register types
#define rTYPE_STATIC	0x01
#define rTYPE_FLOAT	0x02

// what type should this opcode return
#define RET_INT		( !(i_now->regR & rTYPE_FLOAT) )
#define RET_FLOAT	( i_now->regR & rTYPE_FLOAT )
// what type should it accept
#define ARG_INT		( ! i_now->regA1 )
#define ARG_FLOAT	( i_now->regA1 )
#define ARG2_INT	( ! i_now->regA2 )
#define ARG2_FLOAT	( i_now->regA2 )

/*
 * emit OP_CONST, called if nothing has used the const value directly
 */
static void
PPC_EmitConst( source_instruction_t * const i_const )
{
	emitStart( i_const->i_count );

	if ( !(i_const->regR & rTYPE_FLOAT) ) {
		// gpr_pos needed for "rFIRST" to work
		long int gpr_pos = i_const->regPos;

		if ( i_const->arg.si >= -0x8000 && i_const->arg.si < 0x8000 ) {
			in( iLI, rFIRST, i_const->arg.si );
		} else if ( i_const->arg.i < 0x10000 ) {
			in( iLI, rFIRST, 0 );
			in( iORI, rFIRST, rFIRST, i_const->arg.i );
		} else {
			in( iLIS, rFIRST, i_const->arg.ss[ HI16 ] );
			if ( i_const->arg.us[ LO16 ] != 0 )
				in( iORI, rFIRST, rFIRST, i_const->arg.us[ LO16 ] );
		}

	} else {
		// fpr_pos needed for "fFIRST" to work
		long int fpr_pos = i_const->regPos;

		// there's no good way to generate the data,
		// just read it from data section
		in( iLFS, fFIRST, PPC_PushData( i_const->arg.i ), rVMDATA );
	}

	emitEnd();
}
#define MAYBE_EMIT_CONST() if ( i_const ) PPC_EmitConst( i_const )

/*
 * emit empty instruction, just sets the needed pointers
 */
static inline void
PPC_EmitNull( source_instruction_t * const i_null )
{
	PPC_AppendInstructions( i_null->i_count, 0, NULL );
}
#define EMIT_FALSE_CONST() PPC_EmitNull( i_const )


/*
 * analize function for register usage and whether it needs stack (r1) prepared
 */
static void
VM_AnalyzeFunction(
		source_instruction_t * const i_first,
		long int *prepareStack,
		long int *gpr_start_pos,
		long int *fpr_start_pos
		)
{
	source_instruction_t *i_now = i_first;

	source_instruction_t *value_provider[20] = { NULL };
	unsigned long int opstack_depth = 0;

	/*
	 * first step:
	 *  remember what codes returned some value and mark the value type
	 *  when we get to know what it should be
	 */
	while ( (i_now = i_now->next) ) {
		unsigned long int op = i_now->op;
		unsigned long int opi = vm_opInfo[ op ];

		if ( opi & opArgIF ) {
			assert( opstack_depth > 0 );

			opstack_depth--;
			source_instruction_t *vp = value_provider[ opstack_depth ];
			unsigned long int vpopi = vm_opInfo[ vp->op ];

			if ( (opi & opArgI) && (vpopi & opRetI) ) {
				// instruction accepts integer, provider returns integer
				//vp->regR |= rTYPE_INT;
				//i_now->regA1 = rTYPE_INT;
			} else if ( (opi & opArgF) && (vpopi & opRetF) ) {
				// instruction accepts float, provider returns float
				vp->regR |= rTYPE_FLOAT; // use OR here - could be marked as static
				i_now->regA1 = rTYPE_FLOAT;
			} else {
				// instruction arg type does not agree with
				// provider return type
				DIE( "unrecognized instruction combination" );
			}

		}
		if ( opi & opArg2IF ) {
			assert( opstack_depth > 0 );

			opstack_depth--;
			source_instruction_t *vp = value_provider[ opstack_depth ];
			unsigned long int vpopi = vm_opInfo[ vp->op ];

			if ( (opi & opArg2I) && (vpopi & opRetI) ) {
				// instruction accepts integer, provider returns integer
				//vp->regR |= rTYPE_INT;
				//i_now->regA2 = rTYPE_INT;
			} else if ( (opi & opArg2F) && (vpopi & opRetF) ) {
				// instruction accepts float, provider returns float
				vp->regR |= rTYPE_FLOAT; // use OR here - could be marked as static
				i_now->regA2 = rTYPE_FLOAT;
			} else {
				// instruction arg type does not agree with
				// provider return type
				DIE( "unrecognized instruction combination" );
			}
		}


		if (
			( op == OP_CALL )
				||
			( op == OP_BLOCK_COPY && ( i_now->arg.i > SL( 16, 32 ) || !OPTIMIZE_COPY ) )
		) {
			long int i;
			*prepareStack = 1;
			// force caller safe registers so we won't have to save them
			for ( i = 0; i < opstack_depth; i++ ) {
				source_instruction_t *vp = value_provider[ i ];
				vp->regR |= rTYPE_STATIC;
			}
		}


		if ( opi & opRetIF ) {
			value_provider[ opstack_depth ] = i_now;
			opstack_depth++;
		}
	}

	/*
	 * second step:
	 *  now that we know register types; compute exactly how many registers
	 *  of each type we need
	 */

	i_now = i_first;
	long int needed_reg[4] = {0,0,0,0}, max_reg[4] = {0,0,0,0};
	opstack_depth = 0;
	while ( (i_now = i_now->next) ) {
		unsigned long int op = i_now->op;
		unsigned long int opi = vm_opInfo[ op ];

		if ( opi & opArgIF ) {
			assert( opstack_depth > 0 );
			opstack_depth--;
			source_instruction_t *vp = value_provider[ opstack_depth ];

			needed_reg[ ( vp->regR & 2 ) ] -= 1;
			if ( vp->regR & 1 ) // static
				needed_reg[ ( vp->regR & 3 ) ] -= 1;
		}
		if ( opi & opArg2IF ) {
			assert( opstack_depth > 0 );
			opstack_depth--;
			source_instruction_t *vp = value_provider[ opstack_depth ];

			needed_reg[ ( vp->regR & 2 ) ] -= 1;
			if ( vp->regR & 1 ) // static
				needed_reg[ ( vp->regR & 3 ) ] -= 1;
		}

		if ( opi & opRetIF ) {
			long int i;
			value_provider[ opstack_depth ] = i_now;
			opstack_depth++;

			i = i_now->regR & 2;
			needed_reg[ i ] += 1;
			if ( max_reg[ i ] < needed_reg[ i ] )
				max_reg[ i ] = needed_reg[ i ];

			i = i_now->regR & 3;
			if ( i & 1 ) {
				needed_reg[ i ] += 1;
				if ( max_reg[ i ] < needed_reg[ i ] )
					max_reg[ i ] = needed_reg[ i ];
			}
		}
	}

	long int gpr_start = gpr_vstart;
	const long int gpr_volatile = gpr_total - gpr_vstart;
	if ( max_reg[ 1 ] > 0 || max_reg[ 0 ] > gpr_volatile ) {
		// max_reg[ 0 ] - all gprs needed
		// max_reg[ 1 ] - static gprs needed
		long int max = max_reg[ 0 ] - gpr_volatile;
		if ( max_reg[ 1 ] > max )
			max = max_reg[ 1 ];
		if ( max > gpr_vstart ) {
			/* error */
			DIE( "Need more GPRs" );
		}

		gpr_start -= max;

		// need stack to save caller safe registers
		*prepareStack = 1;
	}
	*gpr_start_pos = gpr_start;

	long int fpr_start = fpr_vstart;
	const long int fpr_volatile = fpr_total - fpr_vstart;
	if ( max_reg[ 3 ] > 0 || max_reg[ 2 ] > fpr_volatile ) {
		// max_reg[ 2 ] - all fprs needed
		// max_reg[ 3 ] - static fprs needed
		long int max = max_reg[ 2 ] - fpr_volatile;
		if ( max_reg[ 3 ] > max )
			max = max_reg[ 3 ];
		if ( max > fpr_vstart ) {
			/* error */
			DIE( "Need more FPRs" );
		}

		fpr_start -= max;

		// need stack to save caller safe registers
		*prepareStack = 1;
	}
	*fpr_start_pos = fpr_start;
}

/*
 * translate opcodes to ppc instructions,
 * it works on functions, not on whole code at once
 */
static void
VM_CompileFunction( source_instruction_t * const i_first )
{
	long int prepareStack = 0;
	long int gpr_start_pos, fpr_start_pos;

	VM_AnalyzeFunction( i_first, &prepareStack, &gpr_start_pos, &fpr_start_pos );

	long int gpr_pos = gpr_start_pos, fpr_pos = fpr_start_pos;

	// OP_CONST combines well with many opcodes so we treat it in a special way
	source_instruction_t *i_const = NULL;
	source_instruction_t *i_now = i_first;

	// how big the stack has to be
	long int save_space = STACK_SAVE;
	{
		if ( gpr_start_pos < gpr_vstart )
			save_space += (gpr_vstart - gpr_start_pos) * GPRLEN;
		save_space = ( save_space + 15 ) & ~0x0f;

		if ( fpr_start_pos < fpr_vstart )
			save_space += (fpr_vstart - fpr_start_pos) * FPRLEN;
		save_space = ( save_space + 15 ) & ~0x0f;
	}

	long int stack_temp = prepareStack ? STACK_TEMP : STACK_RTEMP;

	while ( (i_now = i_now->next) ) {
		emitStart( i_now->i_count );

		switch ( i_now->op )
		{
			default:
			case OP_UNDEF:
			case OP_IGNORE:
				MAYBE_EMIT_CONST();
				in( iNOP );
				break;

			case OP_BREAK:
				MAYBE_EMIT_CONST();
				// force SEGV
				in( iLWZ, r0, 0, r0 );
				break;

			case OP_ENTER:
				if ( i_const )
					DIE( "Weird opcode order" );

				// don't prepare stack if not needed
				if ( prepareStack ) {
					long int i, save_pos = STACK_SAVE;

					in( iMFLR, r0 );
					in( iSTLU, r1, -save_space, r1 );
					in( iSTL, r0, save_space + STACK_LR, r1 );

					/* save registers */
					for ( i = gpr_start_pos; i < gpr_vstart; i++ ) {
						in( iSTL, gpr_list[ i ], save_pos, r1 );
						save_pos += GPRLEN;
					}
					save_pos = ( save_pos + 15 ) & ~0x0f;

					for ( i = fpr_start_pos; i < fpr_vstart; i++ ) {
						in( iSTFD, fpr_list[ i ], save_pos, r1 );
						save_pos += FPRLEN;
					}
					prepareStack = 2;
				}

				in( iADDI, rPSTACK, rPSTACK, - i_now->arg.si );
				break;

			case OP_LEAVE:
				if ( i_const ) {
					EMIT_FALSE_CONST();

					if ( i_const->regR & rTYPE_FLOAT)
						DIE( "constant float in OP_LEAVE" );

					if ( i_const->arg.si >= -0x8000 && i_const->arg.si < 0x8000 ) {
						in( iLI, r3, i_const->arg.si );
					} else if ( i_const->arg.i < 0x10000 ) {
						in( iLI, r3, 0 );
						in( iORI, r3, r3, i_const->arg.i );
					} else {
						in( iLIS, r3, i_const->arg.ss[ HI16 ] );
						if ( i_const->arg.us[ LO16 ] != 0 )
							in( iORI, r3, r3, i_const->arg.us[ LO16 ] );
					}
					gpr_pos--;
				} else {
					MAYBE_EMIT_CONST();

					/* place return value in r3 */
					if ( ARG_INT ) {
						if ( rFIRST != r3 )
							in( iMR, r3, rFIRST );
						gpr_pos--;
					} else {
						in( iSTFS, fFIRST, stack_temp, r1 );
						in( iLWZ, r3, stack_temp, r1 );
						fpr_pos--;
					}
				}

				// don't undo stack if not prepared
				if ( prepareStack >= 2 ) {
					long int i, save_pos = STACK_SAVE;

					in( iLL, r0, save_space + STACK_LR, r1 );


					/* restore registers */
					for ( i = gpr_start_pos; i < gpr_vstart; i++ ) {
						in( iLL, gpr_list[ i ], save_pos, r1 );
						save_pos += GPRLEN;
					}
					save_pos = ( save_pos + 15 ) & ~0x0f;
					for ( i = fpr_start_pos; i < fpr_vstart; i++ ) {
						in( iLFD, fpr_list[ i ], save_pos, r1 );
						save_pos += FPRLEN;
					}

					in( iMTLR, r0 );
					in( iADDI, r1, r1, save_space );
				}
				in( iADDI, rPSTACK, rPSTACK, i_now->arg.si);
				in( iBLR );
				assert( gpr_pos == gpr_start_pos );
				assert( fpr_pos == fpr_start_pos );
				break;

			case OP_CALL:
				if ( i_const ) {
					EMIT_FALSE_CONST();

					if ( i_const->arg.si >= 0 ) {
						emitJump(
							i_const->arg.i,
							branchAlways, 0, branchExtLink
						);
					} else {
						/* syscall */
						in( iLL, r0, VM_Data_Offset( AsmCall ), rVMDATA );

						in( iLI, r3, i_const->arg.si ); // negative value
						in( iMR, r4, rPSTACK ); // push PSTACK on argument list

#if ELFV2
						in( iMR, r12, r0 ); // ELFv2: r12 must hold target addr
#endif
						in( iMTCTR, r0 );
						in( iBCTRL );
					}
					if ( rFIRST != r3 )
						in( iMR, rFIRST, r3 );
				} else {
					MAYBE_EMIT_CONST();

					in( iCMPWI, cr7, rFIRST, 0 );
					in( iBLTm, cr7, +4*5 /* syscall */ ); // XXX jump !
					/* instruction call */

					// get instruction address
					in( iLL, r0, VM_Data_Offset( iPointers ), rVMDATA );
					in( iRLWINM, rFIRST, rFIRST, GPRLEN_SHIFT, 0, 31-GPRLEN_SHIFT ); // mul * GPRLEN
					in( iLLX, r0, rFIRST, r0 ); // load pointer

					in( iB, +4*(3 + (rFIRST != r3 ? 1 : 0) ) ); // skip syscall-specific code to common tail

					/* syscall */
					in( iLL, r0, VM_Data_Offset( AsmCall ), rVMDATA ); // get asmCall pointer
					/* rFIRST can be r3 or some static register */
					if ( rFIRST != r3 )
						in( iMR, r3, rFIRST ); // push OPSTACK top value on argument list
					in( iMR, r4, rPSTACK ); // push PSTACK on argument list

					/* common code */
#if ELFV2
					in( iMR, r12, r0 ); // ELFv2: r12 must hold target addr
#endif
					in( iMTCTR, r0 );
					in( iBCTRL );

					if ( rFIRST != r3 )
						in( iMR, rFIRST, r3 ); // push return value on the top of the opstack
				}
				break;

			case OP_PUSH:
				MAYBE_EMIT_CONST();
				if ( RET_INT )
					gpr_pos++;
				else
					fpr_pos++;
				/* no instructions here */
				force_emit = 1;
				break;

			case OP_POP:
				MAYBE_EMIT_CONST();
				if ( ARG_INT )
					gpr_pos--;
				else
					fpr_pos--;
				/* no instructions here */
				force_emit = 1;
				break;

			case OP_CONST:
				MAYBE_EMIT_CONST();
				/* nothing here */
				break;

			case OP_LOCAL:
				MAYBE_EMIT_CONST();
				{
					signed long int hi, lo;
					hi = i_now->arg.ss[ HI16 ];
					lo = i_now->arg.ss[ LO16 ];
					if ( lo < 0 )
						hi += 1;

					gpr_pos++;
					if ( hi == 0 ) {
						in( iADDI, rFIRST, rPSTACK, lo );
					} else {
						in( iADDIS, rFIRST, rPSTACK, hi );
						if ( lo != 0 )
							in( iADDI, rFIRST, rFIRST, lo );
					}
				}
				break;

			case OP_JUMP:
				if ( i_const ) {
					EMIT_FALSE_CONST();

					emitJump(
						i_const->arg.i,
						branchAlways, 0, 0
					);
				} else {
					MAYBE_EMIT_CONST();

					in( iLL, r0, VM_Data_Offset( iPointers ), rVMDATA );
					in( iRLWINM, rFIRST, rFIRST, GPRLEN_SHIFT, 0, 31-GPRLEN_SHIFT ); // mul * GPRLEN
					in( iLLX, r0, rFIRST, r0 ); // load pointer
					in( iMTCTR, r0 );
					in( iBCTR );
				}
				gpr_pos--;
				break;

			case OP_EQ:
			case OP_NE:
				if ( i_const && i_const->arg.si >= -0x8000 && i_const->arg.si < 0x10000 ) {
					EMIT_FALSE_CONST();
					if ( i_const->arg.si >= 0x8000 )
						in( iCMPLWI, cr7, rSECOND, i_const->arg.i );
					else
						in( iCMPWI, cr7, rSECOND, i_const->arg.si );
				} else {
					MAYBE_EMIT_CONST();
					in( iCMPW, cr7, rSECOND, rFIRST );
				}
				emitJump(
					i_now->arg.i,
					(i_now->op == OP_EQ ? branchTrue : branchFalse),
					4*cr7+eq, 0
				);
				gpr_pos -= 2;
				break;

			case OP_LTI:
			case OP_GEI:
				if ( i_const && i_const->arg.si >= -0x8000 && i_const->arg.si < 0x8000 ) {
					EMIT_FALSE_CONST();
					in( iCMPWI, cr7, rSECOND, i_const->arg.si );
				} else {
					MAYBE_EMIT_CONST();
					in( iCMPW, cr7, rSECOND, rFIRST );
				}
				emitJump(
					i_now->arg.i,
					( i_now->op == OP_LTI ? branchTrue : branchFalse ),
					4*cr7+lt, 0
				);
				gpr_pos -= 2;
				break;

			case OP_GTI:
			case OP_LEI:
				if ( i_const && i_const->arg.si >= -0x8000 && i_const->arg.si < 0x8000 ) {
					EMIT_FALSE_CONST();
					in( iCMPWI, cr7, rSECOND, i_const->arg.si );
				} else {
					MAYBE_EMIT_CONST();
					in( iCMPW, cr7, rSECOND, rFIRST );
				}
				emitJump(
					i_now->arg.i,
					( i_now->op == OP_GTI ? branchTrue : branchFalse ),
					4*cr7+gt, 0
				);
				gpr_pos -= 2;
				break;

			case OP_LTU:
			case OP_GEU:
				if ( i_const && i_const->arg.i < 0x10000 ) {
					EMIT_FALSE_CONST();
					in( iCMPLWI, cr7, rSECOND, i_const->arg.i );
				} else {
					MAYBE_EMIT_CONST();
					in( iCMPLW, cr7, rSECOND, rFIRST );
				}
				emitJump(
					i_now->arg.i,
					( i_now->op == OP_LTU ? branchTrue : branchFalse ),
					4*cr7+lt, 0
				);
				gpr_pos -= 2;
				break;

			case OP_GTU:
			case OP_LEU:
				if ( i_const && i_const->arg.i < 0x10000 ) {
					EMIT_FALSE_CONST();
					in( iCMPLWI, cr7, rSECOND, i_const->arg.i );
				} else {
					MAYBE_EMIT_CONST();
					in( iCMPLW, cr7, rSECOND, rFIRST );
				}
				emitJump(
					i_now->arg.i,
					( i_now->op == OP_GTU ? branchTrue : branchFalse ),
					4*cr7+gt, 0
				);
				gpr_pos -= 2;
				break;

			case OP_EQF:
			case OP_NEF:
				MAYBE_EMIT_CONST();
				in( iFCMPU, cr7, fSECOND, fFIRST );
				emitJump(
					i_now->arg.i,
					( i_now->op == OP_EQF ? branchTrue : branchFalse ),
					4*cr7+eq, 0
				);
				fpr_pos -= 2;
				break;

			case OP_LTF:
			case OP_GEF:
				MAYBE_EMIT_CONST();
				in( iFCMPU, cr7, fSECOND, fFIRST );
				emitJump(
					i_now->arg.i,
					( i_now->op == OP_LTF ? branchTrue : branchFalse ),
					4*cr7+lt, 0
				);
				fpr_pos -= 2;
				break;

			case OP_GTF:
			case OP_LEF:
				MAYBE_EMIT_CONST();
				in( iFCMPU, cr7, fSECOND, fFIRST );
				emitJump(
					i_now->arg.i,
					( i_now->op == OP_GTF ? branchTrue : branchFalse ),
					4*cr7+gt, 0
				);
				fpr_pos -= 2;
				break;

			case OP_LOAD1:
				MAYBE_EMIT_CONST();
#if OPTIMIZE_MASK
				in( iRLWINM, rFIRST, rFIRST, 0, fastMaskHi, fastMaskLo );
#else
				in( iLWZ, r0, VM_Data_Offset( dataMask ), rVMDATA );
				in( iAND, rFIRST, rFIRST, r0 );
#endif
				in( iLBZX, rFIRST, rFIRST, rDATABASE );
				break;

			case OP_LOAD2:
				MAYBE_EMIT_CONST();
#if OPTIMIZE_MASK
				in( iRLWINM, rFIRST, rFIRST, 0, fastMaskHi, fastMaskLo );
#else
				in( iLWZ, r0, VM_Data_Offset( dataMask ), rVMDATA );
				in( iAND, rFIRST, rFIRST, r0 );
#endif
				in( iLHZX, rFIRST, rFIRST, rDATABASE );
				break;

			case OP_LOAD4:
				MAYBE_EMIT_CONST();
#if OPTIMIZE_MASK
				in( iRLWINM, rFIRST, rFIRST, 0, fastMaskHi, fastMaskLo );
#else
				in( iLWZ, r0, VM_Data_Offset( dataMask ), rVMDATA );
				in( iAND, rFIRST, rFIRST, r0 );
#endif
				if ( RET_INT ) {
					in( iLWZX, rFIRST, rFIRST, rDATABASE );
				} else {
					fpr_pos++;
					in( iLFSX, fFIRST, rFIRST, rDATABASE );
					gpr_pos--;
				}
				break;

			case OP_STORE1:
				MAYBE_EMIT_CONST();
#if OPTIMIZE_MASK
				in( iRLWINM, rSECOND, rSECOND, 0, fastMaskHi, fastMaskLo );
#else
				in( iLWZ, r0, VM_Data_Offset( dataMask ), rVMDATA );
				in( iAND, rSECOND, rSECOND, r0 );
#endif
				in( iSTBX, rFIRST, rSECOND, rDATABASE );
				gpr_pos -= 2;
				break;

			case OP_STORE2:
				MAYBE_EMIT_CONST();
#if OPTIMIZE_MASK
				in( iRLWINM, rSECOND, rSECOND, 0, fastMaskHi, fastMaskLo );
#else
				in( iLWZ, r0, VM_Data_Offset( dataMask ), rVMDATA );
				in( iAND, rSECOND, rSECOND, r0 );
#endif
				in( iSTHX, rFIRST, rSECOND, rDATABASE );
				gpr_pos -= 2;
				break;

			case OP_STORE4:
				MAYBE_EMIT_CONST();
				if ( ARG_INT ) {
#if OPTIMIZE_MASK
					in( iRLWINM, rSECOND, rSECOND, 0, fastMaskHi, fastMaskLo );
#else
					in( iLWZ, r0, VM_Data_Offset( dataMask ), rVMDATA );
					in( iAND, rSECOND, rSECOND, r0 );
#endif

					in( iSTWX, rFIRST, rSECOND, rDATABASE );
					gpr_pos--;
				} else {
#if OPTIMIZE_MASK
					in( iRLWINM, rFIRST, rFIRST, 0, fastMaskHi, fastMaskLo );
#else
					in( iLWZ, r0, VM_Data_Offset( dataMask ), rVMDATA );
					in( iAND, rFIRST, rFIRST, r0 );
#endif

					in( iSTFSX, fFIRST, rFIRST, rDATABASE );
					fpr_pos--;
				}
				gpr_pos--;
				break;

			case OP_ARG:
				MAYBE_EMIT_CONST();
				in( iADDI, r0, rPSTACK, i_now->arg.b );
				if ( ARG_INT ) {
					in( iSTWX, rFIRST, rDATABASE, r0 );
					gpr_pos--;
				} else {
					in( iSTFSX, fFIRST, rDATABASE, r0 );
					fpr_pos--;
				}
				break;

			case OP_BLOCK_COPY:
				MAYBE_EMIT_CONST();
#if OPTIMIZE_COPY
				if ( i_now->arg.i <= SL( 16, 32 ) ) {
					/* block is very short so copy it in-place */

					unsigned int len = i_now->arg.i;
					unsigned int copied = 0, left = len;

					in( iADD, rFIRST, rFIRST, rDATABASE );
					in( iADD, rSECOND, rSECOND, rDATABASE );

					if ( len >= GPRLEN ) {
						long int i, words = len / GPRLEN;
						in( iLL, r0, 0, rFIRST );
						for ( i = 1; i < words; i++ )
							in( iLL, rTEMP( i - 1 ), GPRLEN * i, rFIRST );

						in( iSTL, r0, 0, rSECOND );
						for ( i = 1; i < words; i++ )
							in( iSTL, rTEMP( i - 1 ), GPRLEN * i, rSECOND );

						copied += words * GPRLEN;
						left -= copied;
					}

					if ( SL( 0, left >= 4 ) ) {
						in( iLWZ, r0, copied+0, rFIRST );
						in( iSTW, r0, copied+0, rSECOND );
						copied += 4;
						left -= 4;
					}
					if ( left >= 4 ) {
						DIE("Bug in OP_BLOCK_COPY");
					}
					if ( left == 3 ) {
						in( iLHZ, r0,	copied+0, rFIRST );
						in( iLBZ, rTMP,	copied+2, rFIRST );
						in( iSTH, r0,	copied+0, rSECOND );
						in( iSTB, rTMP,	copied+2, rSECOND );
					} else if ( left == 2 ) {
						in( iLHZ, r0, copied+0, rFIRST );
						in( iSTH, r0, copied+0, rSECOND );
					} else if ( left == 1 ) {
						in( iLBZ, r0, copied+0, rFIRST );
						in( iSTB, r0, copied+0, rSECOND );
					}
				} else
#endif
				{
					unsigned long int r5_ori = 0;
					if ( i_now->arg.si >= -0x8000 && i_now->arg.si < 0x8000 ) {
						in( iLI, r5, i_now->arg.si );
					} else if ( i_now->arg.i < 0x10000 ) {
						in( iLI, r5, 0 );
						r5_ori = i_now->arg.i;
					} else {
						in( iLIS, r5, i_now->arg.ss[ HI16 ] );
						r5_ori = i_now->arg.us[ LO16 ];
					}

					in( iLL, r0, VM_Data_Offset( BlockCopy ), rVMDATA ); // get blockCopy pointer

					if ( r5_ori )
						in( iORI, r5, r5, r5_ori );

#if ELFV2
					in( iMR, r12, r0 ); // ELFv2: r12 must hold target addr
#endif
					in( iMTCTR, r0 );

					if ( rFIRST != r4 )
						in( iMR, r4, rFIRST );
					if ( rSECOND != r3 )
						in( iMR, r3, rSECOND );

					in( iBCTRL );
				}

				gpr_pos -= 2;
				break;

			case OP_SEX8:
				MAYBE_EMIT_CONST();
				in( iEXTSB, rFIRST, rFIRST );
				break;

			case OP_SEX16:
				MAYBE_EMIT_CONST();
				in( iEXTSH, rFIRST, rFIRST );
				break;

			case OP_NEGI:
				MAYBE_EMIT_CONST();
				in( iNEG, rFIRST, rFIRST );
				break;

			case OP_ADD:
				if ( i_const ) {
					EMIT_FALSE_CONST();

					signed short int hi, lo;
					hi = i_const->arg.ss[ HI16 ];
					lo = i_const->arg.ss[ LO16 ];
					if ( lo < 0 )
						hi += 1;

					if ( hi != 0 )
						in( iADDIS, rSECOND, rSECOND, hi );
					if ( lo != 0 )
						in( iADDI, rSECOND, rSECOND, lo );

					// if both are zero no instruction will be written
					if ( hi == 0 && lo == 0 )
						force_emit = 1;
				} else {
					MAYBE_EMIT_CONST();
					in( iADD, rSECOND, rSECOND, rFIRST );
				}
				gpr_pos--;
				break;

			case OP_SUB:
				MAYBE_EMIT_CONST();
				in( iSUB, rSECOND, rSECOND, rFIRST );
				gpr_pos--;
				break;

			case OP_DIVI:
				MAYBE_EMIT_CONST();
				in( iDIVW, rSECOND, rSECOND, rFIRST );
				gpr_pos--;
				break;

			case OP_DIVU:
				MAYBE_EMIT_CONST();
				in( iDIVWU, rSECOND, rSECOND, rFIRST );
				gpr_pos--;
				break;

			case OP_MODI:
				MAYBE_EMIT_CONST();
				in( iDIVW, r0, rSECOND, rFIRST );
				in( iMULLW, r0, r0, rFIRST );
				in( iSUB, rSECOND, rSECOND, r0 );
				gpr_pos--;
				break;

			case OP_MODU:
				MAYBE_EMIT_CONST();
				in( iDIVWU, r0, rSECOND, rFIRST );
				in( iMULLW, r0, r0, rFIRST );
				in( iSUB, rSECOND, rSECOND, r0 );
				gpr_pos--;
				break;

			case OP_MULI:
			case OP_MULU:
				MAYBE_EMIT_CONST();
				in( iMULLW, rSECOND, rSECOND, rFIRST );
				gpr_pos--;
				break;

			case OP_BAND:
				MAYBE_EMIT_CONST();
				in( iAND, rSECOND, rSECOND, rFIRST );
				gpr_pos--;
				break;

			case OP_BOR:
				MAYBE_EMIT_CONST();
				in( iOR, rSECOND, rSECOND, rFIRST );
				gpr_pos--;
				break;

			case OP_BXOR:
				MAYBE_EMIT_CONST();
				in( iXOR, rSECOND, rSECOND, rFIRST );
				gpr_pos--;
				break;

			case OP_BCOM:
				MAYBE_EMIT_CONST();
				in( iNOT, rFIRST, rFIRST );
				break;

			case OP_LSH:
				MAYBE_EMIT_CONST();
				in( iSLW, rSECOND, rSECOND, rFIRST );
				gpr_pos--;
				break;

			case OP_RSHI:
				MAYBE_EMIT_CONST();
				in( iSRAW, rSECOND, rSECOND, rFIRST );
				gpr_pos--;
				break;

			case OP_RSHU:
				MAYBE_EMIT_CONST();
				in( iSRW, rSECOND, rSECOND, rFIRST );
				gpr_pos--;
				break;

			case OP_NEGF:
				MAYBE_EMIT_CONST();
				in( iFNEG, fFIRST, fFIRST );
				break;

			case OP_ADDF:
				MAYBE_EMIT_CONST();
				in( iFADDS, fSECOND, fSECOND, fFIRST );
				fpr_pos--;
				break;

			case OP_SUBF:
				MAYBE_EMIT_CONST();
				in( iFSUBS, fSECOND, fSECOND, fFIRST );
				fpr_pos--;
				break;

			case OP_DIVF:
				MAYBE_EMIT_CONST();
				in( iFDIVS, fSECOND, fSECOND, fFIRST );
				fpr_pos--;
				break;

			case OP_MULF:
				MAYBE_EMIT_CONST();
				in( iFMULS, fSECOND, fSECOND, fFIRST );
				fpr_pos--;
				break;

			case OP_CVIF:
				MAYBE_EMIT_CONST();
				fpr_pos++;
				in( iXORIS, rFIRST, rFIRST, 0x8000 );
				in( iLIS, r0, 0x4330 );
				in( iSTW, rFIRST, stack_temp + FPRLO, r1 );
				in( iSTW, r0, stack_temp + FPRHI, r1 );
				in( iLFS, fTMP, VM_Data_Offset( floatBase ), rVMDATA );
				in( iLFD, fFIRST, stack_temp, r1 );
				in( iFSUB, fFIRST, fFIRST, fTMP );
				in( iFRSP, fFIRST, fFIRST );
				gpr_pos--;
				break;

			case OP_CVFI:
				MAYBE_EMIT_CONST();
				gpr_pos++;
				in( iFCTIWZ, fFIRST, fFIRST );
				in( iSTFD, fFIRST, stack_temp, r1 );
				in( iLWZ, rFIRST, stack_temp + FPRLO, r1 );
				fpr_pos--;
				break;
		}

		i_const = NULL;

		if ( i_now->op != OP_CONST ) {
			// emit the instructions if it isn't OP_CONST
			emitEnd();
		} else {
			// mark in what register the value should be saved
			if ( RET_INT )
				i_now->regPos = ++gpr_pos;
			else
				i_now->regPos = ++fpr_pos;

#if OPTIMIZE_HOLE
			i_const = i_now;
#else
			PPC_EmitConst( i_now );
#endif
		}
	}
	if ( i_const )
		DIE( "left (unused) OP_CONST" );

	{
		// free opcode information, don't free first dummy one
		source_instruction_t *i_next = i_first->next;
		while ( i_next ) {
			i_now = i_next;
			i_next = i_now->next;
			PPC_Free( i_now );
		}
	}
}


/*
 * check which jumps are short enough to use signed 16bit immediate branch
 */
static void
PPC_ShrinkJumps( void )
{
	symbolic_jump_t *sj_now = sj_first;
	while ( (sj_now = sj_now->nextJump) ) {
		if ( sj_now->bo == branchAlways )
			// non-conditional branch has 26bit immediate
			sj_now->parent->length = 1;

		else {
			dest_instruction_t *di = di_pointers[ sj_now->jump_to ];
			dest_instruction_t *ji = sj_now->parent;
			long int jump_length = 0;
			if ( ! di )
				DIE( "No instruction to jump to" );
			if ( ji->count > di->count ) {
				do {
					jump_length += di->length;
				} while ( ( di = di->next ) != ji );
			} else {
				jump_length = 1;
				while ( ( ji = ji->next ) != di )
					jump_length += ji->length;
			}
			if ( jump_length < 0x2000 )
				// jump is short, use normal instruction
				sj_now->parent->length = 1;
		}
	}
}

/*
 * puts all the data in one place, it consists of many different tasks
 */
static void
PPC_ComputeCode( vm_t *vm )
{
	dest_instruction_t *di_now = di_first;

	unsigned long int codeInstructions = 0;
	// count total instruciton number
	while ( (di_now = di_now->next ) )
		codeInstructions += di_now->length;

	size_t codeLength = sizeof( vm_data_t )
		+ sizeof( unsigned int ) * data_acc
		+ sizeof( ppc_instruction_t ) * codeInstructions;

	// get the memory for the generated code, smarter ppcs need the
	// mem to be marked as executable (whill change later)
	unsigned char *dataAndCode = mmap( NULL, codeLength,
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0 );

	if (dataAndCode == MAP_FAILED)
		DIE( "Not enough memory" );

	ppc_instruction_t *codeNow, *codeBegin;
	codeNow = codeBegin = (ppc_instruction_t *)( dataAndCode + VM_Data_Offset( data[ data_acc ] ) );

	ppc_instruction_t nop = IN( iNOP );

	// copy instructions to the destination
	// fills the jump instructions with nops
	// saves pointers of all instructions
	di_now = di_first;
	while ( (di_now = di_now->next ) ) {
		unsigned long int i_count = di_now->i_count;
		if ( i_count != FALSE_ICOUNT ) {
			if ( ! di_pointers[ i_count ] )
				di_pointers[ i_count ] = (void *) codeNow;
		}

		if ( di_now->jump == NULL ) {
			memcpy( codeNow, &(di_now->code[0]), di_now->length * sizeof( ppc_instruction_t ) );
			codeNow += di_now->length;
		} else {
			long int i;
			symbolic_jump_t *sj;
			for ( i = 0; i < di_now->length; i++ )
				codeNow[ i ] = nop;
			codeNow += di_now->length;

			sj = di_now->jump;
			// save position of jumping instruction
			sj->parent = (void *)(codeNow - 1);
		}
	}

	// compute the jumps and write corresponding instructions
	symbolic_jump_t *sj_now = sj_first;
	while ( (sj_now = sj_now->nextJump ) ) {
		ppc_instruction_t *jumpFrom = (void *) sj_now->parent;
		ppc_instruction_t *jumpTo = (void *) di_pointers[ sj_now->jump_to ];
		signed long int jumpLength = jumpTo - jumpFrom;

		// if jump is short, just write it
		if ( jumpLength >= - 8192 && jumpLength < 8192 ) {
			powerpc_iname_t branchConditional = sj_now->ext & branchExtLink ? iBCL : iBC;
			*jumpFrom = IN( branchConditional, sj_now->bo, sj_now->bi, jumpLength * 4 );
			continue;
		}

		// jump isn't short so write it as two instructions
		//
		// the letter one is a non-conditional branch instruction which
		// accepts immediate values big enough (26 bits)
		*jumpFrom = IN( (sj_now->ext & branchExtLink ? iBL : iB), jumpLength * 4 );
		if ( sj_now->bo == branchAlways )
			continue;

		// there should have been additional space prepared for this case
		if ( jumpFrom[ -1 ] != nop )
			DIE( "additional space for long jump not prepared" );

		// invert instruction condition
		long int bo = 0;
		switch ( sj_now->bo ) {
			case branchTrue:
				bo = branchFalse;
				break;
			case branchFalse:
				bo = branchTrue;
				break;
			default:
				DIE( "unrecognized branch type" );
				break;
		}

		// the former instruction is an inverted conditional branch which
		// jumps over the non-conditional one
		jumpFrom[ -1 ] = IN( iBC, bo, sj_now->bi, +2*4 );
	}

	vm->codeBase = dataAndCode;
	vm->codeLength = codeLength;

	vm_data_t *data = (vm_data_t *)dataAndCode;

#if ELFV1
	// ELFv1: prepare Official Procedure Descriptor for the generated code
	// and retrieve real function pointer for helper functions

	opd_t *ac = (void *)VM_AsmCall, *bc = (void *)VM_BlockCopy;
	data->opd.function = codeBegin;
	// trick it into using the same TOC
	// this way we won't have to switch TOC before calling AsmCall or BlockCopy
	data->opd.toc = ac->toc;
	data->opd.env = ac->env;

	data->AsmCall = ac->function;
	data->BlockCopy = bc->function;
#else
	data->AsmCall = VM_AsmCall;
	data->BlockCopy = VM_BlockCopy;
#endif

	data->dataMask = vm->dataMask;
	data->iPointers = (ppc_instruction_t *)vm->instructionPointers;
	data->dataLength = VM_Data_Offset( data[ data_acc ] );
	data->codeLength = ( codeNow - codeBegin ) * sizeof( ppc_instruction_t );
	data->floatBase = 0x59800004;


	/* write dynamic data (float constants) */
	{
		local_data_t *d_next, *d_now = data_first;
		long int accumulated = 0;

		do {
			long int i;
			for ( i = 0; i < d_now->count; i++ )
				data->data[ accumulated + i ] = d_now->data[ i ];

			accumulated += d_now->count;
			d_next = d_now->next;
			PPC_Free( d_now );

			if ( !d_next )
				break;
			d_now = d_next;
		} while (1);
		data_first = NULL;
	}

	/* free most of the compilation memory */
	{
		di_now = di_first->next;
		PPC_Free( di_first );
		PPC_Free( sj_first );

		while ( di_now ) {
			di_first = di_now->next;
			if ( di_now->jump )
				PPC_Free( di_now->jump );
			PPC_Free( di_now );
			di_now = di_first;
		}
	}
}

static void
VM_Destroy_Compiled( vm_t *self )
{
	if ( self->codeBase ) {
		if ( munmap( self->codeBase, self->codeLength ) )
			Com_Printf( S_COLOR_RED "Memory unmap failed, possible memory leak\n" );
	}
	self->codeBase = NULL;
}

void
VM_Compile( vm_t *vm, vmHeader_t *header )
{
	long int pc = 0;
	unsigned long int i_count;
	char* code;
	struct timeval tvstart = {0, 0};
	source_instruction_t *i_first /* dummy */, *i_last = NULL, *i_now;

	vm->compiled = qfalse;

	gettimeofday(&tvstart, NULL);

	PPC_MakeFastMask( vm->dataMask );

	i_first = PPC_Malloc( sizeof( source_instruction_t ) );
	i_first->next = NULL;

	// vm->instructionPointers is already allocated as intptr_t* by vm.c
	// (pointer-sized entries), so no reallocation needed
	di_pointers = (void *)vm->instructionPointers;
	memset( di_pointers, 0, header->instructionCount * sizeof( void * ) );


	PPC_CompileInit();

	/*
	 * read the input program
	 * divide it into functions and send each function to compiler
	 */
	code = (char *)header + header->codeOffset;
	for ( i_count = 0; i_count < header->instructionCount; ++i_count )
	{
		unsigned char op = code[ pc++ ];

		if ( op == OP_ENTER ) {
			if ( i_first->next )
				VM_CompileFunction( i_first );
			i_first->next = NULL;
			i_last = i_first;
		}

		i_now = PPC_Malloc( sizeof( source_instruction_t ) );
		i_now->op = op;
		i_now->i_count = i_count;
		i_now->arg.i = 0;
		i_now->regA1 = 0;
		i_now->regA2 = 0;
		i_now->regR = 0;
		i_now->regPos = 0;
		i_now->next = NULL;

		if ( vm_opInfo[op] & opImm4 ) {
			i_now->arg.i = (unsigned char)code[ pc ]
				| ( (unsigned char)code[ pc + 1 ] << 8 )
				| ( (unsigned char)code[ pc + 2 ] << 16 )
				| ( (unsigned char)code[ pc + 3 ] << 24 );
			pc += 4;
		} else if ( vm_opInfo[op] & opImm1 ) {
			i_now->arg.b = code[ pc++ ];
		}

		i_last->next = i_now;
		i_last = i_now;
	}
	VM_CompileFunction( i_first );
	PPC_Free( i_first );

	PPC_ShrinkJumps();
	memset( di_pointers, 0, header->instructionCount * sizeof( void * ) );
	PPC_ComputeCode( vm );

	/* check for uninitialized pointers */
#ifdef DEBUG_VM
	long int i;
	for ( i = 0; i < header->instructionCount; i++ )
		if ( di_pointers[ i ] == 0 )
			Com_Printf( S_COLOR_RED "Pointer %ld not initialized !\n", i );
#endif

	/* flush data cache and invalidate instruction cache for generated code;
	 * PowerPC has split D/I caches and requires explicit synchronization */
	__builtin___clear_cache( vm->codeBase, vm->codeBase + vm->codeLength );

	/* mark memory as executable and not writeable */
	if ( mprotect( vm->codeBase, vm->codeLength, PROT_READ|PROT_EXEC ) ) {

		// it has failed, make sure memory is unmapped before throwing the error
		VM_Destroy_Compiled( vm );
		DIE( "mprotect failed" );
	}

	vm->destroy = VM_Destroy_Compiled;
	vm->compiled = qtrue;

	{
		struct timeval tvdone = {0, 0};
		struct timeval dur = {0, 0};
		Com_Printf( "VM file %s compiled to %i bytes of code (%p - %p)\n",
			vm->name, vm->codeLength, vm->codeBase, vm->codeBase+vm->codeLength );

		gettimeofday(&tvdone, NULL);
		timersub(&tvdone, &tvstart, &dur);
		Com_Printf( "compilation took %lu.%06lu seconds\n",
			(long unsigned int)dur.tv_sec, (long unsigned int)dur.tv_usec );
	}
}

int
VM_CallCompiled( vm_t *vm, int *args )
{
	int retVal;
	int *argPointer;

	vm_data_t *vm_dataAndCode = (void *)( vm->codeBase );
	int programStack = vm->programStack;
	int stackOnEntry = programStack;

	byte *image = vm->dataBase;

	currentVM = vm;

	vm->currentlyInterpreting = qtrue;

	programStack -= ( 8 + 4 * MAX_VMMAIN_ARGS );
	argPointer = (int *)&image[ programStack + 8 ];
	memcpy( argPointer, args, 4 * MAX_VMMAIN_ARGS );
	argPointer[ -1 ] = 0;
	argPointer[ -2 ] = -1;

#ifdef VM_TIMES
	struct tms start_time, stop_time;
	clock_t time_diff;

	times( &start_time );
	time_outside_vm = 0;
#endif

	/* call generated code */
	{
		int ( *entry )( void *, int, void * );
#if ELFV1
		entry = (void *)&(vm_dataAndCode->opd);
#else
		entry = (void *)(vm->codeBase + vm_dataAndCode->dataLength);
#endif
		retVal = entry( vm->codeBase, programStack, vm->dataBase );
	}

#ifdef VM_TIMES
	times( &stop_time );
	time_diff = stop_time.tms_utime - start_time.tms_utime;
	time_total_vm += time_diff - time_outside_vm;
	if ( time_diff > 100 ) {
		printf( "App clock: %ld, vm total: %ld, vm this: %ld, vm real: %ld, vm out: %ld\n"
			"Inside VM %f%% of app time\n",
			stop_time.tms_utime,
			time_total_vm,
			time_diff,
			time_diff - time_outside_vm,
			time_outside_vm,
			(double)100 * time_total_vm / stop_time.tms_utime );
	}
#endif

	vm->programStack = stackOnEntry;
	vm->currentlyInterpreting = qfalse;

	return retVal;
}

#endif // powerpc
