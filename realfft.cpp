// Time-stamp: <2016-12-19 17:54:56 awarkentin>
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
#include "realfft.h"
#include "stl_ext.h"

RealFFT::RealFFT (bool optimal_plan, MPIRFFTWInput & input, const char *import_wisdom_file_name):friendly_input (&input)
{

  // Flags for plan creation.
  int
    rfftw_mpi_plan_flags = 0;

  // Check if we need to import wisdom.
  if (import_wisdom_file_name != NULL)
    {

      // Yup. Open wisdom file.
      FILE *
	wisdom_file;
      if ((wisdom_file = fopen (import_wisdom_file_name, "r")) != NULL)
	// And import.
	fftw_import_wisdom_from_file (wisdom_file);
      else
	throw RealFFTException (RealFFTException::EFIO,
				std::
				string ("couldn't open input wisdom file '") +
				import_wisdom_file_name +
				std::string ("' for import"));
    }

  // If we are creating an optimal (slow creation, fastest transform) plan.
  if (optimal_plan)
    rfftw_mpi_plan_flags = FFTW_MEASURE;
  else
    rfftw_mpi_plan_flags = FFTW_ESTIMATE;

  // Create a forward two-dimensional RFFTW MPI plan, with the size
  // of the second dimension 1, as we really are doing a one-dimenstional
  // transformation. FFTW2 refuses to create an MPI one-dimensional plan :-(.
  myplan = rfftw2d_mpi_create_plan (MPI_COMM_WORLD,
				    (*friendly_input).total_data_points_count,
				    1,
				    FFTW_REAL_TO_COMPLEX,
				    rfftw_mpi_plan_flags | FFTW_USE_WISDOM);

  // Check if we actually created the plan.
  if (myplan == NULL)
    throw
      RealFFTException (RealFFTException::EPLAN,
			std::string ("plan creation failed :-(("));

  // Compute how much data (and what data) we need to load in this MPI process.
  rfftwnd_mpi_local_sizes (myplan,
			   &how_many_to_be_read,
			   &how_many_to_be_skipped,
			   &how_many_to_be_read_transposed,
			   &how_many_to_be_skipped_transposed,
			   &local_data_array_length);

  // local_data_array_length is counted in fftw_real(s).
  // Lets page-align this array.
  if (posix_memalign ((void **) (&output_data_array),
		      sysconf (_SC_PAGESIZE),
		      sizeof (fftw_real) * local_data_array_length) == ENOMEM)
    throw
      RealFFTException (RealFFTException::EMEM,
			std::string ("couldn't allocate output array of ") +
			to_string (local_data_array_length) +
			std::
			string
			(" fftw_reals. Maybe data too big to fit in memory? Increase number of MPI nodes"));
}

RealFFT::~RealFFT ()
{
  free (output_data_array);
}

void
RealFFT::export_wisdom (const char *export_wisdom_file_name)
{

  // Only export if we are given a file name.
  if (export_wisdom_file_name != NULL)
    {

      // Open wisdom file.
      FILE *wisdom_file;
      if ((wisdom_file = fopen (export_wisdom_file_name, "w")) != NULL)

	// And export.
	fftw_export_wisdom_to_file (wisdom_file);
      else
	throw RealFFTException (RealFFTException::EFIO,
				std::
				string ("couldn't open output wisdom file '")
				+ std::string (export_wisdom_file_name) +
				std::string ("' for export"));
    }
}

void
RealFFT::do_transform ()
{
  
  // Do transform.
  rfftwnd_mpi (myplan,
	       1,
	       (*friendly_input).input_data_array,
	       (fftw_real *) output_data_array, FFTW_NORMAL_ORDER);
  
  // Destroy the plan. Not needed anymore.
  rfftwnd_mpi_destroy_plan (myplan);
}

void
RealFFT::export_transformed (const char *export_transformed_file_name)
{

  // Only export if we are given a file name.
  if (export_transformed_file_name != NULL)
    {

      // Open file.
      std::ofstream fout;
      fout.open (export_transformed_file_name);
      if (!fout.is_open ())
	throw RealFFTException (RealFFTException::EFIO,
				std::string ("could not open '") +
				std::string (export_transformed_file_name) +
				std::string ("' for writing"));

      // Attempt writing-out data.
      try
      {

        // Set output format.
        // Yuck :-). Hey, at least this way I don't need to determine the
        // precision by hand.
        fout.setf(std::ios_base::scientific, std::ios_base::floatfield); 
        fout.precision((int)(std::ceil(std::log10(std::pow(2.0,(double)(CHAR_BIT*sizeof(double)))))));
	fout << "# re, im" << std::endl;

        // The output consists of the same number of points as the input.
        // Yes this isn't clean, but the alternative is not clean either
        // when it's patently obvious this is the case.
	for (size_t ix = 0; ix < (*friendly_input).total_data_points_count;
	     ix++)
	  fout << output_data_array[ix].re << ", " << output_data_array[ix].
	    im << std::endl;
	fout.close ();
      }
      catch (std::ios::failure & err)
      {
	throw RealFFTException (RealFFTException::EFIO,
				std::string ("could not write to '") +
				std::string (export_transformed_file_name) +
				std::string ("'"));
      }
    }
}
