/* -*- indent-tabs-mode: t -*- */

#ifndef INQ__UTILS__GET_REMOTE_POINTS
#define INQ__UTILS__GET_REMOTE_POINTS

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

#include <utils/raw_pointer_cast.hpp>
#include <basis/field.hpp>
#include <basis/field_set.hpp>

#include <mpi3/communicator.hpp>

#include <cstdlib> //drand48

namespace inq {
namespace operations {

struct remote_points_table {

	struct point_position {
		long point;
		long position;
	};
	
	std::vector<std::vector<point_position>> points_needed;
	int total_needed;
	int total_requested;
	math::array<int, 1> list_points_requested;
	math::array<int, 1> list_sizes_requested;
	math::array<int, 1> list_sizes_needed;	
	math::array<int, 1> list_needed;
	math::array<int, 1> list_displs_needed;
	math::array<int, 1> list_displs_requested;	
	
	template <class BasisType, class ArrayType>
	remote_points_table(BasisType & basis, ArrayType const & point_list){

		CALI_CXX_MARK_FUNCTION;

		auto const num_proc = basis.comm().size();

		// create a list per processor of the points we need
		points_needed.resize(num_proc);
		
		for(long ilist = 0; ilist < long(point_list.size()); ilist++){
			auto point = point_list[ilist];
			assert(point >= 0);
			assert(point < basis.size());

			auto src_proc = basis.part().location(point);
			assert(src_proc >= 0 and src_proc < num_proc); 
			points_needed[src_proc].push_back(point_position{point, ilist});
		}

		// find out how many points each processor requests from us
		list_sizes_needed.reextent(num_proc);
		list_sizes_requested.reextent(num_proc);	
	
		total_needed = 0;
		for(int iproc = 0; iproc < num_proc; iproc++){
			list_sizes_needed[iproc] = points_needed[iproc].size();
			total_needed += list_sizes_needed[iproc];
		}

		assert(total_needed == long(point_list.size()));

		MPI_Alltoall(raw_pointer_cast(list_sizes_needed.data_elements()), 1, MPI_INT, raw_pointer_cast(list_sizes_requested.data_elements()), 1, MPI_INT, basis.comm().get());

		// get the list of points each processor requests from us and send a list of the points we need
		list_needed.reextent(total_needed);
		list_displs_needed.reextent(num_proc);
		list_displs_requested.reextent(num_proc);
	
		total_requested = 0;

		basis.comm().barrier();
	
		for(int iproc = 0; iproc < num_proc; iproc++){

			total_requested += list_sizes_requested[iproc];
		
			if(iproc > 0){
				list_displs_needed[iproc] = list_displs_needed[iproc - 1] + list_sizes_needed[iproc - 1];
				list_displs_requested[iproc] = list_displs_requested[iproc - 1] + list_sizes_requested[iproc - 1];			
			} else {
				list_displs_needed[iproc] = 0;
				list_displs_requested[iproc] = 0;
			}

		}

		basis.comm().barrier();
	
		for(int iproc = 0; iproc < num_proc; iproc++){

			basis.comm().barrier();
		
			for(long ip = 0; ip < list_sizes_needed[iproc]; ip++) {
				list_needed[list_displs_needed[iproc] + ip] = points_needed[iproc][ip].point;
			}
		}

		list_points_requested.reextent(total_requested);
	
		MPI_Alltoallv(raw_pointer_cast(list_needed.data_elements()), raw_pointer_cast(list_sizes_needed.data_elements()), raw_pointer_cast(list_displs_needed.data_elements()), MPI_INT,
									raw_pointer_cast(list_points_requested.data_elements()), raw_pointer_cast(list_sizes_requested.data_elements()), raw_pointer_cast(list_displs_requested.data_elements()), MPI_INT, basis.comm().get());
		
	}

};

template <class BasisType, class ElementType, class ArrayType>
math::array<ElementType, 1> get_remote_points(basis::field<BasisType, ElementType> const & source, ArrayType const & point_list){

	CALI_CXX_MARK_FUNCTION;

	auto const num_proc = source.basis().comm().size();
 
	if(num_proc == 1){
		math::array<ElementType, 1> remote_points(point_list.size());
		
		gpu::run(point_list.size(),
						 [rem = begin(remote_points), sou = begin(source.linear()), poi = begin(point_list)] GPU_LAMBDA (auto ip){
							 rem[ip] = sou[poi[ip]];
						 });
		
		return remote_points;
	}
	
	remote_points_table rp(source.basis(), point_list);
	
	// send the value of the request points
	math::array<ElementType, 1> value_points_requested(rp.total_requested);
	math::array<ElementType, 1> value_points_needed(rp.total_needed);

	gpu::run(rp.total_requested,
					 [par =  source.basis().part(), lreq = begin(rp.list_points_requested), vreq = begin(value_points_requested), sou = begin(source.linear())] GPU_LAMBDA (auto ip){
						 auto iplocal = par.global_to_local(utils::global_index(lreq[ip]));
						 vreq[ip] = sou[iplocal];
					 });
	
	auto mpi_type = boost::mpi3::detail::basic_datatype<ElementType>();
	
	MPI_Alltoallv(raw_pointer_cast(value_points_requested.data_elements()), raw_pointer_cast(rp.list_sizes_requested.data_elements()), raw_pointer_cast(rp.list_displs_requested.data_elements()), mpi_type,
								raw_pointer_cast(value_points_needed.data_elements()), raw_pointer_cast(rp.list_sizes_needed.data_elements()), raw_pointer_cast(rp.list_displs_needed.data_elements()), mpi_type, source.basis().comm().get());

	// Finally copy the values to the return array in the proper order
	math::array<ElementType, 1> remote_points(point_list.size());

	long ip = 0;
	for(int iproc = 0; iproc < num_proc; iproc++){
		for(long jp = 0; jp < long(rp.points_needed[iproc].size()); jp++) {
			remote_points[rp.points_needed[iproc][jp].position] = value_points_needed[ip];
			ip++;
		}
	}

	assert(ip == long(point_list.size()));
	
  return remote_points;
}

template <class BasisType, class ElementType, class ArrayType>
math::array<ElementType, 2> get_remote_points(basis::field_set<BasisType, ElementType> const & source, ArrayType const & point_list){

	CALI_CXX_MARK_FUNCTION;

	auto const nset = source.local_set_size();
	auto const num_proc = source.basis().comm().size();
 
	if(num_proc == 1){
		math::array<ElementType, 2> remote_points({point_list.size(), nset});
		
		gpu::run(nset, point_list.size(),
						 [rem = begin(remote_points), sou = begin(source.matrix()), poi = begin(point_list)] GPU_LAMBDA (auto ist, auto ip){
							 rem[ip][ist] = sou[poi[ip]][ist];
						 });
		
		return remote_points;
	}
	
	remote_points_table rp(source.basis(), point_list);
	
	// send the value of the request points
	math::array<ElementType, 2> value_points_requested({rp.total_requested, nset});
	math::array<ElementType, 2> value_points_needed({rp.total_needed, nset});

	gpu::run(nset, rp.total_requested,
					 [par =  source.basis().part(), lreq = begin(rp.list_points_requested), vreq = begin(value_points_requested), sou = begin(source.matrix())] GPU_LAMBDA (auto iset, auto ip){
						 auto iplocal = par.global_to_local(utils::global_index(lreq[ip]));
						 vreq[ip][iset] = sou[iplocal][iset];
					 });
	
	MPI_Datatype mpi_type;
	MPI_Type_contiguous(nset, boost::mpi3::detail::basic_datatype<ElementType>(), &mpi_type);
	MPI_Type_commit(&mpi_type);
	
	MPI_Alltoallv(raw_pointer_cast(value_points_requested.data_elements()), raw_pointer_cast(rp.list_sizes_requested.data_elements()), raw_pointer_cast(rp.list_displs_requested.data_elements()), mpi_type,
								raw_pointer_cast(value_points_needed.data_elements()), raw_pointer_cast(rp.list_sizes_needed.data_elements()), raw_pointer_cast(rp.list_displs_needed.data_elements()), mpi_type, source.basis().comm().get());

	MPI_Type_free(&mpi_type);
	
	// Finally copy the values to the return array in the proper order
	math::array<ElementType, 2> remote_points({point_list.size(), nset});

	long ip = 0;
	for(int iproc = 0; iproc < num_proc; iproc++){
		for(long jp = 0; jp < long(rp.points_needed[iproc].size()); jp++) {
				for(long iset = 0; iset < nset; iset++) remote_points[rp.points_needed[iproc][jp].position][iset] = value_points_needed[ip][iset];
			ip++;
		}
	}

	assert(ip == long(point_list.size()));
	
  return remote_points;
}

}
}

#ifdef INQ_OPERATIONS_GET_REMOTE_POINTS_UNIT_TEST
#undef INQ_OPERATIONS_GET_REMOTE_POINTS_UNIT_TEST

#include <math/vector3.hpp>

#include <catch2/catch_all.hpp>
#include <mpi3/cartesian_communicator.hpp>

TEST_CASE("Class operations::get_remote_points", "[operations::get_remote_points]"){

	using namespace inq;
	using namespace inq::magnitude;	
	using namespace Catch::literals;
	using Catch::Approx;
	using math::vector3;

  boost::mpi3::cartesian_communicator<2> cart_comm(boost::mpi3::environment::get_world_instance(), {});
	auto set_comm = cart_comm.axis(0);
	auto basis_comm = cart_comm.axis(1);	

	systems::box box = systems::box::orthorhombic(13.3_b, 6.55_b, 8.02_b).cutoff_energy(20.0_Ha);
  basis::real_space rs(box, cart_comm);

	int const nvec = 19;
	
  basis::field<basis::real_space, complex> test_field(rs);
  basis::field_set<basis::real_space, complex> test_field_set(rs, 19);

  for(long ip = 0; ip < rs.local_size(); ip++){
    auto ipg = rs.part().local_to_global(ip);
    test_field.linear()[ip] = complex(ipg.value(), 0.1*ipg.value());
    for(int ivec = 0; ivec < nvec; ivec++) test_field_set.matrix()[ip][ivec] = (ivec + 1.0)*complex(ipg.value(), 0.1*ipg.value());		
  }

	srand48(500 + 34895783*cart_comm.rank());
	
	long const npoints = drand48()*rs.size();

	math::array<long, 1> list(npoints);
	
	for(long ip = 0; ip < npoints; ip++){
		list[ip] = drand48()*(rs.size() - 1);
		assert(list[ip] < rs.size());
	}

	auto remote_points = operations::get_remote_points(test_field, list);
	auto remote_points_set = operations::get_remote_points(test_field_set, list);	

	for(long ip = 0; ip < npoints; ip++){
		CHECK(double(list[ip]) == Approx(real(remote_points[ip])));
		CHECK(0.1*double(list[ip]) == Approx(imag(remote_points[ip])));
		for(int ivec = 0; ivec < nvec; ivec++){
			CHECK((ivec + 1.0)*double(list[ip]) == Approx(real(remote_points_set[ip][ivec])));
			CHECK((ivec + 1.0)*0.1*double(list[ip]) == Approx(imag(remote_points_set[ip][ivec])));
		}
	}

	
}


#endif
#endif


