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
// clang-format off
/**
 * @file main.hpp
 * @brief Main page documentation
 *
 * @mainpage svZeroDSolver
 *
 * \tableofcontents
 *
 * The svZeroDSolver is a fast simulation tool for modeling the hemodynamics of
 * vascular networks using zero-dimensional (0D) lumped parameter models.
 *
 * * <a href="https://github.com/StanfordCBCL/svZeroDPlus">Source
 * repository</a>
 * * <a href="https://simvascular.github.io">About SimVascular</a>
 *
 * \f[
 * \begin{circuitikz} \draw
 * node[left] {$Q_{in}$} [-latex] (0,0) -- (0.8,0);
 * \draw (1,0) node[anchor=south]{$P_{in}$}
 * to [R, l=$R$, *-] (3,0) node[anchor=south] {$P_{C}$}
 * (3,0) to [L, l=$L$, *-*] (5,0)
 * node[anchor=south]{$P_{out}$}
 * (3,0) to [C, l=$C$, *-] (3,-1.5)
 * node[ground]{};
 * \draw [-latex] (5.2,0) -- (6,0) node[right] {$Q_{out}$};
 * \end{circuitikz}
 * \f]
 *
 * # Background
 *
 * Zero-dimensional (0D) models
 * are lightweight methods to simulate bulk hemodynamic quantities in the
 * cardiovascular system. Unlike 3D and 1D models, 0D models are purely
 * time-dependent; they are unable to simulate spatial patterns in the
 * hemodynamics. 0D models are analogous to electrical circuits. The flow rate
 * simulated by 0D models represents electrical current, while the pressure
 * represents voltage. Three primary building blocks of 0D models are resistors,
 * capacitors, and inductors Resistance captures the viscous effects of blood
 * flow, capacitance represents the compliance and distensibility of the vessel
 * wall, and inductance represents the inertia of the blood flow. Different
 * combinations of these building blocks, as well as others, can be formed to
 * reflect the hemodynamics and physiology of different cardiovascular
 * anatomies. These 0D models are governed by differential algebraic equations
 * (DAEs).
 *
 * # Architecture
 *
 * ## Model
 *
 * The solver uses a highly modular framework to model the vascular anatomy,
 * using individual classes to represent different 0D elements of the
 * model. The elements are part of the MODEL namespace. Currently
 * supported elements are:
 * 
 * #### Vessels
 *
 * * MODEL::BloodVessel: RCL blood vessel with optional stenosis.
 * 
 * #### Junctions
 * 
 * * MODEL::Junction: Mass conservation junction element.
 * * MODEL::ResistiveJunction: Resistive junction element.
 * * MODEL::BloodVesselJunction: RCL junction element with optional stenoses.
 * 
 * #### Boundary conditions
 * 
 * * MODEL::FlowReferenceBC: Prescribed flow boundary condition.
 * * MODEL::PressureReferenceBC: Prescribed pressure boundary condition.
 * * MODEL::ResistanceBC: Resistance boundary condition.
 * * MODEL::WindkesselBC: RCR Windkessel boundary condition.
 * * MODEL::OpenLoopCoronaryBC: Open Loop coronary boundary condition.
 * * MODEL::ClosedLoopCoronaryBC: Closed Loop coronary boundary condition.
 * * MODEL::ClosedLoopHeartPulmonary: Heart and pulmonary circulation model.
 *
 * The elements are based on the parent MODEL::Block class. More information
 * about the elements can be found on their respective pages. The elements are
 * connected to each other via nodes (see MODEL::Node). Each node corresponds to
 * a flow and a pressure value of the model. The MODEL::DOFHandler handles the
 * degrees-of-freedom (DOF) of the system by assigning DOF indices to each
 * element that determine the location of the local element contribution in the
 * global system. The complete model is stored and managed by the MODEL::Model
 * class.
 *
 * ## Algebra
 *
 * Everything related to solving the system of equation is contained in the
 * ALGEBRA namespace. The ALGEBRA::Integrator
 * handles the time integration scheme (see Integrator documentation for
 * more information on that).
 *
 * ## Input/output
 *
 * Finally the IO namespace provides everything related to reading and writing
 * files. The IO::ConfigReader reads the simulation config and IO::to_vessel_csv and
 * IO::to_variable_csv are two different methods for writing the result files.
 *
 * # Build svZeroDSolver
 *
 * svZeroDSolver uses CMake to build the tool. The dependencies are installed
 * by CMake. Currently supported are:
 * 
 * * Ubuntu 18
 * * Ubuntu 20
 * * Ubuntu 22
 * * macOS Big Sur
 * * macOS Monterey
 * 
 * @note If you want to use the Python interface of svZeroDPlus make
 * sure to **activate the correct Python environment** before building the
 * solver. Also make sure to install the Python package using the `-e` options,
 * i.e.
 * \code
 * pip install -e .
 * \endcode
 *
 * ## Build in debug mode
 *
 * \code
 * mkdir Debug
 * cd Debug
 * cmake -DCMAKE_BUILD_TYPE=Debug ..
 * cmake --build .
 * \endcode
 *
 * ## Build in release mode
 * \code
 * mkdir Release
 * cd Release
 * cmake -DCMAKE_BUILD_TYPE=Release ..
 * cmake --build .
 * \endcode
 * 
 * ## Build on Sherlock
 * \code
 * module load cmake/3.23.1 gcc/12.1.0 binutils/2.38
 * mkdir Release
 * cd Release
 * cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/share/software/user/open/gcc/12.1.0/bin/g++ -DCMAKE_C_COMPILER=/share/software/user/open/gcc/12.1.0/bin/gcc ..
 * cmake --build .
 * \endcode
 *
 * # Run svZeroDSolver
 *
 * After building svZeroDSolver, the build folder contains an executable
 * called `svzerodsolver`. Run it with one of:
 *
 * \code
 * ./svzerodsolver path/to/config.json path/to/output.csv
 * \endcode
 *
 * `path/to/config.json` and `path/to/output.csv` should be replaced by the
 * correct paths to the input and output file, respectively.
 *
 * ## Simulation parameters
 *
 * The svZeroDSolver can be configured with the following options in the
 * `simulation_parameters` section of the input file. Parameters without a
 * default value must be specified.
 * 
 *
 * Parameter key                           | Description                               | Default value
 * --------------------------------------- | ----------------------------------------- | ----------- 
 * number_of_cardiac_cycles                | Number of cardiac cycles to simulate      | -
 * number_of_time_pts_per_cardiac_cycle    | Number of time steps per cardiac cycle    | -
 * absolute_tolerance                      | Absolute tolerance for time integration   | \f$10^{-8}\f$
 * maximum_nonlinear_iterations            | Maximum number of nonlinear iterations for time integration | \f$30\f$
 * steady_initial                          | Toggle whether to use the steady solution as the initial condition for the simulation | true
 * output_variable_based                   | Output solution based on variables (i.e. flow+pressure at nodes and internal variables) | false
 * output_interval                         | The frequency of writing timesteps to the output (1 means every time step is written to output) | \f$1\f$
 * output_mean_only                        | Write only the mean values over every timestep to output file | false
 * output_derivative                       | Write time derivatives to output file | false
 * output_last_cycle_only                  | Write only the last cardiac cycle | false
 * 
 * 
 * # Developer Guide
 * 
 * If you are a developer and want to contribute to svZeroDSolver, you can find
 * more helpful information in our [Developer Guide](docs/cpp/pages/developer_guide.md).
 * 
 */
// clang-format on
#include "algebra/integrator.hpp"
#include "algebra/state.hpp"
#include "helpers/debug.hpp"
#include "helpers/endswith.hpp"
#include "io/configreader.hpp"
#include "io/csvwriter.hpp"
#include "io/jsonhandler.hpp"
#include "model/model.hpp"

// Setting scalar type to double
typedef double T;

/**
 *
 * @brief Run svZeroDSolver with configuration
 *
 * 1. Read the input file
 * 2. Create the 0D model
 * 3. (Optional) Solve for steady initial condition
 * 4. Run simulation
 * 5. Write output to file
 *
 * @param handler Handler for the json style configuration
 * @return Result as csv encoded string
 */
const std::string run(IO::JsonHandler handler) {
  // Load model and configuration
  DEBUG_MSG("Read configuration");
  auto simparams = IO::load_simulation_params<T>(handler);
  auto model = IO::load_model<T>(handler);
  auto state = IO::load_initial_condition<T>(handler, model.get());

  // Check that steady initial is not set when ClosedLoopHeartAndPulmonary is
  // used
  if ((simparams.sim_steady_initial == true) &&
      (model->get_block("CLH") != nullptr)) {
    std::runtime_error(
        "ERROR: Steady initial condition is not compatible with "
        "ClosedLoopHeartAndPulmonary block.");
  }

  // Set default cardiac cycle period if not set by model
  if (model->cardiac_cycle_period < 0.0) {
    model->cardiac_cycle_period =
        1.0;  // If it has not been read from config or Parameter
              // yet, set as default value of 1.0
  }

  // Calculate time step size
  if (!simparams.sim_coupled) {
    simparams.sim_time_step_size =
        model->cardiac_cycle_period / (T(simparams.sim_pts_per_cycle) - 1.0);
  } else {
    simparams.sim_time_step_size = simparams.sim_external_step_size /
                                   (T(simparams.sim_num_time_steps) - 1.0);
  }

  // Create steady initial
  if (simparams.sim_steady_initial) {
    DEBUG_MSG("Calculate steady initial condition");
    T time_step_size_steady = model->cardiac_cycle_period / 10.0;
    model->to_steady();
    ALGEBRA::Integrator<T> integrator_steady(model.get(), time_step_size_steady,
                                             0.1, simparams.sim_abs_tol,
                                             simparams.sim_nliter);
    for (int i = 0; i < 31; i++) {
      state = integrator_steady.step(state, time_step_size_steady * T(i));
    }
    model->to_unsteady();
  }

  // Set-up integrator
  DEBUG_MSG("Setup time integration");
  ALGEBRA::Integrator<T> integrator(model.get(), simparams.sim_time_step_size,
                                    0.1, simparams.sim_abs_tol,
                                    simparams.sim_nliter);

  // Initialize loop
  std::vector<ALGEBRA::State<T>> states;
  std::vector<T> times;
  if (simparams.output_last_cycle_only) {
    int num_states =
        simparams.sim_pts_per_cycle / simparams.output_interval + 1;
    states.reserve(num_states);
    times.reserve(num_states);
  } else {
    int num_states =
        simparams.sim_num_time_steps / simparams.output_interval + 1;
    states.reserve(num_states);
    times.reserve(num_states);
  }
  T time = 0.0;

  // Run integrator
  DEBUG_MSG("Run time integration");
  int interval_counter = 0;
  int start_last_cycle =
      simparams.sim_num_time_steps - simparams.sim_pts_per_cycle;
  if ((simparams.output_last_cycle_only == false) || (0 >= start_last_cycle)) {
    times.push_back(time);
    states.push_back(std::move(state));
  }
  for (int i = 1; i < simparams.sim_num_time_steps; i++) {
    // DEBUG_MSG("Integration time: "<< time << std::endl);
    state = integrator.step(state, time);
    interval_counter += 1;
    time = simparams.sim_time_step_size * T(i);
    if ((interval_counter == simparams.output_interval) ||
        (simparams.output_last_cycle_only && (i == start_last_cycle))) {
      if ((simparams.output_last_cycle_only == false) ||
          (i >= start_last_cycle)) {
        times.push_back(time);
        states.push_back(std::move(state));
      }
      interval_counter = 0;
    }
  }

  // Make times start from 0
  if (simparams.output_last_cycle_only) {
    T start_time = times[0];
    for (auto& time : times) {
      time -= start_time;
    }
  }

  // Write csv output string
  DEBUG_MSG("Write output");
  std::string output;
  if (simparams.output_variable_based) {
    output = IO::to_variable_csv<T>(times, states, model.get(),
                                    simparams.output_mean_only,
                                    simparams.output_derivative);
  } else {
    output = IO::to_vessel_csv<T>(times, states, model.get(),
                                  simparams.output_mean_only,
                                  simparams.output_derivative);
  }
  DEBUG_MSG("Finished");
  return output;
}
