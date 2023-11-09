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
 * @file Integrator.h
 * @brief Integrator source file
 */
#ifndef SVZERODSOLVER_ALGEBRA_INTEGRATOR_HPP_
#define SVZERODSOLVER_ALGEBRA_INTEGRATOR_HPP_

#include <Eigen/Dense>

#include "Model.h"
#include "State.h"

/**
 * @brief Generalized-alpha integrator
 *
 * This class handles the time integration scheme for solving 0D blood
 * flow system using the generalized-\f$\alpha\f$ method \cite JANSEN2000305.
 *
 * We are interested in solving the DAE system defined in SparseSystem for the
 * solutions, \f$\mathbf{y}_{n+1}\f$ and \f$\dot{\mathbf{y}}_{n+1}\f$, at the
 * next time, \f$t_{n+1}\f$, using the known solutions, \f$\mathbf{y}_{n}\f$
 * and \f$\dot{\mathbf{y}}_{n}\f$, at the current time, \f$t_{n}\f$. Note that
 * \f$t_{n+1} = t_{n} + \Delta t\f$, where \f$\Delta t\f$ is the time step
 * size.
 *
 * Using the generalized-\f$\alpha\f$ method, we launch a predictor step
 * and a series of multi-corrector steps to solve for \f$\mathbf{y}_{n+1}\f$
 * and \f$\dot{\mathbf{y}}_{n+1}\f$. Similar to other predictor-corrector
 * schemes, we evaluate the solutions at intermediate times between \f$t_{n}\f$
 * and \f$t_{n + 1}\f$. However, in the generalized-\f$\alpha\f$ method, we
 * evaluate \f$\mathbf{y}\f$ and \f$\dot{\mathbf{y}}\f$ at different
 * intermediate times. Specifically, we evaluate \f$\mathbf{y}\f$ at
 * \f$t_{n+\alpha_{f}}\f$ and \f$\dot{\mathbf{y}}\f$ at
 * \f$t_{n+\alpha_{m}}\f$, where \f$t_{n+\alpha_{f}} = t_{n} +
 * \alpha_{f}\Delta t\f$ and \f$t_{n+\alpha_{m}} = t_{n} + \alpha_{m}\Delta
 * t\f$. Here, \f$\alpha_{m}\f$ and \f$\alpha_{f}\f$ are the
 * generalized-\f$\alpha\f$ parameters, where \f$\alpha_{m} = \frac{3 - \rho}{2
 * + 2\rho}\f$ and \f$\alpha_{f} = \frac{1}{1 + \rho}\f$. In the 0D solver, we
 * set the spectral radius, \f$\rho\f$, to be \f$0.1\f$. For each time step, the
 * procedure works as follows.
 *
 * 1. \f$\textbf{Predictor step}\f$: First, we make an initial guess for
 * \f$\mathbf{y}_{n+1}\f$ and \f$\dot{\mathbf{y}}_{n+1}\f$,
 * \f[
 * \mathbf{y}_{n+1} = \mathbf{y}_{n},\\
 * \dot{\mathbf{y}}_{n+1} = \frac{\gamma - 1}{\gamma}\dot{\mathbf{y}}_{n},
 * \f]
 * where \f$\gamma = 0.5 + \alpha_{m} - \alpha_{f}\f$.
 *
 * 2. \f$\textbf{Initiator step}\f$: Then, we initialize the values of
 * \f$\dot{\mathbf{y}}_{n+\alpha_{m}}\f$ and
 * \f$\mathbf{y}_{n+\alpha_{f}}\f$,
 * \f[\dot{\mathbf{y}}_{n+\alpha_{m}}^{k=0} = \dot{\mathbf{y}}_{n} +
 * \alpha_{m}\left(\dot{\mathbf{y}}_{n+1} - \dot{\mathbf{y}}_{n}\right),\\
 * \mathbf{y}_{n+\alpha_{f}}^{k=0} = \mathbf{y}_{n} +
 * \alpha_{f}\left(\mathbf{y}_{n+1} - \mathbf{y}_{n}\right).\f]
 *
 * 3. \f$\textbf{Multi-corrector step}\f$: Then, for \f$k \in \left[0, N_{int}
 * - 1\right]\f$, we iteratively update our guess of
 * \f$\dot{\mathbf{y}}_{n+\alpha_{m}}^{k}\f$ and
 * \f$\mathbf{y}_{n+\alpha_{f}}^{k}\f$. We desire the residual,
 * \f$\textbf{r}\left(\dot{\mathbf{y}}_{n+\alpha_{m}}^{k + 1},
 * \mathbf{y}_{n+\alpha_{f}}^{k + 1}, t_{n+\alpha_{f}}\right)\f$, to be
 * \f$\textbf{0}\f$. We solve this system using Newton's method. For details,see
 * SparseSystem.
 *
 * 4. \f$\textbf{Update step}\f$: Finally, we update \f$\mathbf{y}_{n+1}\f$ and
 * \f$\dot{\mathbf{y}}_{n+1}\f$ using our final value of
 * \f$\dot{\mathbf{y}}_{n+\alpha_{m}}\f$ and
 * \f$\mathbf{y}_{n+\alpha_{f}}\f$. \f[
 * \mathbf{y}_{n+1} = \mathbf{y}_{n} +
 * \frac{\mathbf{y}_{n+\alpha_{f}}^{N_{int}} -
 * \mathbf{y}_{n}}{\alpha_{f}},\\ \dot{\mathbf{y}}_{n+1} =
 * \dot{\mathbf{y}}_{n} + \frac{\dot{\mathbf{y}}_{n+\alpha_{m}}^{N_{int}} -
 * \dot{\mathbf{y}}_{n}}{\alpha_{m}} \f]
 *
 */
class Integrator {
 private:
  double alpha_m{0.0};
  double alpha_f{0.0};
  double gamma{0.0};
  double time_step_size{0.0};
  double ydot_init_coeff{0.0};
  double y_coeff{0.0};
  double y_coeff_jacobian{0.0};
  double atol{0.0};
  int max_iter{0};
  int size{0};
  int n_iter{0};
  int n_nonlin_iter{0};
  Eigen::Matrix<double, Eigen::Dynamic, 1> y_af;
  Eigen::Matrix<double, Eigen::Dynamic, 1> ydot_am;
  SparseSystem system;
  Model* model{nullptr};

 public:
  /**
   * @brief Construct a new Integrator object
   *
   * @param model The model to simulate
   * @param time_step_size Time step size for generalized-alpha step
   * @param rho Spectral radius for generalized-alpha step
   * @param atol Absolut tolerance for non-linear iteration termination
   * @param max_iter Maximum number of non-linear iterations
   */
  Integrator(Model* model, double time_step_size, double rho, double atol,
             int max_iter);

  /**
   * @brief Construct a new Integrator object
   *
   */
  Integrator();

  /**
   * @brief Destroy the Integrator object
   *
   */
  ~Integrator();

  /**
   * @brief Delete dynamically allocated memory (in class member
   * SparseSystem<double> system).
   */
  void clean();

  /**
   * @brief Update integrator parameter and system matrices with model parameter
   * updates.
   *
   * @param time_step_size Time step size for 0D model
   */
  void update_params(double time_step_size);

  /**
   * @brief Perform a time step
   *
   * @param state Current state
   * @param time Current time
   * @return New state
   */
  State step(const State& state, double time);

  /**
   * @brief Get average number of nonlinear iterations in all step calls
   *
   */
  double avg_nonlin_iter();
};

#endif  // SVZERODSOLVER_ALGEBRA_INTEGRATOR_HPP_
