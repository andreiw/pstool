// Time-stamp: <2005-02-20 01:54:09 andyw>
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

#ifndef PS_GENERATOR
#define PS_GENERATOR

// System includes.
#include <string>
#include <fstream>
#include <cstddef>

// Local includes.
#include "realfft.h"
#include "generic_exception.h"

// Forward declaration.
class RealFFT;

// Thrown at PSGenerator errors.
class PSGeneratorException:public GenericException
{
public:

  // Error types thrown.
  typedef enum
  {

    // File I/O error.
    EFIO,

    // Memory allocation error.
    EMEM
  } error_t;
private:

  // Error code associated with the exception.
    error_t error_code;
public:

  // Constructor used for creation of object.
    PSGeneratorException (error_t err,
			  const std::
			  string & aux_err):GenericException (aux_err),
    error_code (err)
  {
  }

  // Returns the error code association with the exception.
  error_t get_error_code () const
  {
    return error_code;
  }
};

class PSGenerator
{
public:
  typedef struct
  {
    double hz;
    double joules_per_hz;
  }
  ps_entry;
private:

  // Pointer to an array of ps_entry elements.
    ps_entry * ps_entries;

  // Number of entries in the above array.
  size_t ps_entries_count;
public:

  // Computes a one-sided power spectrum.
    PSGenerator (RealFFT & transform, double sample_rate);
   ~PSGenerator ();

  // Exports the power spectrum to a file, as long as the file
  // name isn't a NULL pointer.
  void export_spectrum (const char *export_spectrum_file_name);
};

#endif
