/* -*- indent-tabs-mode: t -*- */

#ifndef INQ__GROUND_STATE__INITIAL_GUESS
#define INQ__GROUND_STATE__INITIAL_GUESS

#include <cfloat>

#include <systems/ions.hpp>
#include <operations/randomize.hpp>
#include <operations/orthogonalize.hpp>
#include <density/normalize.hpp>
#include <systems/electrons.hpp>

namespace inq {
namespace ground_state {
	
void initial_guess(const systems::ions & ions, systems::electrons & electrons){
  electrons.density_ = electrons.atomic_pot_.atomic_electronic_density(electrons.density_basis_, ions.cell(), ions.geo());

  density::normalize(electrons.density_, electrons.states_.num_electrons());

	int iphi = 0;
	for(auto & phi : electrons.lot()) {
		
		operations::randomize(phi.fields());
		operations::orthogonalize(phi.fields());
		for(long ist = 0; ist < phi.fields().local_set_size(); ist++) electrons.eigenvalues()[iphi][ist] = ist + phi.fields().set_part().start();

		iphi++;
	}
	
	electrons.update_occupations(electrons.eigenvalues());

}
}
}

#ifdef INQ_GROUND_STATE_INITIAL_GUESS_UNIT_TEST
#undef INQ_GROUND_STATE_INITIAL_GUESS_UNIT_TEST

#include <catch2/catch_all.hpp>

TEST_CASE("ground_state::initial_guess", "[ground_state::initial_guess]") {
	using namespace inq;
	using namespace Catch::literals;
	using Catch::Approx;
}

#endif
#endif

