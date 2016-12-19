// Time-stamp: <2016-12-19 17:53:50 awarkentin>
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
#include <cerrno>
#include <unistd.h>

// Local includes.
#include "stl_ext.h"
#include "mpirfftw_input.h"

MPIRFFTWInput::MPIRFFTWInput (char *file_name):
  total_data_points_count (0)
{

  // Open the file.
  MPI_File_open (MPI_COMM_WORLD,
		 file_name, MPI_MODE_RDONLY, MPI_INFO_NULL, &infile_opened);

  // Check for failure.
  if (infile_opened == NULL)
    throw
      MPIRFFTWInputException (MPIRFFTWInputException::EFIO,
			      std::
			      string ("couldn't open input data file '") +
			      std::string (file_name) +
			      std::string ("' for reading"));

  // Find the total number of data points inside the opened file.
  // A data point consists of a single fftw_real.
  MPI_Offset
    filesize;
  MPI_File_get_size (infile_opened, &filesize);
  total_data_points_count = filesize / sizeof (fftw_real);

  // If the file doesn't contain at least one data point.
  if (total_data_points_count == 0)
    throw
      MPIRFFTWInputException (MPIRFFTWInputException::EEMPTY,
			      std::string ("input data file '") +
			      std::string (file_name) +
			      std::string ("' is lacking in data points"));
}

MPIRFFTWInput::~MPIRFFTWInput ()
{
  free (input_data_array);
}

void
MPIRFFTWInput::read_data (RealFFT & transform)
{

  // Allocate memory for input data array. Page align it.
  // Yes, even if rfftwnd_mpi_local_sizes dictates nothing to be read,
  // it still dictates an array to be allocated.
  if (posix_memalign ((void **) (&input_data_array),
                      sysconf (_SC_PAGESIZE),
                      sizeof (fftw_real) *
                      transform.local_data_array_length) == ENOMEM)
    throw MPIRFFTWInputException (MPIRFFTWInputException::EMEM,
                                  std::
                                  string
                                  ("couldn't allocate input array of ")
                                  +
                                  to_string (transform.
                                             local_data_array_length)
                                  +
                                  std::
                                  string
                                  (" fftw_reals. Maybe data too big to fit in memory? Increase number of MPI nodes"));

  // Create the file view.
  MPI_File_set_view (infile_opened,
                     transform.how_many_to_be_skipped *
                     sizeof (fftw_real), MPI_DOUBLE, MPI_DOUBLE,
                     (char *) "native", MPI_INFO_NULL);
  
  // Read in our data. First we must create a new input_data_array datatype, since
  // the data we read in will not be placed continuously in the input_data_array.
  // Instead we need to pad each fftw_real placed with another fftw_real.
  // This is due to the way rfftwnd_mpi expects input_data_array to be structured.
  MPI_Datatype input_data_array_type;
  MPI_Type_vector (transform.how_many_to_be_read,	// This many data points...
                   1,                               	// Each data point consisting of one double...
                   2,                                 	// ... which is padded with another double.
                   MPI_DOUBLE, &input_data_array_type);
  
  // Commit the datatype. Needed before we can use it.
  MPI_Type_commit (&input_data_array_type);
  
  // Read in the appropriate data.
  MPI_Status read_status;
  MPI_File_read (infile_opened,	        // Read from out opened input data file.
                 input_data_array,	// Into the input_data_array.
                 1,	                // Read 1 such layout described by input_data_array_type.
                 input_data_array_type, &read_status);
  
  // Close the file as it's not needed anymore.
  MPI_File_close (&infile_opened);
}
