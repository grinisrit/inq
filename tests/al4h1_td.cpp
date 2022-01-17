/* -*- indent-tabs-mode: t -*- */

/*
 Copyright (C) 2019 Xavier Andrade

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

#include <inq/inq.hpp>

int main(int argc, char ** argv){

	using namespace inq;
	using namespace inq::magnitude;
	
	input::environment env(argc, argv);
	
	utils::match energy_match(1.0e-5);

	double alat = 7.6524459;
	
	std::vector<input::atom> cell;

	cell.push_back( "Al" | alat*math::vector3<double>(0.0, 0.0, 0.0));
	cell.push_back( "Al" | alat*math::vector3<double>(0.0, 0.5, 0.5));
	cell.push_back( "Al" | alat*math::vector3<double>(0.5, 0.0, 0.5));
	cell.push_back( "Al" | alat*math::vector3<double>(0.5, 0.5, 0.0));	
	cell.push_back( "H"  | alat*math::vector3<double>(0.1, 0.2, 0.3));
	
	systems::box box = systems::box::orthorhombic(alat*1.0_b, alat*1.0_b, alat*1.0_b).cutoff_energy(30.0_Ha);
	systems::ions ions(box, cell);
	
	input::config conf;

	conf.extra_states = 1;
	conf.temperature = 300.0_K;

	auto par = env.dist();
	if(par.size() != 5) par = par.kpoints().domains(1);
	
	systems::electrons electrons(par, ions, box, conf, input::kpoints::grid({2, 2, 2}, true));
	
	electrons.load("al4h1_restart");

	auto result = real_time::propagate<>(ions, electrons, input::interaction::dft(), input::rt::num_steps(30) | input::rt::dt(0.055_atomictime));

	energy_match.check("energy step   0", result.energy[0],   -9.798687545590);
	energy_match.check("energy step  10", result.energy[10],  -9.798687882908);
	energy_match.check("energy step  20", result.energy[20],  -9.798688620193);
	energy_match.check("energy step  30", result.energy[30],  -9.798688818773);
	
	return energy_match.fail();
	
}

