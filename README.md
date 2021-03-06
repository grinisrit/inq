# INQ

[Xavier Andrade](mailto:xavier@llnl.gov) (LLNL), [Alfredo A. Correa](mailto:correaa@llnl.gov) (LLNL)

INQ is an engine for electronic structure calculations.
It can work in three ways, as a standalone electronic structure code, as a library to implement complex electronic structure methods, or a proxy-app to evaluate the performance of electronic structure algorithms in high-performance computing platforms.
It concentrates on algorithms as a portability hardware layer, generic types and flexibility of usage.

INQ is based on density functional theory (DFT).
It can calculate ground state properties in DFT, and also excited states using time-dependent DFT (TDDFT) in real-time and linear-response.
It implements different approximations for the exchange and correlation part: semi-local functionals like LDAs, GGAs and metaGGAs, and hybrid functionals that are implemented through the ACE approach for fast execution.

One key feature of INQ is that is designed from the ground up to work in modern high-performance computing platforms.
It has support for GPU parallelization (through different frameworks), thread parallelization and distributed memory parallelization using MPI (and Nvidia NCCL).
The thread parallelization is designed so that different tasks whithin the DFT approach are performed simultaneously, this achieves better scalability with respect to data parallelization alone.
INQ can perform calculations with unpolarized, polarized and non-colinear spin.

INQ attempts to be as agnostic as possible with the representation used for the states and other quantities that appear in DFT.
It uses plane waves by default, but other representations like real space will be available.

In order to keep the code simple, INQ is designed as modular as possible, it uses libraries for most of the operations that can be done independently.
Some libraries, like pseudopod (for pseudopotential parsing) or multi (for multidimensional arrays) are developed by INQ authors.
Other are written by third parties, like libxc (for exchange correlation functionals).
In the future, some other parts of INQ might be split into independent library.

This is a list of libraries INQ depends on:

* boost-multi: for multidimensional arrays (developed by A. A. Correa)
* pseudopod: for pseudopotential parsing (developed X. Andrade)
* boost-mpi3: for distributed memory parallelization (developed by A. A. Correa)
* libxc: for exchange correlation functionals (X. Andrade is a contributor)
* cuda: for GPU parallelization
* pcg-cpp: for random number generation
* catch2: for unit testing and validation
* blas/lapack: for linear algebra
* slate: for parallel linear algebra (work in progress)
* spglib: for symmetries and k-point generation (work in progress)
* spdlog: Logging messages (different systems can have different log sinks)

INQ is work in progress, some of the features are not well tested or are not available at the moment.

## Components

* Electronic states, complex and real fields
* Solvers
* Diagonalization
* CPU/GPU generic algorithms
* Parallel distribution MPI/GPU/threads

## Basic installation

Have dependencies and the environment ready in your system, for example in a desktop:
```
sudo apt install libblas-dev liblapack-dev libfftw3-dev
```

Instructions for compiling

```bash 
git clone --recurse-submodules git@gitlab.com:npneq/inq.git
cd inq
mkdir build
cd build
# set up environment if necessary, e.g. export CUDACXX=/usr/local/cuda/bin/nvcc
../configure --prefix=$HOME # change prefix, e.g. $HOME/.local or /usr/local (needs root access), if necessary,
make -j
make install
make test
```

This instructions might be incomplete for your particular setup; 
see other possibilities in [`.gitlab-ci.yml`](https://gitlab.com/npneq/inq/blob/master/.gitlab-ci.yml).

## Reference

- [INQ, a modern GPU-accelerated computational framework for (time-dependent) density functional theory, `arXiv:2106.03872 [cond-mat.mtrl-sci]` (2021)](https://arxiv.org/abs/2106.03872)

## Release information 

INQ is licensed under the terms of the [LGPL v3 License](/COPYING).

``LLNL-CODE-803817``

CP Number: CP02279
Software Title: Electronic Structure Engine
Date Submitted: Wednesday, December 11, 2019
Date Accepted: Wednesday, January 29, 2020

> The work was supported by the Center for Non-Perturbative Studies of Functional Materials Under Non-Equilibrium Conditions (NPNEQ) funded by the Computational Materials Sciences Program of the US Department of Energy, Office of Science, Basic Energy Sciences, Materials Sciences and Engineering  Division.
> This work was performed under the auspices of the U.S. Department of Energy by Lawrence Livermore National Laboratory under Contract DE-AC52-07NA27344.
