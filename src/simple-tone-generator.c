/*
 * simple-tone-generator.c
 *
 * Copyright (C) 2011-2012 Kamal Mostafa <kamal@whence.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <math.h>
#include <strings.h>
#include <malloc.h>
#include <assert.h>

#include "simpleaudio.h"



static unsigned int sin_table_len;
static short *sin_table_short;
static float *sin_table_float;

void
simpleaudio_tone_init( unsigned int new_sin_table_len )
{
    sin_table_len = new_sin_table_len;

    if ( sin_table_len != 0 ) {
	sin_table_short = realloc(sin_table_short, sin_table_len * sizeof(short));
	sin_table_float = realloc(sin_table_float, sin_table_len * sizeof(float));
	if ( !sin_table_short || !sin_table_float ) {
	    perror("malloc");
	    assert(0);
	}

	unsigned int i;
	for ( i=0; i<sin_table_len; i++ )
	    sin_table_short[i] = 32767 * sin(M_PI*2.0*(float)i/sin_table_len);
	for ( i=0; i<sin_table_len; i++ )
	    sin_table_float[i] = sinf(M_PI*2.0*(float)i/sin_table_len);

    } else {
	if ( sin_table_short ) {
	    free(sin_table_short);
	    sin_table_short = NULL;
	}
	if ( sin_table_float ) {
	    free(sin_table_float);
	    sin_table_float = NULL;
	}
    }
}

/*
 * in: turns (0.0 to 1.0)    out: (-32767 to +32767)
 */
static inline short
sin_lu_short( float turns )
{
    int t = (float)sin_table_len * turns + 0.5f;
    t %= sin_table_len;
    return sin_table_short[t];
}

/*
 * in: turns (0.0 to 1.0)    out: -1.0 to +1.0
 */
static inline float
sin_lu_float( float turns )
{
    int t = (float)sin_table_len * turns + 0.5f;
    t %= sin_table_len;
    return sin_table_float[t];
}


/* "current" phase state of the tone generator */
static float sa_tone_cphase = 0.0;

void
simpleaudio_tone_reset()
{
    sa_tone_cphase = 0.0;
}

void
simpleaudio_tone(simpleaudio *sa_out, float tone_freq, size_t nsamples_dur)
{
    unsigned int framesize = simpleaudio_get_framesize(sa_out);

    void *buf = malloc(nsamples_dur * framesize);
    assert(buf);

    if ( tone_freq != 0 ) {

	float wave_nsamples = simpleaudio_get_rate(sa_out) / tone_freq;
	size_t i;

#define TURNS_TO_RADIANS(t)	( M_PI*2.0 * (t) )

#define SINE_PHASE_TURNS	( (float)i/wave_nsamples + sa_tone_cphase )
#define SINE_PHASE_RADIANS	TURNS_TO_RADIANS(SINE_PHASE_TURNS)

	switch ( simpleaudio_get_format(sa_out) ) {

	    case SA_SAMPLE_FORMAT_FLOAT:
		{
		    float *float_buf = buf;
		    if ( sin_table_float ) {
			for ( i=0; i<nsamples_dur; i++ )
			    float_buf[i] = sin_lu_float(SINE_PHASE_TURNS);
		    } else {
			for ( i=0; i<nsamples_dur; i++ )
			    float_buf[i] = sinf(SINE_PHASE_RADIANS);
		    }
		}
		break;

	    case SA_SAMPLE_FORMAT_S16:
		{
		    short *short_buf = buf;
		    if ( sin_table_short ) {
			for ( i=0; i<nsamples_dur; i++ )
			    short_buf[i] = sin_lu_short(SINE_PHASE_TURNS);
		    } else {
			for ( i=0; i<nsamples_dur; i++ )
			    short_buf[i] = 32767 * sinf(SINE_PHASE_RADIANS);
		    }
		    break;
		}

	    default:
		assert(0);
		break;
	}

	sa_tone_cphase
	    = fmodf(sa_tone_cphase + (float)nsamples_dur/wave_nsamples, 1.0);

    } else {

	bzero(buf, nsamples_dur * framesize);
	sa_tone_cphase = 0.0;

    }

    assert ( simpleaudio_write(sa_out, buf, nsamples_dur) > 0 );

    free(buf);
}

