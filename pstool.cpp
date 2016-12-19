// Time-stamp: <2016-12-19 17:51:32 awarkentin>
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
#include <string>
#include <cstdlib>
#include <iostream>
#include <signal.h>
#include <rfftw_mpi.h>
#include <exception>
#include <cerrno>
#include <cmath>
#include <unistd.h>
#include <getopt.h>


// If we're compiling with G++ >= 3, include these files - their contents 
// will be needed in exc_handler to create a more verbose 
// exception-handling-failure handler.
#if(defined(__GNUG__) && (__GNUC__ >= 3))
#   include <cxxabi.h>
#   include <typeinfo>
#endif

// Local includes.
#include "stl_ext.h"
#include "realfft.h"
#include "ps_generator.h"
#include "mpirfftw_input.h"

// Our version.
#define VERSION 1

void *
fftw_complex_aligned_malloc (size_t n)
{
  void *p;
  if (posix_memalign (&p, sizeof (fftw_complex), n) == ENOMEM)

    return NULL;
  else
    return p;
}

void
sig_handler(int signum)
{
  std::cerr << "Received UNIX signal "
            << signum
            << '.'
            << std::endl;

  // Die. A signal could technically arrive 
  // before MPI::Init is called, so we want to check that.
  if(MPI::Is_initialized ()) MPI::COMM_WORLD.Abort (EXIT_FAILURE); 
  else
    exit (-1);
}

void exc_handler()
{
  
  // Notify the user of exception-handling-failure.
  // If pstool is compiled with G++ >= 3 or above,
  // then we can be more specific about the exception-handling
  // problem. This G++ >= 3-based miracle has been ruthlessly 
  // ripped from G++ >= 3's verbose_terminate_handler and modified
  // as seen fit.
  // G++ specific code, which uses G++'s ABI namespace, follows.
#if(defined(__GNUG__) && (__GNUC__ >= 3))
  const char *name;
  char *demangled_name; 
  int demangle_status = -1;
  const char *exception_what_msg = NULL;
  
  // Make sure there was an exception. Terminate is also called for an
  // attempt to rethrow when there is no suitable exception.
  // Thus we get "type information" using obscure G++ ABI.
  std::type_info *t_exc = abi::__cxa_current_exception_type();
  if(t_exc)
    {

      // Note that "name" is the mangled name.
      name = t_exc->name();
      
      // Demangle it using obscure G++ ABI.
      // We may succeed or not. If we don't succeed,
      // we'll just end up looking at the mangled name.
      demangled_name = abi::__cxa_demangle(name, 
                                           0, 
                                           0, 
                                           &demangle_status);
      
      // If the exception was derived from std::exception, we can
      // gather a bit more information.
      try 
        { 
          throw; 
        }
      catch(std::exception &err)
        { 
          exception_what_msg = err.what();
        }
      catch(...) 
        {

          // Nope, wasn't derived from std::exception.
        }
                  
      // Print name of offending exception.
      std::cerr << "ERROR: Unhandled/unexpected C++ exception of type: "
                << (char *) (demangle_status == 0 ? demangled_name : name)
                << '.'
                << std::endl; 
      
      // Print the string associated with the offending exception,
      // if the exception was derived from std::exception. 
      if(exception_what_msg)
        {
          std::cerr << (char *)(demangle_status == 0 ? demangled_name : name)
                    << ".what() = "
                    << exception_what_msg
                    << '.'
                    << std::endl; 
        }
      
      // If we succeded demangling we need to free the buffer.
      if(demangle_status == 0)
        std::free(demangled_name);
      
    } 
  
  // This is a terminate_handler case, when terminate_handler gets called 
  // because of an attempt to rethrow when there is no suitable exception.
  else
    {
      std::cerr << "ERROR: C++ exception failure without an active exception."
                << std::endl;
    }
#else  
  
  // The not-so-verbose error reporter.
  std::cerr << "ERROR: Exception-handling-failure."
                 << std::endl;
#endif 
  
  // Die.
  MPI::COMM_WORLD.Abort (EXIT_FAILURE); 
}

int
main (int argc, char **argv)
{ 
#if defined(__GNUC__) && defined(__i386__) 

  // Horrible hack to align the stack to a 16-byte boundary.
  //
  // We assume a gcc version >= 2.95 so that
  // -mpreferred-stack-boundary works.  Otherwise, all bets are
  // off.  However, -mpreferred-stack-boundary does not create a
  // stack alignment, but it only preserves it.  Unfortunately,
  // many versions of libc on linux call main() with the wrong
  // initial stack alignment, with the result that the code is now
  // pessimally aligned instead of having a 50% chance of being
  // correct. (borrowed from aligned-main.c from fftw-3.0.1)
  {
    // Use alloca to allocate some memory on the stack.
    // This alerts gcc that something funny is going
    // on, so that it does not omit the frame pointer
    // etc.
    (void) __builtin_alloca (16);

    // Now align the stack pointer.
    __asm__ __volatile__ ("andl $-16, %esp");
  }
#endif

  // Set up exception handlers. (C++ stuff).
  std::set_terminate((std::terminate_handler)exc_handler);
  std::set_unexpected((std::unexpected_handler)exc_handler);

  // Force FFTW2 to allocate memory that is fftw_complex-aligned.
  // This might result in a performance boost as reading unaligned
  // data is slow. 
  fftw_malloc_hook = &fftw_complex_aligned_malloc;
  fftw_free_hook = &free;

  // Perform MPI initialization.
  MPI::Init (argc, argv);

  // We're going to set handlers for all the normal termination 
  // signals, as we want graceful shutdown. 
  struct sigaction new_sig_act;
  
  // The handler on signal.
  new_sig_act.sa_handler = sig_handler;
  
  // No signals blocked.
  sigemptyset(&new_sig_act.sa_mask);  
  
  // No special options.
  new_sig_act.sa_flags = 0;              
  
  // Set the handlers to handle typical termination signals.
  if(sigaction(SIGHUP, 
               &new_sig_act, 
               NULL) == -1 ||
     sigaction(SIGINT, 
               &new_sig_act, 
               NULL) == -1 ||
     sigaction(SIGQUIT, 
               &new_sig_act, 
               NULL) == -1 ||
     sigaction(SIGTERM,
               &new_sig_act, 
               NULL) == -1)
    {
      std::cerr << "ERROR: Could not install the signal handlers."
                << std::endl;
      MPI::COMM_WORLD.Abort (EXIT_FAILURE); 
    }

  // Print banner. Display usage information only if we are the primary 
  // process in our communicator group.
  if (MPI::COMM_WORLD.Get_rank () == 0)
    std::cout << "pstool - power spectrum calculation tool." << std::endl
              << "Copyright (C) 2005 Andrey Warkentin. Licensed under GPL v2." << std::endl;

  int c;
  opterr = 0;
  double sample_rate = 0;
  bool help_flag = false,	// Show help information?
    optimum_plan = false,	// Have RealFFT create an optimal plan?
    sample_flag = false;	// Have we been passed a sample rate for the data?
  char *input_data_file_name = NULL,	      // Input data file name.
    *export_spectrum_file_name = NULL,	      // Output data file name. (used for exporting power spectrum).
    *export_wisdom_file_name = NULL,	      // File name for RealFFT wisdom export.
    *import_wisdom_file_name = NULL,	      // File name for RealFFT wisdom import.
    *export_realfft_results_file_name = NULL; // File name for RealFFT results export.

  // Get command line parameters.
  while ((c = getopt (argc, argv, "e:hi:o:s:t:w:")) != -1)
    switch (c)
      {
      case 'e':

	// We will want to export FFTW2 wisdom to a file
	optimum_plan = true;
	export_wisdom_file_name = optarg;
	break;
      case 'h':

	// Show help information.
	help_flag = true;
	break;
      case 'i':

	// Set input data file name.
	input_data_file_name = optarg;
	break;
      case 'o':
        
	// Set output data file name.
	export_spectrum_file_name = optarg;
	break;
      case 's':

	// Yes, we were passed a sample rate.
	sample_flag = true;

	// Parse the sample rate parameter.
	// Convert from base-10.
	char *strtod_end;
	sample_rate = std::strtod (optarg, &strtod_end);

	// Make sure we have non-garbage input.
	if ((*strtod_end != '\0') ||
	    (sample_rate <= 0) ||
	    (sample_rate == HUGE_VAL) || (sample_rate == NAN))
	  {

	    // No need to print this more than once.
	    // So have the primary process in the
	    // communicator group do it.
	    if (MPI::COMM_WORLD.Get_rank () == 0)
	      std::cerr << "ERROR: Invalid sample rate passed." << std::endl;
	    MPI::Finalize ();
	    exit (-1);
	  }
	break;
      case 't':
        
	// We will save the results produced by RealFFT::do_transform to a file.
	export_realfft_results_file_name = optarg;
	break;
      case 'w':

	// We will want to import wisdom prior to creating our plan.
	import_wisdom_file_name = optarg;
	break;
      default:

	// Show help information if passed an unrecognised option.
	help_flag = true;
	break;
      }

  // Make sure we were executed correctly. We need the input and output file names,
  // as well as the sample rate.
  if ((input_data_file_name == NULL) ||
      (export_spectrum_file_name == NULL) || !sample_flag)
    help_flag = true;
        
  // Display usage information only if we are the primary process in our
  // communicator group.
  if (help_flag)
    {
      if (MPI::COMM_WORLD.Get_rank () == 0)
	std::cerr << "Usage: " << argv[0] 
                  << " [-e <file>] [-h] -i <file> -o <file> -s <sample rate> [-t <file>] [-w <file>]"  << std::endl 
                  << "\t-e\t- Save wisdom for RFFT plan creation to <file>." <<  std::endl 
                  << "\t-h\t- Show this helpful information." << std::endl 
                  << "\t-i\t- Set input data file name to <file>." << std::endl 
                  << "\t-o\t- Set output data file name to <file>." << std::endl
                  << "\t-s\t- Set sample rate of input data to <sample rate> Hz." << std::endl 
                  << "\t-t\t- Save results of RFFT to <file>." << std::endl 
                  << "\t-w\t- Import wisdom for RFFT plan creation from <file>." << std::endl;
      MPI::Finalize ();
      exit (-1);
    }

  try
  {

    // Create the input data object.
    MPIRFFTWInput input_data (input_data_file_name);

    // Create the transform object. Calculate how much and what data to read.
    RealFFT transform (optimum_plan, input_data, import_wisdom_file_name);

    // Read the appropriate data.
    input_data.read_data (transform);

    // Execute transform.
    transform.do_transform ();
    
    // Only find the power spectrum, write it out, write out the results of the
    // transformation and save the FFTW2 wisdom if we are the primary
    // process in our communicator group, since we are the only process with
    // the actual results of the transformation, the computed spectrum and
    // the wisdom.
    if (MPI::COMM_WORLD.Get_rank () == 0)
      {

	// Find the power spectrum.
	PSGenerator power_spectrum (transform, sample_rate);

	// Write out power spectrum to disk.
	power_spectrum.export_spectrum (export_spectrum_file_name);

        // Write out the results of the transformation to disk if we need to. 
        transform.export_transformed (export_realfft_results_file_name);
        
        // Save wisdom if we need to. 
        transform.export_wisdom (export_wisdom_file_name);
      }
  }
  catch (GenericException & err)
  {
    std::cerr << "ERROR: Process #"
      << MPI::COMM_WORLD.Get_rank ()
      << ": " << err.what () << "." << std::endl;
    MPI::COMM_WORLD.Abort (EXIT_FAILURE);
  }

  // Finish.
  MPI::Finalize ();
  return 0;
}
