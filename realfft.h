// Time-stamp: <2005-02-20 01:54:20 andyw>
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

#ifndef REALFFT_H
#define REALFFT_H

// System includes.
#include <mpi.h>
#include <cstdio>
#include <string>
#include <cstddef>
#include <rfftw_mpi.h>

// Local includes.
#include "ps_generator.h"
#include "mpirfftw_input.h"
#include "generic_exception.h"

// Forward declaration.
class PSGenerator;
class MPIRFFTWInput;

class RealFFTException:public GenericException
{
public:

  // Error types thrown.
  typedef enum
  {

    // File I/O error.
    EFIO,

    // Plan creation error.
    EPLAN,

    // Failure in memory allocation.
    EMEM
  } error_t;
private:

  // Error code associated with the exception.
    error_t error_code;
public:

  // Constructor used for creation of object.
    RealFFTException (error_t err,
		      const std::string & aux_err):GenericException (aux_err),
    error_code (err)
  {
  }

  // Returns the error code association with the exception.
  error_t get_error_code () const
  {
    return error_code;
  }
};

class RealFFT
{
private:

  // We're friends with MPIRFFTWInput.
  friend class MPIRFFTWInput;

  // We're friends with PSGenerator.
  friend class PSGenerator;

  // Plan.
  rfftwnd_mpi_plan myplan;

  // Current process needs to read-in this many data points...
  int how_many_to_be_read;

  // ...after skipping this many.
  int how_many_to_be_skipped;

  // Size (in fftw_reals) of output_data_array. (also of the input data array).
  int local_data_array_length;

  // If fftwnd_mpi is called with FFTW_TRANSPOSED_ORDER, then y will
  // be the first dimension for the output and the local y extend will be given
  // by how_many_to_be_read_transposed and local_y_start_after_transpose. We don't
  // care about these. Completely irrelevant for our task.
  int how_many_to_be_read_transposed;
  int how_many_to_be_skipped_transposed;

  // Pointer to the class friend object.
  MPIRFFTWInput *friendly_input;

  // Array to hold output data.
  fftw_complex *output_data_array;
public:

  // Constructor. Set true to optimal_plan if plan creation with FFTW_MEASURE
  // is desired. (slow plan creation!). Pass an MPIRFFTWInput object as it will be
  // needed. Pass import_wisdom_file_name as NULL if no wisdom is to be imported.
    RealFFT (bool optimal_plan,
	     MPIRFFTWInput & input, const char *import_wisdom_file_name);

  // Destructor.
   ~RealFFT ();

  // Exports wisdom to file, as long as the file name isn't a NULL pointer.
  void export_wisdom (const char *export_wisdom_file_name);

  // Exports the result of the transform to file, as long as the file name isn't a NULL pointer.
  void export_transformed (const char *export_transformed_file_name);

  // Performs transform.
  void do_transform ();
};
#endif
