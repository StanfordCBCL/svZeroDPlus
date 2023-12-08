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
 * @file ChamberKerckhoffs.h
 * @brief model::ChamberKH source file
 */
#ifndef SVZERODSOLVER_MODEL_CHAMBERKH_HPP_
#define SVZERODSOLVER_MODEL_CHAMBERKH_HPP_

#include <math.h>

#include "Block.h"
#include "SparseSystem.h"
#include "debug.h"

/**
 * @brief Chamber (Kerckhoffs, 2006) block.
 *
 * Models a chamber as a time-varying capacitor based on an elastance and a set of volumes.
 *
 * \f[
 * \begin{circuitikz} \draw
 * node[left] {$Q_{in}$} [-latex] (0,0) -- (0.8,0);
 * \draw (1,0) node[anchor=south]{$P_{in}$}
 * to (3,0)
 * node[anchor=south]{$P_{c}$}
 * to [L, l=$L$, *-*] (5,0)
 * node[anchor=south]{$P_{out}$}
 * (3,0) to [vC, l=$E$, *-] (3,-1.5)
 * node[ground]{};
 * \draw [-latex] (5.2,0) -- (6.0,0) node[right] {$Q_{out}$} ;
 * \end{circuitikz}
 * \f]
 *
 * ### Governing equations
 *
 * \f[
 * P_{in}-P_{out}-Q_{in}\left[R_{min} +
 * (R_{max}-R_{min})\frac{1}{2}\left[1+tanh\{k(P_{out}-P{in})\}\right]\right]=0
 * \f]
 *
 * \f[
 * Q_{in}-Q_{out}=0
 * \f]
 *
 * ### Local contributions
 *
 * \f[
 * \mathbf{y}^{e}=\left[\begin{array}{llll}P_{in} & Q_{in} &
 * P_{out} & Q_{out}\end{array}\right]^{T} \f]
 *
 * \f[
 * \mathbf{E}^{e}=\left[\begin{array}{cccc}
 * 0 & 0 & 0 & 0 \\
 * 0 & 0 & 0 & 0
 * \end{array}\right]
 * \f]
 *
 * \f[
 * \mathbf{F}^{e}=\left[\begin{array}{cccc}
 * 1 & -(R_{max}+R_{min})/2.0 & -1 & 0 \\
 * 0 &      1                 &  0 & -1
 * \end{array}\right]
 * \f]
 *
 * \f[
 * \mathbf{c}^{e}=\left[\begin{array}{c}
 * -\frac{1}{2}Q_{in}(R_{max}-R_{min})tanh\{k(P_{out}-P_{in})\} \\
 * 0
 * \end{array}\right]
 * \f]
 *
 * \f[
 * \left(\frac{\partial\mathbf{c}}{\partial\mathbf{y}}\right)^{e} =
 * \left[\begin{array}{cccc}
 * A & B & C & 0 \\
 * 0 & 0 & 0 & 0 \end{array}\right] \f]
 * where,
 * \f[
 * A = \frac{1}{2} k Q_{in}
 * (R_{max}-R_{min})\left[1-tanh^2\{k(P_{out}-P_{in})\}\right] \\
 * \f]
 * \f[
 * B = -\frac{1}{2}(R_{max}-R_{min})tanh\{k(P_{out}-P_{in})\} \\
 * \f]
 * \f[
 * C = -\frac{1}{2} k Q_{in}
 * (R_{max}-R_{min})\left[1-tanh^2\{k(P_{out}-P_{in})\}\right] \f]
 *
 * \f[
 * \left(\frac{\partial\mathbf{c}}{\partial\dot{\mathbf{y}}}\right)^{e} =
 * \left[\begin{array}{cccc}
 * 0 & 0 & 0 & 0 \\
 * 0 & 0 & 0 & 0
 * \end{array}\right]
 * \f]
 *
 * ### Parameters
 *
 * Parameter sequence for constructing this block
 *
 * * `0` Maximum (closed) valve resistance
 * * `1` Minimum (open) valve resistance
 * * `2` Steepness of sigmoid function
 *
 */
class ChamberKH : public Block {
 public:
  
  /**
   * @brief Construct a new BloodVessel object
   *
   * @param id Global ID of the block
   * @param model The model to which the block belongs
   */
  ChamberKH(int id, Model *model)
      : Block(id, model, BlockType::chamber_kerckhoffs, BlockClass::chamber,
              {{"Emax", InputParameter()},
               {"Emin", InputParameter()},
               {"Vrd", InputParameter()},
               {"Vrs", InputParameter()},
               {"t_active", InputParameter()},
               {"t_twitch", InputParameter()},
               {"Impedance", InputParameter()}}) {}
  
  /**
   * @brief Local IDs of the parameters
   *
   */
  enum ParamId {
    EMAX = 0,
    EMIN = 1,
    VRD = 2,
    VRS = 3,
    TACTIVE = 4,
    TTWITCH = 5,
    IMPEDANCE = 6
  };

//explicit ChamberKH(int id, const std::vector<int> &param_ids, Model *model)
//    : Block(id, param_ids, model){};

  /**
   * @brief Set up the degrees of freedom (DOF) of the block
   *
   * Set global_var_ids and global_eqn_ids of the element based on the
   * number of equations and the number of internal variables of the
   * element.
   *
   * @param dofhandler Degree-of-freedom handler to register variables and
   * equations at
   */
  void setup_dofs(DOFHandler &dofhandler);

  /**
   * @brief Update the constant contributions of the element in a sparse
   system
   *
   * @param system System to update contributions at
   * @param parameters Parameters of the model
   */
  void update_constant(SparseSystem &system, std::vector<double> &parameters);

  /**
   * @brief Update the time-dependent contributions of the element in a sparse
   * system
   *
   * @param system System to update contributions at
   * @param parameters Parameters of the model
   */
  void update_time(SparseSystem &system, std::vector<double> &parameters);

  /**
   * @brief Number of triplets of element
   *
   * Number of triplets that the element contributes to the global system
   * (relevant for sparse memory reservation)
   */
  TripletsContributions num_triplets{8, 2, 1};

  private:
  // Below variables change every timestep and are then combined with
  // expressions that are updated with solution
  double Elas;   // Chamber Elastance
  double Vrest;  // Rest Volume

  /**
   * @brief Update the elastance functions which depend on time
   *
   * @param parameters Parameters of the model
   */
  void get_elastance_values(std::vector<double> &parameters);
};

#endif  // SVZERODSOLVER_MODEL_CHAMBERKH_HPP_
