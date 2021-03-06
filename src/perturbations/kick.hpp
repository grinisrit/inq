/* -*- indent-tabs-mode: t -*- */

#ifndef INQ__PERTURBATIONS__KICK
#define INQ__PERTURBATIONS__KICK

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

#include <inq_config.h>

#include <math/vector3.hpp>
#include <basis/real_space.hpp>
#include <basis/field_set.hpp>

namespace inq {
namespace perturbations {

void kick(math::vector3<double, math::covariant> kick_field, basis::field_set<basis::real_space, complex> & phi){

	//DATAOPERATIONS LOOP 4D
	for(int ix = 0; ix < phi.basis().local_sizes()[0]; ix++){
		for(int iy = 0; iy < phi.basis().local_sizes()[1]; iy++){
			for(int iz = 0; iz < phi.basis().local_sizes()[2]; iz++){
				
				auto ixg = phi.basis().cubic_dist(0).local_to_global(ix);
				auto iyg = phi.basis().cubic_dist(1).local_to_global(iy);
				auto izg = phi.basis().cubic_dist(2).local_to_global(iz);
					
				auto rr = phi.basis().point_op().rvector(ixg, iyg, izg);
				
				auto kick_factor = exp(complex(0.0, dot(kick_field, rr)));
				
				for(int ist = 0; ist < phi.set_part().local_size(); ist++){
					phi.cubic()[ix][iy][iz][ist] *= kick_factor;
				}
				
			}
		}
	}
}
}
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#ifdef INQ_PERTURBATIONS_KICK_UNIT_TEST
#undef INQ_PERTURBATIONS_KICK_UNIT_TEST

#include <catch2/catch_all.hpp>
#include <basis/real_space.hpp>
#include <ions/unitcell.hpp>

TEST_CASE("perturbations::kick", "[perturbations::kick]") {

	using namespace inq;
	using namespace inq::magnitude;
	using namespace Catch::literals;
	using math::vector3;
	
	const int nvec = 12;

	auto ecut = 31.2_Ha;
	double phi_absdif = 0.0;
	double phi_dif = 0.0;

	systems::box box = systems::box::orthorhombic(4.2_b, 3.5_b, 6.4_b).cutoff_energy(ecut);
	
	basis::real_space bas(box);

	basis::field_set<basis::real_space, complex> phi(bas, nvec);

	//Construct a field
	for(int ix = 0; ix < phi.basis().local_sizes()[0]; ix++){
		for(int iy = 0; iy < phi.basis().local_sizes()[1]; iy++){
			for(int iz = 0; iz < phi.basis().local_sizes()[2]; iz++){
				for(int ist = 0; ist < phi.set_part().local_size(); ist++){
					phi.cubic()[ix][iy][iz][ist] = complex(cos(ist+(ix+iy+iz)), 1.3*sin(ist+(cos(ix-iy-iz))));
				}
			}
		}
	}

	auto phi_old = phi;

	perturbations::kick({0.1, 0.0, 0.0}, phi);

	for(int ix = 0; ix < phi.basis().local_sizes()[0]; ix++){
		for(int iy = 0; iy < phi.basis().local_sizes()[1]; iy++){
			for(int iz = 0; iz < phi.basis().local_sizes()[2]; iz++){
				for(int ist = 0; ist < phi.set_part().local_size(); ist++){
					phi_absdif += norm(phi.cubic()[ix][iy][iz][ist]) - norm(phi_old.cubic()[ix][iy][iz][ist]);
					phi_dif += norm(phi.cubic()[ix][iy][iz][ist] - phi_old.cubic()[ix][iy][iz][ist]);
				}
			}
		}
	}

	//Kick should not change the phi absolute value - kick pulse change only the phase of a wave fucntion in the frame of TDDFTThe kick should not change the phi absolute value - kick pulse change only the phase of a wave function in the frame of TDDFT

	using Catch::Approx;

	CHECK(phi_absdif == Approx(0).margin(1.0e-9));
	//The wave function should changes after applying a kick potetntial
	CHECK(phi_dif > 1.0e-9);

}

#endif
#endif
