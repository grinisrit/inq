/* -*- indent-tabs-mode: t -*- */

#ifndef INQ__OPERATIONS__PRECONDITIONER
#define INQ__OPERATIONS__PRECONDITIONER

/*
 Copyright (C) 2019 Xavier Andrade, Alfredo Correa.

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

#include <basis/field.hpp>
#include <basis/fourier_space.hpp>
#include <operations/space.hpp>
#include <states/orbital_set.hpp>

#include <cstdlib>

#include <utils/profiling.hpp>

#include <operations/overlap_diagonal.hpp>

namespace inq {
namespace operations {

class preconditioner {
	
	// Implements the preconditioner of Teter, Payne and Allan, Phys. Rev. B, 40 12255 (1989)
	
public:
	
	GPU_FUNCTION static auto k_function(double x) {

		// The original function of the TPA paper
		auto num = ((8.0*x + 12.0)*x + 18.0)*x + 27.0;
		auto den = (((16.0*x + 8.0)*x + 12.0)*x + 18.0)*x + 27.0;

		/* 
		// A better version by Zhou, Chelikowsky, Gao and Zhou, Commun. Comput. Phys. 18 167 (2015)

		// I won't use it for the moment, since it doesn't seem to
		// change the results and it might have numerical issues. XA

		auto num = ((((32.0*x + 48.0)*x + 72.0)*x + 108.0)*x + 162)*x + 243.0;
		auto den = (((((64.0*x + 32.0)*x + 48.0)*x + 72.0)*x + 108.0)*x + 162)*x + 243.0;
			
		*/
			
		return num/den;
	}
		
	template <class type>
	void operator()(states::orbital_set<basis::fourier_space, type> & phi) const {

		CALI_MARK_BEGIN("preconditioner_reduction");
		
		auto kinetic = operations::overlap_diagonal_normalized(laplacian(phi.fields(), -0.5), phi.fields());

		CALI_MARK_END("preconditioner_reduction");
		
		{

			CALI_CXX_MARK_SCOPE("preconditioner_apply");
			
			//DATAOPERATIONS GPU::RUN 4D
			gpu::run(phi.set_size(), phi.basis().local_sizes()[2], phi.basis().local_sizes()[1], phi.basis().local_sizes()[0], 
							 [kine = begin(kinetic),
								phcub = begin(phi.cubic()),
								point_op = phi.basis().point_op()] GPU_LAMBDA
							 (auto ist, auto iz, auto iy, auto ix){
								 auto lapl = -0.5*(-point_op.g2(ix, iy, iz));
								 phcub[ix][iy][iz][ist] = k_function(lapl/real(kine[ist]))*phcub[ix][iy][iz][ist];
							 });
		}
		
	}

	template <class type>
	void operator()(states::orbital_set<basis::real_space, type> & phi) const {
			
		auto fphi = operations::space::to_fourier(phi);
		operator()(fphi);
		phi = operations::space::to_real(fphi);

	}


private:

};
	
}
}

#ifdef INQ_OPERATIONS_PRECONDITIONER_UNIT_TEST
#undef INQ_OPERATIONS_PRECONDITIONER_UNIT_TEST

#include <catch2/catch_all.hpp>

TEST_CASE("function operations::precondition", "[precondition]") {

	using namespace inq;
	using namespace Catch::literals;

}


#endif

#endif
