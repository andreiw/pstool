// Time-stamp: <2016-12-19 17:55:23 awarkentin>
// Copyright (C) 2004 Andrey Warkentin
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

// System includes.
#include <cmath>
#include <cerrno>
#include <climits>
#include <unistd.h>

// Local includes.
#include "stl_ext.h"
#include "ps_generator.h"

PSGenerator::PSGenerator (RealFFT & transform, double sample_rate)
{

  // Size of power spectrum array.
  double data_points_count =
    (double) (*(transform.friendly_input)).total_data_points_count;
  ps_entries_count = (size_t) (data_points_count / 2 + 1);

  // Allocate space on heap for said array. Page align the array.
  if (posix_memalign ((void **) (&ps_entries),
		      sysconf (_SC_PAGESIZE),
		      sizeof (ps_entry) * ps_entries_count) == ENOMEM)
    throw PSGeneratorException (PSGeneratorException::EMEM,
				std::
				string
				("couldn't allocate power spectrum array of ")
				+ to_string (ps_entries_count) +
				std::string (" entries."));

  // Find size of each bin (in Hz).
  double bin_size = sample_rate / data_points_count;

  // Calculate power spectrum. Normalize according to Parseval's theorem.
  // Find DC component.
  ps_entries[0].hz = 0;
  ps_entries[0].joules_per_hz = (transform.output_data_array[0].re *
				 transform.output_data_array[0].re) /
    data_points_count;

  // ix < (data_points_count / 2) rounded up.
  for (double ix = 1; ix < (data_points_count + 1) / 2; ix++)
    {

      ps_entries[(size_t) ix].hz = ix * bin_size;
      ps_entries[(size_t) ix].joules_per_hz =
	2 *
	((transform.output_data_array[(size_t) ix].re *
	  transform.output_data_array[(size_t) ix].re) +
	 (transform.output_data_array[(size_t) ix].im *
	  transform.output_data_array[(size_t) ix].im)) / data_points_count;
    }
  if ((size_t) (data_points_count) % 2 == 0)
    {

      // Nyquist frequency.
      ps_entries[(size_t) (data_points_count / 2)].hz =
	(data_points_count / 2) * bin_size;
      ps_entries[(size_t) (data_points_count / 2)].joules_per_hz =
	(transform.output_data_array[(size_t) (data_points_count / 2)].re *
	 transform.output_data_array[(size_t) (data_points_count / 2)].re) /
	data_points_count;
    }
}

PSGenerator::~PSGenerator ()
{
  free (ps_entries);
}

void
PSGenerator::export_spectrum (const char *export_spectrum_file_name)
{

  // Only export if we are given a file name.
  if (export_spectrum_file_name != NULL)
    {
      std::ofstream fout;
      fout.open (export_spectrum_file_name);
      if (!fout.is_open ())
	throw PSGeneratorException (PSGeneratorException::EFIO,
				    std::string ("could not open '") +
				    std::string (export_spectrum_file_name) +
				    std::string ("' for writing"));

      // Attempt writing-out data.
      try
      {

        // Set output format.
        // Yuck :-). Hey, at least this way I don't need to determine the
        // precision by hand.
        fout.setf(std::ios_base::scientific, std::ios_base::floatfield); 
        fout.precision((int)(std::ceil(std::log10(std::pow(2.0,(double)(CHAR_BIT*sizeof(double)))))));
    	fout << "# Hz, J" << std::endl;
	for (size_t ix = 0; ix < ps_entries_count; ix++)
	  fout << ps_entries[ix].hz
	    << ", " << ps_entries[ix].joules_per_hz << std::endl;
	fout.close ();
      }
      catch (std::ios::failure & err)
      {
	throw PSGeneratorException (PSGeneratorException::EFIO,
				    std::string ("could not write to '") +
				    std::string (export_spectrum_file_name) +
				    std::string ("'"));
      }
    }
}
