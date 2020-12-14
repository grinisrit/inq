/* -*- indent-tabs-mode: t -*- */

#ifndef INQ__UTILS__PROFILING
#define INQ__UTILS__PROFILING

/*
 Copyright (C) 2020 Xavier Andrade

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
  
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
  
 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifdef ENABLE_CALIPER
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#else
#include <kalimotxo/cali.h>
#endif

#ifdef INQ_UTILS_PROFILING_UNIT_TEST
#undef INQ_UTILS_PROFILING_UNIT_TEST

#endif
#endif
