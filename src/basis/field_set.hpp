/* -*- indent-tabs-mode: t -*- */

#ifndef BASIS_FIELD_SET
#define BASIS_FIELD_SET

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

#include <multi/array.hpp>

namespace basis {
	
	template<class basis_type, class type>
  class field_set : public boost::multi::array<type, 2>{

  public:

		typedef type value_type;
		typedef boost::multi::array<type, 2> base;
		
    field_set(const basis_type & basis, const int num_vectors):
			boost::multi::array<type, 2>({basis.size(), num_vectors}),
			num_vectors_(num_vectors),
			basis_(basis){
    }

		field_set(const field_set & coeff) = delete;
		field_set(field_set && coeff) = default;
		field_set & operator=(const field_set & coeff) = delete;
		field_set & operator=(field_set && coeff) = default;

		//set to a scalar value
		field_set & operator=(const value_type value) {
			//DATAOPERATIONS
			for(int ii = 0; ii < basis_.size()*num_vectors_; ii++) this->data()[ii] = value;
			return *this;
		}
		
		const basis_type & basis() const {
			return basis_;
		}

		const int & set_size() const {
			return num_vectors_;
		}
		
		auto cubic() const {
			return this->partitioned(basis_.sizes()[1]*basis_.sizes()[0]).partitioned(basis_.sizes()[0]);
		}

		auto cubic() {
			return this->partitioned(basis_.sizes()[1]*basis_.sizes()[0]).partitioned(basis_.sizes()[0]);
		}

	private:

		int num_vectors_;
		basis_type basis_;

  };

}

#ifdef UNIT_TEST

#include <ions/unitcell.hpp>
#include <catch2/catch.hpp>

TEST_CASE("Class basis::field_set", "[basis::field_set]"){
  
  using namespace Catch::literals;
  using math::d3vector;
  
  double ecut = 40.0;

  ions::UnitCell cell(d3vector(10.0, 0.0, 0.0), d3vector(0.0, 4.0, 0.0), d3vector(0.0, 0.0, 7.0));
  basis::real_space rs(cell, input::basis::cutoff_energy(ecut));

	basis::field_set<basis::real_space, double> ff(rs, 12);

	namespace fftw = boost::multi::fftw;
	using boost::multi::array_ref;

	REQUIRE(sizes(rs)[0] == 28);
	REQUIRE(sizes(rs)[1] == 11);
	REQUIRE(sizes(rs)[2] == 20);	

	REQUIRE(std::get<0>(sizes(ff)) == 6160);	
	REQUIRE(std::get<1>(sizes(ff)) == 12);

	REQUIRE(std::get<0>(sizes(ff.cubic())) == 28);
	REQUIRE(std::get<1>(sizes(ff.cubic())) == 11);
	REQUIRE(std::get<2>(sizes(ff.cubic())) == 20);
	REQUIRE(std::get<3>(sizes(ff.cubic())) == 12);
	
}

#endif

#endif
