/* -*- indent-tabs-mode: t -*- */

#ifndef INQ__OPERATIONS__SHIFT
#define INQ__OPERATIONS__SHIFT

/*
 Copyright (C) 2019 Xavier Andrade, Alfredo A. Correa.

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

#include <gpu/run.hpp>
#include <basis/field_set.hpp>
#include <cassert>

namespace inq {
namespace operations {

template <class array_1d, class FieldSetType1, class FieldSetType2>
void shift(double scale, const array_1d & factor, const FieldSetType1 & shift, FieldSetType2 & phi){

	CALI_CXX_MARK_SCOPE("shift");
	
	assert(size(factor) == phi.set_part().local_size());

	gpu::run(phi.set_part().local_size(), phi.basis().part().local_size(),
					 [factorp = begin(factor), shiftp = begin(shift.matrix()), phip = begin(phi.matrix()), scale]
					 GPU_LAMBDA (auto ist, auto ipoint){
						 phip[ipoint][ist] += scale*(factorp[ist]*shiftp[ipoint][ist]);
					 });
}

template <class FieldSetType1, class FieldSetType2>
void shift(typename FieldSetType1::element_type const & factor, const FieldSetType1 & shift, FieldSetType2 & phi){

	CALI_CXX_MARK_SCOPE("shift_const_factor");
	
	//this could be done with axpy
	gpu::run(phi.set_part().local_size(), phi.basis().part().local_size(),
					 [factor, shiftp = begin(shift.matrix()), phip = begin(phi.matrix())]
					 GPU_LAMBDA (auto ist, auto ipoint){
						 phip[ipoint][ist] += factor*shiftp[ipoint][ist];
					 });
}

}
}

#ifdef INQ_OPERATIONS_SHIFT_UNIT_TEST
#undef INQ_OPERATIONS_SHIFT_UNIT_TEST

#include <basis/trivial.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("function operations::shift", "[operations::shift]") {

	using namespace inq;
	using namespace Catch::literals;
	using Catch::Approx;

	const int npoint = 185193;
	const int nvec = 7;

	auto comm = boost::mpi3::environment::get_world_instance();
	
	boost::mpi3::cartesian_communicator<2> cart_comm(comm, {});

	auto basis_comm = cart_comm.axis(1);
	
	basis::trivial bas(npoint, basis_comm);
	
	SECTION("double"){
		
		basis::field_set<basis::trivial, double> aa(bas, nvec, cart_comm);
		basis::field_set<basis::trivial, double> bb(bas, nvec, cart_comm);

		math::array<double, 1> factor(aa.set_part().local_size());
		
		for(int jj = 0; jj < aa.set_part().local_size(); jj++){
			auto jjg = aa.set_part().local_to_global(jj);
			for(int ii = 0; ii < bas.part().local_size(); ii++){
				auto iig = bas.part().local_to_global(ii);
				aa.matrix()[ii][jj] = 1.0 + 0.765*iig.value()*jjg.value();
				bb.matrix()[ii][jj] = iig.value();
			}
			factor[jj] = 2.0*0.765*jjg.value();
		}

		operations::shift(-0.5, factor, bb, aa);
				
		for(int ii = 0; ii < bas.part().local_size(); ii++){
			for(int jj = 0; jj < aa.set_part().local_size(); jj++) CHECK(aa.matrix()[ii][jj] == Approx(1.0));
		}
	}
	
	SECTION("complex"){
		
		basis::field_set<basis::trivial, complex> aa(bas, nvec, cart_comm);
		basis::field_set<basis::trivial, complex> bb(bas, nvec, cart_comm);

		math::array<complex, 1> factor(aa.set_part().local_size());
		
		for(int jj = 0; jj < aa.set_part().local_size(); jj++){
			auto jjg = aa.set_part().local_to_global(jj);
			for(int ii = 0; ii < bas.part().local_size(); ii++){
				auto iig = bas.part().local_to_global(ii);
				aa.matrix()[ii][jj] = complex(iig.value(), 1.0 + 0.765*iig.value()*jjg.value());
				bb.matrix()[ii][jj] = iig.value();
			}
			factor[jj] = complex(0.0, 2.0*0.765*jjg.value());
		}

		operations::shift(-0.5, factor, bb, aa);
				
		for(int ii = 0; ii < bas.part().local_size(); ii++){
			auto iig = bas.part().local_to_global(ii);
			for(int jj = 0; jj < aa.set_part().local_size(); jj++) CHECK(real(aa.matrix()[ii][jj]) == Approx(iig.value()));
			for(int jj = 0; jj < aa.set_part().local_size(); jj++) CHECK(imag(aa.matrix()[ii][jj]) == Approx(1.0));
		}
	}	
	
	SECTION("mixed types"){
		
		basis::field_set<basis::trivial, complex> aa(bas, nvec, cart_comm);
		basis::field_set<basis::trivial, complex> bb(bas, nvec, cart_comm);

		math::array<double, 1> factor(aa.set_part().local_size());
		
		for(int jj = 0; jj < aa.set_part().local_size(); jj++){
			auto jjg = aa.set_part().local_to_global(jj);
			for(int ii = 0; ii < bas.part().local_size(); ii++){
				auto iig = bas.part().local_to_global(ii);
				aa.matrix()[ii][jj] = complex(iig.value(), 1.0 + 0.765*iig.value()*jjg.value());
				bb.matrix()[ii][jj] = complex(0.0, iig.value());
			}
			factor[jj] = 2.0*0.765*jjg.value();
		}

		operations::shift(-0.5, factor, bb, aa);
				
		for(int ii = 0; ii < bas.part().local_size(); ii++){
			for(int jj = 0; jj < aa.set_part().local_size(); jj++) {
				auto iig = bas.part().local_to_global(ii);
				CHECK(real(aa.matrix()[ii][jj]) == Approx(iig.value()));
				CHECK(imag(aa.matrix()[ii][jj]) == Approx(1.0));
			}
		}
	}
	
}


#endif

#endif
