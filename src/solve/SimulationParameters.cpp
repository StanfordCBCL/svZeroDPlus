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

#include "SimulationParameters.h"

#include "State.h"

std::vector<double> get_double_array(const nlohmann::json& data,
                                     const std::string_view& key) {
  std::vector<double> vector;
  if (!data[key].is_array()) {
    return {data[key]};
  }
  return data[key].get<std::vector<double>>();
}

std::vector<double> get_double_array(const nlohmann::json& data,
                                     const std::string_view& key,
                                     const std::vector<double>& default_value) {
  if (!data.contains(key)) {
    return default_value;
  }
  if (!data[key].is_array()) {
    return {data[key]};
  }
  return data[key].get<std::vector<double>>();
}

int generate_block(Model& model, const nlohmann::json& config,
                   const std::string& block_name, const std::string_view& name,
                   bool internal, bool periodic) {
  // Generate block from factory
  auto block = model.create_block(block_name);

  // Read block input parameters
  // todo: improve error checking
  std::vector<int> block_param_ids;
  int new_id;

  // Only blood_vessel_junction has lists of parameters
  if (block->block_type == BlockType::blood_vessel_junction) {
    for (const InputParameter& param : block->input_params) {
      for (double value : config[param.name]) {
        block_param_ids.push_back(model.add_parameter(value));
      }
    }
  } else {
    for (const InputParameter& param : block->input_params) {
      if (param.is_array) {
        // Get parameter vector
        std::vector<double> val;
        if (param.is_optional) {
          val = get_double_array(config, param.name, {param.default_val});
        } else {
          val = get_double_array(config, param.name);
        }

        // Get time vector
        std::vector<double> time = get_double_array(config, "t", {0.0});

        // Add parameters to model
        new_id = model.add_parameter(time, val, periodic);

      } else {
        // Get scalar parameter
        double val;
        if (param.is_optional) {
          val = config.value(param.name, param.default_val);
        } else {
          val = config[param.name];
        }

        // Add parameter to model
        new_id = model.add_parameter(val);
      }
      // Store parameter IDs
      block_param_ids.push_back(new_id);
    }
  }

  // Add block to model (with parameter IDs)
  return model.add_block(block, name, block_param_ids, internal);
}

/**
 * @brief Load the simulation parameters from a json configuration
 *
 * @param config The json configuration
 * @return SimulationParameters Simulation parameters read from configuration
 */
SimulationParameters load_simulation_params(const nlohmann::json& config) {
  // DEBUG_MSG("Loading simulation parameters");
  SimulationParameters sim_params;
  const auto& sim_config = config["simulation_parameters"];
  sim_params.sim_coupled = sim_config.value("coupled_simulation", false);

  if (!sim_params.sim_coupled) {
    sim_params.sim_num_cycles = sim_config["number_of_cardiac_cycles"];
    sim_params.sim_pts_per_cycle =
        sim_config["number_of_time_pts_per_cardiac_cycle"];
    sim_params.sim_num_time_steps =
        (sim_params.sim_pts_per_cycle - 1) * sim_params.sim_num_cycles + 1;
    sim_params.sim_external_step_size = 0.0;

  } else {
    sim_params.sim_num_cycles = 1;
    sim_params.sim_num_time_steps = sim_config["number_of_time_pts"];
    sim_params.sim_pts_per_cycle = sim_params.sim_num_time_steps;
    sim_params.sim_external_step_size =
        sim_config.value("external_step_size", 0.1);
  }
  sim_params.sim_abs_tol = sim_config.value("absolute_tolerance", 1e-8);
  sim_params.sim_nliter = sim_config.value("maximum_nonlinear_iterations", 30);
  sim_params.sim_steady_initial = sim_config.value("steady_initial", true);
  sim_params.output_variable_based =
      sim_config.value("output_variable_based", false);
  sim_params.output_interval = sim_config.value("output_interval", 1);
  sim_params.output_mean_only = sim_config.value("output_mean_only", false);
  sim_params.output_derivative = sim_config.value("output_derivative", false);
  sim_params.output_all_cycles = sim_config.value("output_all_cycles", false);
  // DEBUG_MSG("Finished loading simulation parameters");
  return sim_params;
}

/**
 * @brief Load model from a configuration
 *
 * @param config The json configuration
 */
void load_simulation_model(const nlohmann::json& config, Model& model) {
  // DEBUG_MSG("Loading model");

  // Create list to store block connections while generating blocks
  std::vector<std::tuple<std::string, std::string>> connections;

  // Create vessels
  // DEBUG_MSG("Load vessels");
  std::map<int, std::string> vessel_id_map;
  const auto& vessels = config["vessels"];

  for (size_t i = 0; i < vessels.size(); i++) {
    const auto& vessel_config = vessels[i];
    const auto& vessel_values = vessel_config["zero_d_element_values"];
    const std::string vessel_name = vessel_config["vessel_name"];
    vessel_id_map.insert({vessel_config["vessel_id"], vessel_name});

    generate_block(model, vessel_values, vessel_config["zero_d_element_type"],
                   vessel_name);

    // Read connected boundary conditions
    if (vessel_config.contains("boundary_conditions")) {
      const auto& vessel_bc_config = vessel_config["boundary_conditions"];
      if (vessel_bc_config.contains("inlet")) {
        connections.push_back({vessel_bc_config["inlet"], vessel_name});
      }
      if (vessel_bc_config.contains("outlet")) {
        connections.push_back({vessel_name, vessel_bc_config["outlet"]});
      }
    }
  }

  // Create map for boundary conditions to boundary condition type
  // DEBUG_MSG("Create BC name to BC type map");
  const auto& bc_configs = config["boundary_conditions"];
  std::map<std::string, std::string> bc_type_map;
  for (size_t i = 0; i < bc_configs.size(); i++) {
    const auto& bc_config = bc_configs[i];
    std::string bc_name = bc_config["bc_name"];
    std::string bc_type = bc_config["bc_type"];
    bc_type_map.insert({bc_name, bc_type});
  }

  // Create external coupling blocks
  if (config.contains("external_solver_coupling_blocks")) {
    // DEBUG_MSG("Create external coupling blocks");
    const auto& coupling_configs = config["external_solver_coupling_blocks"];
    for (const auto& coupling_config : coupling_configs) {
      std::string coupling_type = coupling_config["type"];
      std::string coupling_name = coupling_config["name"];
      std::string coupling_loc = coupling_config["location"];
      bool periodic = coupling_config.value("periodic", true);
      const auto& coupling_values = coupling_config["values"];

      generate_block(model, coupling_values, coupling_type, coupling_name);

      // Determine the type of connected block
      std::string connected_block = coupling_config["connected_block"];
      std::string connected_type;
      int found_block = 0;
      if (connected_block == "ClosedLoopHeartAndPulmonary") {
        connected_type = "ClosedLoopHeartAndPulmonary";
        found_block = 1;
      } else {
        try {
          connected_type = bc_type_map.at(connected_block);
          found_block = 1;
        } catch (...) {
        }
        if (found_block == 0) {
          // Search for connected_block in the list of vessel names
          for (auto const vessel : vessel_id_map) {
            if (connected_block == vessel.second) {
              connected_type = "BloodVessel";
              found_block = 1;
              break;
            }
          }
        }
        if (found_block == 0) {
          std::cout << "Error! Could not connected type for block: "
                    << connected_block << std::endl;
          throw std::runtime_error("Terminating.");
        }
      }  // connected_block != "ClosedLoopHeartAndPulmonary"
      // Create connections
      if (coupling_loc == "inlet") {
        std::vector<std::string> possible_types = {"RESISTANCE",
                                                   "RCR",
                                                   "ClosedLoopRCR",
                                                   "SimplifiedRCR",
                                                   "CORONARY",
                                                   "ClosedLoopCoronaryLeft",
                                                   "ClosedLoopCoronaryRight",
                                                   "BloodVessel"};
        if (std::find(std::begin(possible_types), std::end(possible_types),
                      connected_type) == std::end(possible_types)) {
          throw std::runtime_error(
              "Error: The specified connection type for inlet "
              "external_coupling_block is invalid.");
        }
        connections.push_back({coupling_name, connected_block});
        // DEBUG_MSG("Created coupling block connection: " << coupling_name <<
        // "->" << connected_block);
      } else if (coupling_loc == "outlet") {
        std::vector<std::string> possible_types = {
            "ClosedLoopRCR", "ClosedLoopHeartAndPulmonary", "BloodVessel"};
        if (std::find(std::begin(possible_types), std::end(possible_types),
                      connected_type) == std::end(possible_types)) {
          throw std::runtime_error(
              "Error: The specified connection type for outlet "
              "external_coupling_block is invalid.");
        }
        // Add connection only for closedLoopRCR and BloodVessel. Connection to
        // ClosedLoopHeartAndPulmonary will be handled in
        // ClosedLoopHeartAndPulmonary creation.
        if ((connected_type == "ClosedLoopRCR") ||
            (connected_type == "BloodVessel")) {
          connections.push_back({connected_block, coupling_name});
          // DEBUG_MSG("Created coupling block connection: " << connected_block
          // << "-> " << coupling_name);
        }  // connected_type == "ClosedLoopRCR"
      }    // coupling_loc
    }      // for (size_t i = 0; i < coupling_configs.length(); i++)
  }

  // Create boundary conditions
  std::vector<std::string> closed_loop_bcs;
  // DEBUG_MSG("Create boundary conditions");
  for (size_t i = 0; i < bc_configs.size(); i++) {
    const auto& bc_config = bc_configs[i];
    std::string bc_type = bc_config["bc_type"];
    std::string bc_name = bc_config["bc_name"];
    const auto& bc_values = bc_config["bc_values"];

    int block_id = generate_block(model, bc_values, bc_type, bc_name);

    // Keep track of closed loop blocks
    Block* block = model.get_block(block_id);

    if (block->block_type == BlockType::closed_loop_rcr_bc) {
      if (bc_values["closed_loop_outlet"] == true) {
        closed_loop_bcs.push_back(bc_name);
      }
    }
    if (block->block_class == BlockClass::closed_loop) {
      closed_loop_bcs.push_back(bc_name);
    }
  }

  // Create junctions
  for (const auto& junction_config : config["junctions"]) {
    std::string j_type = junction_config["junction_type"];
    std::string junction_name = junction_config["junction_name"];

    if (!junction_config.contains("junction_values")) {
      generate_block(model, {}, j_type, junction_name);
    } else {
      generate_block(model, junction_config["junction_values"], j_type,
                     junction_name);
    }

    // Check for connections to inlet and outlet vessels and append to
    // connections list
    for (int vessel_id : junction_config["inlet_vessels"]) {
      connections.push_back({vessel_id_map[vessel_id], junction_name});
    }
    for (int vessel_id : junction_config["outlet_vessels"]) {
      connections.push_back({junction_name, vessel_id_map[vessel_id]});
    }
    // DEBUG_MSG("Created junction " << junction_name);
  }

  // Create closed-loop blocks
  bool heartpulmonary_block_present =
      false;  ///< Flag to check if heart block is present (requires different
              ///< handling)
  if (config.contains("closed_loop_blocks")) {
    for (const auto& closed_loop_config : config["closed_loop_blocks"]) {
      std::string closed_loop_type = closed_loop_config["closed_loop_type"];
      if (closed_loop_type == "ClosedLoopHeartAndPulmonary") {
        if (heartpulmonary_block_present == false) {
          heartpulmonary_block_present = true;
          std::string heartpulmonary_name = "CLH";
          double cycle_period = closed_loop_config["cardiac_cycle_period"];
          if ((model.cardiac_cycle_period > 0.0) &&
              (cycle_period != model.cardiac_cycle_period)) {
            throw std::runtime_error(
                "Inconsistent cardiac cycle period defined in "
                "ClosedLoopHeartAndPulmonary.");
          } else {
            model.cardiac_cycle_period = cycle_period;
          }
          const auto& heart_params = closed_loop_config["parameters"];

          generate_block(model, heart_params, closed_loop_type,
                         heartpulmonary_name);

          // Junction at inlet to heart
          std::string heart_inlet_junction_name = "J_heart_inlet";
          connections.push_back(
              {heart_inlet_junction_name, heartpulmonary_name});
          generate_block(model, {}, "NORMAL_JUNCTION",
                         heart_inlet_junction_name);

          for (auto heart_inlet_elem : closed_loop_bcs) {
            connections.push_back(
                {heart_inlet_elem, heart_inlet_junction_name});
          }

          // Junction at outlet from heart
          std::string heart_outlet_junction_name = "J_heart_outlet";
          connections.push_back(
              {heartpulmonary_name, heart_outlet_junction_name});
          generate_block(model, {}, "NORMAL_JUNCTION",
                         heart_outlet_junction_name);
          for (auto& outlet_block : closed_loop_config["outlet_blocks"]) {
            connections.push_back({heart_outlet_junction_name, outlet_block});
          }
        } else {
          throw std::runtime_error(
              "Error. Only one ClosedLoopHeartAndPulmonary can be included.");
        }
      }
    }
  }

  // Create Connections
  for (auto& connection : connections) {
    auto ele1 = model.get_block(std::get<0>(connection));
    auto ele2 = model.get_block(std::get<1>(connection));
    model.add_node({ele1}, {ele2}, ele1->get_name() + ":" + ele2->get_name());
  }
  // Finalize model
  model.finalize();
}

/**
 * @brief Load initial conditions from a configuration
 *
 * @param config The json configuration
 * @param model The model
 * @return State Initial configuration for the model
 */
State load_initial_condition(const nlohmann::json& config, Model& model) {
  // Read initial condition
  auto initial_state = State::Zero(model.dofhandler.size());

  if (config.contains("initial_condition")) {
    const auto& initial_condition = config["initial_condition"];
    // Check for pressure_all or flow_all condition.
    // This will initialize all pressure:* and flow:* variables.
    double init_p, init_q;
    bool init_p_flag = initial_condition.contains("pressure_all");
    bool init_q_flag = initial_condition.contains("flow_all");
    if (init_p_flag) {
      init_p = initial_condition["pressure_all"];
    }
    if (init_q_flag) {
      init_q = initial_condition["flow_all"];
    }

    // Loop through variables and check for initial conditions.
    for (size_t i = 0; i < model.dofhandler.size(); i++) {
      std::string var_name = model.dofhandler.variables[i];
      double default_val = 0.0;
      // If initial condition is not specified for this variable,
      // check if pressure_all/flow_all are applicable
      if (!initial_condition.contains(var_name)) {
        if ((init_p_flag == true) && ((var_name.substr(0, 9) == "pressure:") ||
                                      (var_name.substr(0, 4) == "P_c:"))) {
          default_val = init_p;
          // DEBUG_MSG("pressure_all initial condition for " << var_name);
        } else if ((init_q_flag == true) &&
                   (var_name.substr(0, 5) == "flow:")) {
          default_val = init_q;
          // DEBUG_MSG("flow_all initial condition for " << var_name);
        } else {
          // DEBUG_MSG("No initial condition found for " << var_name << ".
          // Using default value = 0.");
        }
      }
      initial_state.y[i] = initial_condition.value(var_name, default_val);
    }
  }
  if (config.contains("initial_condition_d")) {
    // DEBUG_MSG("Reading initial condition derivative");
    const auto& initial_condition_d = config["initial_condition_d"];
    // Loop through variables and check for initial conditions.
    for (size_t i = 0; i < model.dofhandler.size(); i++) {
      std::string var_name = model.dofhandler.variables[i];
      if (!initial_condition_d.contains(var_name)) {
        // DEBUG_MSG("No initial condition derivative found for " <<
        // var_name);
      }
      initial_state.ydot[i] = initial_condition_d.value(var_name, 0.0);
    }
  }
  return initial_state;
}
