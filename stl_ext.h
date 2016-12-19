// Time-stamp: <2005-01-03 01:05:40 nodeuser>
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

#include <string>
#include <sstream>

// Converts Value to an std::string object.
template < class T > inline std::string to_string (const T & Value)
{
  std::stringstream streamOut;
  streamOut << Value;
  return streamOut.str ();
}
