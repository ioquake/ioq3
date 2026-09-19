/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

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

/* This file is only compiled for PowerPC builds with Altivec support.
   Altivec intrinsics need to be in a separate file, so GCC's -maltivec
   command line can enable them, but give us the option to _not_ use that
   on other files, where the compiler might then generate Altivec
   instructions for normal floating point, crashing on G3 (etc) processors. */

#include "client.h"
#include "snd_local.h"

#if idppc_altivec

#if !defined(__APPLE__)
#include <altivec.h>
#undef bool
#undef pixel
#else
/* Apple's -faltivec provides the AltiVec keywords without altivec.h,
   but only the unprefixed spellings */
#define __vector vector
#endif

void S_PaintChannelFrom16_altivec( portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE], int snd_vol, channel_t *ch, const sfx_t *sc, int count, int sampleOffset, int bufferOffset ) {
	int						data, aoff, boff;
	int						leftvol, rightvol;
	int						i, j;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	short					*samples;
	float					ooff, fdata[2], fdiv, fleftvol, frightvol;

	if (sc->soundChannels <= 0) {
		return;
	}

	samp = &paintbuffer[ bufferOffset ];

	if (ch->doppler) {
		sampleOffset = sampleOffset*ch->oldDopplerScale;
	}

	if ( sc->soundChannels == 2 ) {
		sampleOffset *= sc->soundChannels;

		if ( sampleOffset & 1 ) {
			sampleOffset &= ~1;
		}
	}

	chunk = sc->soundData;
	while (sampleOffset>=SND_CHUNK_SIZE) {
		chunk = chunk->next;
		sampleOffset -= SND_CHUNK_SIZE;
		if (!chunk) {
			chunk = sc->soundData;
		}
	}

	if (!ch->doppler || ch->dopplerScale==1.0f) {
		__vector signed short volume_vec;
		__vector unsigned int volume_shift;
		int vectorCount, samplesLeft, chunkSamplesLeft;
		leftvol = ch->leftvol*snd_vol;
		rightvol = ch->rightvol*snd_vol;
		samples = chunk->sndChunk;
#if defined(__VSX__)
		/* Build volume vector: {L, L, R, R, L, L, R, R}
		   Use vec_perm to interleave left and right volume splats
		   in an endian-safe way (BE-style indices). */
		{
			__vector signed short vl = vec_splats((short)leftvol);
			__vector signed short vr = vec_splats((short)rightvol);
			__vector unsigned char volume_perm = VECCONST_UINT8(
				0, 1, 2, 3, 16, 17, 18, 19,
				4, 5, 6, 7, 20, 21, 22, 23);
			volume_vec = (__vector signed short)vec_perm(vl, vr, volume_perm);
		}
#else
		((short *)&volume_vec)[0] = leftvol;
		((short *)&volume_vec)[1] = leftvol;
		((short *)&volume_vec)[4] = leftvol;
		((short *)&volume_vec)[5] = leftvol;
		((short *)&volume_vec)[2] = rightvol;
		((short *)&volume_vec)[3] = rightvol;
		((short *)&volume_vec)[6] = rightvol;
		((short *)&volume_vec)[7] = rightvol;
#endif
		volume_shift = vec_splat_u32(8);
		i = 0;

		while(i < count) {
			/* Try to align destination to 16-byte boundary */
			while(i < count && (((unsigned long)&samp[i] & 0x1f) || ((count-i) < 8) || ((SND_CHUNK_SIZE - sampleOffset) < 8))) {
				data  = samples[sampleOffset++];
				samp[i].left += (data * leftvol)>>8;

				if ( sc->soundChannels == 2 ) {
					data = samples[sampleOffset++];
				}
				samp[i].right += (data * rightvol)>>8;
	
				if (sampleOffset == SND_CHUNK_SIZE) {
					chunk = chunk->next;
					samples = chunk->sndChunk;
					sampleOffset = 0;
				}
				i++;
			}
			/* Destination is now aligned.  Process as many 8-sample 
			   chunks as we can before we run out of room from the current
			   sound chunk.  We do 8 per loop to avoid extra source data reads. */
			samplesLeft = count - i;
			chunkSamplesLeft = SND_CHUNK_SIZE - sampleOffset;
			if(samplesLeft > chunkSamplesLeft)
				samplesLeft = chunkSamplesLeft;
			
			vectorCount = samplesLeft / 8;
			
			if(vectorCount)
			{
#if defined(__VSX__)
				__vector short s0, sampleData0, sampleData1;
				__vector signed int merge0, merge1;
				__vector signed int d0, d1, d2, d3;
				__vector unsigned char samplePermute0 =
					VECCONST_UINT8(0, 1, 4, 5, 0, 1, 4, 5, 2, 3, 6, 7, 2, 3, 6, 7);
				__vector unsigned char samplePermute1 =
					VECCONST_UINT8(8, 9, 12, 13, 8, 9, 12, 13, 10, 11, 14, 15, 10, 11, 14, 15);

				while(vectorCount)
				{
					/* Load up source (16-bit) sample data */
					s0 = vec_xl(0, &samples[sampleOffset]);

					/* Load up destination sample data */
					d0 = vec_xl(0, (int *)&samp[i]);
					d1 = vec_xl(0, (int *)&samp[i+2]);
					d2 = vec_xl(0, (int *)&samp[i+4]);
					d3 = vec_xl(0, (int *)&samp[i+6]);

					/* Rearrange samples: duplicate each sample for L/R processing.
					   samplePermute0 selects first 4 samples (each duplicated twice),
					   samplePermute1 selects second 4 samples (each duplicated twice). */
					sampleData0 = vec_perm(s0, s0, samplePermute0);
					sampleData1 = vec_perm(s0, s0, samplePermute1);

					merge0 = vec_mule(sampleData0, volume_vec);
					merge0 = vec_sra(merge0, volume_shift);

					merge1 = vec_mulo(sampleData0, volume_vec);
					merge1 = vec_sra(merge1, volume_shift);

					d0 = vec_add(merge0, d0);
					d1 = vec_add(merge1, d1);

					merge0 = vec_mule(sampleData1, volume_vec);
					merge0 = vec_sra(merge0, volume_shift);

					merge1 = vec_mulo(sampleData1, volume_vec);
					merge1 = vec_sra(merge1, volume_shift);

					d2 = vec_add(merge0, d2);
					d3 = vec_add(merge1, d3);

					/* Store destination sample data */
					vec_xst(d0, 0, (int *)&samp[i]);
					vec_xst(d1, 0, (int *)&samp[i+2]);
					vec_xst(d2, 0, (int *)&samp[i+4]);
					vec_xst(d3, 0, (int *)&samp[i+6]);

					i += 8;
					vectorCount--;
					sampleOffset += 8;
				}
#else
				__vector unsigned char tmp;
				__vector short s0, s1, sampleData0, sampleData1;
				__vector signed int merge0, merge1;
				__vector signed int d0, d1, d2, d3;
				__vector unsigned char samplePermute0 =
					VECCONST_UINT8(0, 1, 4, 5, 0, 1, 4, 5, 2, 3, 6, 7, 2, 3, 6, 7);
				__vector unsigned char samplePermute1 =
					VECCONST_UINT8(8, 9, 12, 13, 8, 9, 12, 13, 10, 11, 14, 15, 10, 11, 14, 15);
				__vector unsigned char loadPermute0, loadPermute1;
				
				// Rather than permute the vectors after we load them to do the sample
				// replication and rearrangement, we permute the alignment vector so
				// we do everything in one step below and avoid data shuffling.
				tmp = vec_lvsl(0,&samples[sampleOffset]);								
				loadPermute0 = vec_perm(tmp,tmp,samplePermute0);
				loadPermute1 = vec_perm(tmp,tmp,samplePermute1);
				
				s0 = *(__vector short *)&samples[sampleOffset];
				while(vectorCount)
				{
					/* Load up source (16-bit) sample data.  The offset
					   must reach the next 16-byte block for every source
					   alignment: +8 shorts (+16 bytes) always does, while
					   the old +7 stayed in the current block when
					   sampleOffset was 16-byte aligned, going stale after
					   the s0 = s1 carry below */
					s1 = *(__vector short *)&samples[sampleOffset+8];
					
					/* Load up destination sample data */
					d0 = *(__vector signed int *)&samp[i];
					d1 = *(__vector signed int *)&samp[i+2];
					d2 = *(__vector signed int *)&samp[i+4];
					d3 = *(__vector signed int *)&samp[i+6];

					sampleData0 = vec_perm(s0,s1,loadPermute0);
					sampleData1 = vec_perm(s0,s1,loadPermute1);
					
					merge0 = vec_mule(sampleData0,volume_vec);
					merge0 = vec_sra(merge0,volume_shift);	/* Shift down to proper range */
					
					merge1 = vec_mulo(sampleData0,volume_vec);
					merge1 = vec_sra(merge1,volume_shift);
					
					d0 = vec_add(merge0,d0);
					d1 = vec_add(merge1,d1);
					
					merge0 = vec_mule(sampleData1,volume_vec);
					merge0 = vec_sra(merge0,volume_shift);	/* Shift down to proper range */
					
					merge1 = vec_mulo(sampleData1,volume_vec);
					merge1 = vec_sra(merge1,volume_shift);					

					d2 = vec_add(merge0,d2);
					d3 = vec_add(merge1,d3);

					/* Store destination sample data */
					*(__vector signed int *)&samp[i] = d0;
					*(__vector signed int *)&samp[i+2] = d1;
					*(__vector signed int *)&samp[i+4] = d2;
					*(__vector signed int *)&samp[i+6] = d3;

					i += 8;
					vectorCount--;
					s0 = s1;
					sampleOffset += 8;
				}
#endif
				if (sampleOffset == SND_CHUNK_SIZE) {
					chunk = chunk->next;
					samples = chunk->sndChunk;
					sampleOffset = 0;
				}
			}
		}
	} else {
		fleftvol = ch->leftvol*snd_vol;
		frightvol = ch->rightvol*snd_vol;

		ooff = sampleOffset;
		samples = chunk->sndChunk;
		
		for ( i=0 ; i<count ; i++ ) {

			aoff = ooff;
			ooff = ooff + ch->dopplerScale * sc->soundChannels;
			boff = ooff;
			fdata[0] = fdata[1] = 0;
			for (j=aoff; j<boff; j += sc->soundChannels) {
				if (j == SND_CHUNK_SIZE) {
					chunk = chunk->next;
					if (!chunk) {
						chunk = sc->soundData;
					}
					samples = chunk->sndChunk;
					ooff -= SND_CHUNK_SIZE;
				}
				if ( sc->soundChannels == 2 ) {
					fdata[0] += samples[j&(SND_CHUNK_SIZE-1)];
					fdata[1] += samples[(j+1)&(SND_CHUNK_SIZE-1)];
				} else {
					fdata[0] += samples[j&(SND_CHUNK_SIZE-1)];
					fdata[1] += samples[j&(SND_CHUNK_SIZE-1)];
				}
			}
			fdiv = 256 * (boff-aoff) / sc->soundChannels;
			samp[i].left += (fdata[0] * fleftvol)/fdiv;
			samp[i].right += (fdata[1] * frightvol)/fdiv;
		}
	}
}


#endif
