/**
 * @file main.cpp
 * @brief Main routine of svZeroDSolver
 */
#include "algebra/densesystem.hpp"
#include "algebra/integrator.hpp"
#include "algebra/sparsesystem.hpp"
#include "algebra/state.hpp"
#include "helpers/debug.hpp"
#include "helpers/endswith.hpp"
#include "io/configreader.hpp"
#include "io/csvwriter.hpp"
#include "io/jsonwriter.hpp"
#include "model/model.hpp"

typedef double T;

template <typename TT>
using S = ALGEBRA::SparseSystem<TT>;

/**
 *
 * @brief svZeroDSolver main routine
 *
 * This is the main routine of the svZeroDSolver. It exectutes the following
 * steps:
 *
 * 1. Read the input file
 * 2. Create the 0D model
 * 3. (Optional) Solve for steady initial condition
 * 4. Run simulation
 * 5. Write output to file
 *
 * @param argc Number of command line arguments
 * @param argv Command line arguments
 * @return Return code
 */
int main(int argc, char *argv[]) {
  DEBUG_MSG("Starting svZeroDSolver");

  // Get input and output file name
  if (argc != 3) {
    std::cout << "Usage: svzerodsolver path/to/config.json path/to/output.json"
              << std::endl;
    exit(1);
  }
  std::string input_file = argv[1];
  std::string output_file = argv[2];
  DEBUG_MSG("Reading configuration from " << input_file);

  // Create configuration reader
  IO::ConfigReader<T> config(input_file);

  // Create model
  DEBUG_MSG("Creating model");
  auto model = config.get_model();
  DEBUG_MSG("Size of system:      " << model.dofhandler.size());

  // Get simulation parameters
  DEBUG_MSG("Setup simulutation");
  T time_step_size = config.get_time_step_size();
  DEBUG_MSG("Time step size:      " << time_step_size);
  int num_time_steps = config.get_num_time_steps();
  DEBUG_MSG("Number of timesteps: " << num_time_steps);
  T absolute_tolerance =
      config.get_scalar_simulation_parameter("absolute_tolerance", 1e-8);
  int max_nliter =
      config.get_int_simulation_parameter("maximum_nonlinear_iterations", 30);
  int output_interval =
      config.get_int_simulation_parameter("output_interval", 1);
  bool steady_initial =
      config.get_bool_simulation_parameter("steady_initial", true);
  bool output_mean_only =
      config.get_bool_simulation_parameter("output_mean_only", false);

  // Setup system
  DEBUG_MSG("Starting simulation");
  ALGEBRA::State<T> state = ALGEBRA::State<T>::Zero(model.dofhandler.size());

  // Create steady initial
  if (steady_initial) {
    DEBUG_MSG("Calculating steady initial condition");
    T time_step_size_steady = config.cardiac_cycle_period / 10.0;
    auto model_steady = config.get_model();
    model_steady.to_steady();
    ALGEBRA::Integrator<T, S> integrator_steady(model_steady,
                                                time_step_size_steady, 0.1,
                                                absolute_tolerance, max_nliter);
    for (size_t i = 0; i < 31; i++) {
      state = integrator_steady.step(state, time_step_size_steady * T(i),
                                     model_steady);
    }
  }

  ALGEBRA::Integrator<T, S> integrator(model, time_step_size, 0.1,
                                       absolute_tolerance, max_nliter);

  std::vector<ALGEBRA::State<T>> states;
  std::vector<T> times;
  states.reserve(num_time_steps);
  times.reserve(num_time_steps);

  T time = 0.0;

  states.push_back(state);
  times.push_back(time);

  int interval_counter = 0;
  for (size_t i = 1; i < num_time_steps; i++) {
    state = integrator.step(state, time, model);
    interval_counter += 1;
    time = time_step_size * T(i);
    if (interval_counter == output_interval) {
      times.push_back(time);
      states.push_back(std::move(state));
      interval_counter = 0;
    }
  }
  DEBUG_MSG("Simulation completed");

  if (HELPERS::endswith(output_file, ".csv")) {
    DEBUG_MSG("Saving csv result file to " << output_file);
    IO::write_csv<T>(output_file, times, states, model, output_mean_only);
  } else if (HELPERS::endswith(output_file, ".json")) {
    DEBUG_MSG("Saving json result file to " << output_file);
    IO::write_json<T>(output_file, times, states, model);
  } else {
    throw std::runtime_error("Unsupported outfile file format.");
  }

  return 0;
}
