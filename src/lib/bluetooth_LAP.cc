/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*
 * config.h is generated by configure.  It contains the results
 * of probing for features, options etc.  It should be the first
 * file included in your .cc file.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <bluetooth_LAP.h>
#include <sys/time.h>

/*
 * Create a new instance of bluetooth_LAP and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
bluetooth_LAP_sptr
bluetooth_make_LAP (int x)
{
  return bluetooth_LAP_sptr (new bluetooth_LAP (x));
}

//private constructor
bluetooth_LAP::bluetooth_LAP (int x)
  : bluetooth_block ()
{
	d_LAP = 0;
	d_stream_length = 0;
	d_consumed = 0;
	d_x = x;
	d_cumulative_count = 0;
	// ensure that we are always given at least 72 symbols
	set_history(72);
}

//virtual destructor.
bluetooth_LAP::~bluetooth_LAP ()
{
}

int 
bluetooth_LAP::work (int noutput_items,
			       gr_vector_const_void_star &input_items,
			       gr_vector_void_star &output_items)
{
	d_stream = (char *) input_items[0];
	// actual number of samples available to us is noutput_items + 72 (history)
	d_stream_length = noutput_items;
	int retval;

	retval = sniff_ac();
	d_consumed = (-1 == retval) ? noutput_items : retval + 72;
	d_cumulative_count += d_consumed;

    // Tell runtime system how many output items we produced.
	return d_consumed;
}

/* Looks for an AC in the stream */
int bluetooth_LAP::sniff_ac()
{
	int jump, count;
	uint8_t preamble; // start of sync word (includes LSB of LAP)
	uint16_t trailer; // end of sync word: barker sequence and trailer (includes MSB of LAP)
	int max_distance = 2; // maximum number of bit errors to tolerate in preamble + trailer
	uint32_t LAP;
	char *stream;

	for(count = 0; count < d_stream_length; count ++)
	{
		stream = &d_stream[count];
		preamble = air_to_host8(&stream[0], 5);
		trailer = air_to_host16(&stream[61], 11);
		if((preamble_distance[preamble] + trailer_distance[trailer]) <= max_distance)
		{
			LAP = air_to_host32(&stream[38], 24);
			if(check_ac(stream, LAP))
			{
				timeval tim;
				gettimeofday(&tim, NULL);
				//double timenow = tim.tv_usec;
				printf("GOT PACKET on %d , LAP = %06x at sample %d, wall time: %d.%06d\n", d_x, LAP, d_cumulative_count + count, tim.tv_sec, tim.tv_usec);
				return count;
			}
		}
	}
	return -1;
}
