// Time-stamp: <2005-02-13 22:37:53 andyw>
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

#ifndef GENERIC_EXCEPTION_H
#define GENERIC_EXCEPTION_H

// System includes.
#include <string>

// This is a generic exception class.
// It should be never used directly, but instead be derived from.
class GenericException
{
private:

  // The error message associated with the exception.
  std::string error;
public:

  // Constructor used for creation of object.
  GenericException (const std::string & msg):error (msg)
  {
  }

  // Destructor used for destruction of object.
   ~GenericException ()
  {
  }

  // Returns the message associated with the exception
  // as an STL string.
  std::string what ()
  {
    return error;
  }
};

#endif
