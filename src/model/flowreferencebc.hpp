// Copyright (c) Stanford University, The Regents of the University of
//               California, and others.
//
// All Rights Reserved.
//
// See Copyright-SimVascular.txt for additional details.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject
// to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/**
 * @file flowreferencebc.hpp
 * @brief MODEL::FlowReferenceBC source file
 */
#ifndef SVZERODSOLVER_MODEL_FLOWREFERENCEBC_HPP_
#define SVZERODSOLVER_MODEL_FLOWREFERENCEBC_HPP_

#include "../algebra/densesystem.hpp"
#include "../algebra/sparsesystem.hpp"
#include "block.hpp"
#include "timedependentparameter.hpp"

namespace MODEL {

/**
 * @brief Flow reference boundary condition.
 *
 * Applies a prescribed flow to a boundary.
 *
 * \f[
 * \begin{circuitikz} \draw
 * node[left] {$\hat{Q}$} [-latex] (0,0) -- (0.8,0);
 * \draw (1,0) node[anchor=south]{$P$} to [short, *-] (1.2,0) ;
 * \draw [-latex] (1.4,0) -- (2.2,0) node[right] {$Q$};
 * \end{circuitikz}
 * \f]
 *
 * ### Governing equations
 *
 * \f[
 * Q=\hat{Q}
 * \f]
 *
 * ### Local contributions
 *
 * \f[
 * \mathbf{y}^{e}=\left[\begin{array}{ll}P^{e} & Q^{e}\end{array}\right]^{T}
 * \f]
 *
 * \f[
 * \mathbf{F}^{e}=\left[\begin{array}{ll}0 & 1\end{array}\right]
 * \f]
 *
 * \f[
 * \mathbf{C}^{e}=\left[\hat{Q}\right]
 * \f]
 *
 * @tparam T Scalar type (e.g. `float`, `double`)
 */
template <typename T>
class FlowReferenceBC : public Block<T> {
 public:
  /**
   * @brief Parameters of the element.
   *
   * Struct containing all constant and/or time-dependent parameters of the
   * element.
   */
  struct Parameters : public Block<T>::Parameters {
    TimeDependentParameter<T> Q;  ///< Time-dependent flow
  };

  /**
   * @brief Construct a new FlowReferenceBC object
   *
   * @param Q Time dependent flow
   * @param name Name
   */
  FlowReferenceBC(TimeDependentParameter<T> Q, std::string name);

  /**
   * @brief Destroy the FlowReferenceBC object
   *
   */
  ~FlowReferenceBC();

  /**
   * @brief Set up the degrees of freedom (DOF) of the block
   *
   * Set \ref global_var_ids and \ref global_eqn_ids of the element based on the
   * number of equations and the number of internal variables of the
   * element.
   *
   * @param dofhandler Degree-of-freedom handler to register variables and
   * equations at
   */
  void setup_dofs(DOFHandler &dofhandler);

  /**
   * @brief Update the constant contributions of the element in a dense system
   *
   * @param system System to update contributions at
   */
  void update_constant(ALGEBRA::DenseSystem<T> &system);

  /**
   * @brief Update the time-dependent contributions of the element in a dense
   * system
   *
   * @param system System to update contributions at
   * @param time Current time
   */
  void update_time(ALGEBRA::DenseSystem<T> &system, T time);

  /**
   * @brief Update the constant contributions of the element in a sparse system
   *
   * @param system System to update contributions at
   */
  void update_constant(ALGEBRA::SparseSystem<T> &system);

  /**
   * @brief Update the time-dependent contributions of the element in a sparse
   * system
   *
   * @param system System to update contributions at
   * @param time Current time
   */
  void update_time(ALGEBRA::SparseSystem<T> &system, T time);

  /**
   * @brief Number of triplets of element
   *
   * Number of triplets that the element contributes to the global system
   * (relevant for sparse memory reservation)
   */
  std::map<std::string, int> num_triplets = {
      {"F", 1},
      {"E", 0},
      {"D", 0},
  };

  /**
   * @brief Convert the block to a steady behavior
   *
   * Converts the prescribed flow to the constant mean of itself
   *
   */
  void to_steady();

 private:
  Parameters params;
};

template <typename T>
FlowReferenceBC<T>::FlowReferenceBC(TimeDependentParameter<T> Q,
                                    std::string name)
    : Block<T>(name) {
  this->name = name;
  this->params.Q = Q;
}

template <typename T>
FlowReferenceBC<T>::~FlowReferenceBC() {}

template <typename T>
void FlowReferenceBC<T>::setup_dofs(DOFHandler &dofhandler) {
  Block<T>::setup_dofs_(dofhandler, 1, {});
}

template <typename T>
void FlowReferenceBC<T>::update_constant(ALGEBRA::DenseSystem<T> &system) {
  system.F(this->global_eqn_ids[0], this->global_var_ids[1]) = 1.0;
}

template <typename T>
void FlowReferenceBC<T>::update_time(ALGEBRA::DenseSystem<T> &system, T time) {
  system.C(this->global_eqn_ids[0]) = -params.Q.get(time);
}

template <typename T>
void FlowReferenceBC<T>::update_constant(ALGEBRA::SparseSystem<T> &system) {
  system.F.coeffRef(this->global_eqn_ids[0], this->global_var_ids[1]) = 1.0;
}

template <typename T>
void FlowReferenceBC<T>::update_time(ALGEBRA::SparseSystem<T> &system, T time) {
  system.C(this->global_eqn_ids[0]) = -params.Q.get(time);
}

template <typename T>
void FlowReferenceBC<T>::to_steady() {
  params.Q.to_steady();
}

}  // namespace MODEL

#endif  // SVZERODSOLVER_MODEL_FLOWREFERENCEBC_HPP_