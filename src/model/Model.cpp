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

#include "Model.h"

template <typename block_type>
BlockFactoryFunc block_factory() {
  return [](int count, Model *model) -> Block * {
    return new block_type(count, model);
  };
}

Model::Model() {
  // Add all implemented blocks to factory
  block_factory_map = {
      {"BloodVessel", block_factory<BloodVessel>()},
      {"BloodVesselJunction", block_factory<BloodVesselJunction>()},
      {"ClosedLoopCoronaryLeft", block_factory<ClosedLoopCoronaryLeftBC>()},
      {"ClosedLoopCoronaryRight", block_factory<ClosedLoopCoronaryRightBC>()},
      {"ClosedLoopHeartAndPulmonary",
       block_factory<ClosedLoopHeartPulmonary>()},
      {"ClosedLoopRCR", block_factory<ClosedLoopRCRBC>()},
      {"CORONARY", block_factory<OpenLoopCoronaryBC>()},
      {"FLOW", block_factory<FlowReferenceBC>()},
      {"NORMAL_JUNCTION", block_factory<Junction>()},
      {"PRESSURE", block_factory<PressureReferenceBC>()},
      {"RCR", block_factory<WindkesselBC>()},
      {"RESISTANCE", block_factory<ResistanceBC>()},
      {"resistive_junction", block_factory<ResistiveJunction>()},
      {"ValveTanh", block_factory<ValveTanh>()},
      {"ChamberElastanceInductor", block_factory<ChamberElastanceInductor>()}};
}

Model::~Model() {}

Block *Model::create_block(const std::string &block_type) {
  // Get block from factory
  auto it = block_factory_map.find(block_type);
  if (it == block_factory_map.end()) {
    throw std::runtime_error("Invalid block type " + block_type);
  }
  Block *block = it->second(block_count, this);
  return block;
}

int Model::add_block(Block *block, const std::string_view &name,
                     const std::vector<int> &block_param_ids, bool internal) {
  // Set global parameter IDs
  block->setup_params_(block_param_ids);

  auto name_string = static_cast<std::string>(name);

  if (internal) {
    hidden_blocks.push_back(std::shared_ptr<Block>(block));
  } else {
    blocks.push_back(std::shared_ptr<Block>(block));
  }

  block_types.push_back(block->block_type);
  block_index_map.insert({name_string, block_count});
  block_names.push_back(name_string);

  return block_count++;
}

int Model::add_block(const std::string &block_name,
                     const std::vector<int> &block_param_ids,
                     const std::string_view &name, bool internal) {
  // Generate block from factory
  auto block = this->create_block(block_name);

  // Add block to model
  return this->add_block(block, name, block_param_ids, internal);
}

bool Model::has_block(const std::string &name) const {
  if (block_index_map.find(name) == block_index_map.end()) {
    return false;
  } else {
    return true;
  }
}

Block *Model::get_block(const std::string_view &name) const {
  auto name_string = static_cast<std::string>(name);

  if (!has_block(name_string)) {
    throw std::runtime_error("No block defined with name " + name_string);
  }

  return blocks[block_index_map.at(name_string)].get();
}

Block *Model::get_block(int block_id) const {
  if (block_id >= blocks.size()) {
    return hidden_blocks[block_id - blocks.size()].get();
  }

  return blocks[block_id].get();
}

BlockType Model::get_block_type(const std::string_view &name) const {
  auto name_string = static_cast<std::string>(name);

  if (block_index_map.find(name_string) == block_index_map.end()) {
    throw std::runtime_error("Could not find block with name " + name_string);
  }

  return block_types[block_index_map.at(name_string)];
}

std::string Model::get_block_name(int block_id) const {
  return block_names[block_id];
}

int Model::add_node(const std::vector<Block *> &inlet_eles,
                    const std::vector<Block *> &outlet_eles,
                    const std::string_view &name) {
  // DEBUG_MSG("Adding node " << name);
  auto node = std::shared_ptr<Node>(
      new Node(node_count, inlet_eles, outlet_eles, this));
  nodes.push_back(node);
  node_names.push_back(static_cast<std::string>(name));

  return node_count++;
}

std::string Model::get_node_name(int node_id) const {
  return node_names[node_id];
}

int Model::add_parameter(double value) {
  parameters.push_back(Parameter(parameter_count, value));
  parameter_values.push_back(parameters.back().get(0.0));
  return parameter_count++;
}

int Model::add_parameter(const std::vector<double> &times,
                         const std::vector<double> &values, bool periodic) {
  auto param = Parameter(parameter_count, times, values, periodic);
  if (periodic && (param.is_constant == false)) {
    if ((this->cardiac_cycle_period > 0.0) &&
        (param.cycle_period != this->cardiac_cycle_period)) {
      throw std::runtime_error(
          "Inconsistent cardiac cycle period defined in parameters");
    }
    this->cardiac_cycle_period = param.cycle_period;
  }
  parameter_values.push_back(param.get(0.0));
  parameters.push_back(std::move(param));
  return parameter_count++;
}

Parameter *Model::get_parameter(int param_id) { return &parameters[param_id]; }

double Model::get_parameter_value(int param_id) const {
  return parameter_values[param_id];
}

void Model::update_parameter_value(int param_id, double param_value) {
  parameter_values[param_id] = param_value;
}

void Model::finalize() {
  DEBUG_MSG("Setup degrees-of-freedom of nodes");
  for (auto &node : nodes) {
    node->setup_dofs(dofhandler);
  }
  DEBUG_MSG("Setup degrees-of-freedom of blocks");
  for (auto &block : blocks) {
    block->setup_dofs(dofhandler);
  }
  DEBUG_MSG("Setup model-dependent parameters");
  for (auto &block : blocks) {
    block->setup_model_dependent_params();
  }

  // if (cardiac_cycle_period < 0.0) {
  //   cardiac_cycle_period = 1.0;
  // }
}

int Model::get_num_blocks(bool internal) const {
  int num_blocks = blocks.size();

  if (internal) {
    num_blocks += hidden_blocks.size();
  }

  return num_blocks;
}

void Model::update_constant(SparseSystem &system) {
  for (auto block : blocks) {
    block->update_constant(system, parameter_values);
  }
}

void Model::update_time(SparseSystem &system, double time) {
  this->time = time;

  for (auto &param : parameters) {
    parameter_values[param.id] = param.get(time);
  }

  for (auto block : blocks) {
    block->update_time(system, parameter_values);
  }
}

void Model::update_solution(SparseSystem &system,
                            Eigen::Matrix<double, Eigen::Dynamic, 1> &y,
                            Eigen::Matrix<double, Eigen::Dynamic, 1> &dy) {
  for (auto block : blocks) {
    block->update_solution(system, parameter_values, y, dy);
  }
}

void Model::post_solve(Eigen::Matrix<double, Eigen::Dynamic, 1> &y) {
  for (auto block : blocks) {
    block->post_solve(y);
  }
}

void Model::to_steady() {
  for (auto &param : parameters) {
    param.to_steady();
  }

  // Special handling for time-varying capacitance
  for (size_t i = 0; i < get_num_blocks(true); i++) {
    get_block(i)->steady = true;
    if ((block_types[i] == BlockType::windkessel_bc) ||
        (block_types[i] == BlockType::closed_loop_rcr_bc)) {
      int param_id_capacitance = blocks[i]->global_param_ids[1];
      double value = parameters[param_id_capacitance].get(0.0);
      param_value_cache.insert({param_id_capacitance, value});
      parameters[param_id_capacitance].update(0.0);
    }
  }
}

void Model::to_unsteady() {
  for (auto &param : parameters) {
    param.to_unsteady();
  }
  for (auto &[param_id_capacitance, value] : param_value_cache) {
    // DEBUG_MSG("Setting Windkessel capacitance back to " << value);
    parameters[param_id_capacitance].update(value);
  }
  for (size_t i = 0; i < get_num_blocks(true); i++) {
    get_block(i)->steady = false;
  }
}

TripletsContributions Model::get_num_triplets() const {
  TripletsContributions triplets_sum;

  for (auto &elem : blocks) {
    triplets_sum += elem->get_num_triplets();
  }

  return triplets_sum;
}

void Model::setup_initial_state_dependent_parameters(State initial_state) {
  DEBUG_MSG("Setup initial state dependent parameters");
  for (auto &block : blocks) {
    block->setup_initial_state_dependent_params(initial_state,
                                                parameter_values);
  }
}

void Model::update_has_windkessel_bc(bool has_windkessel) {
  has_windkessel_bc = has_windkessel;
}

void Model::update_largest_windkessel_time_constant(double time_constant) {
  largest_windkessel_time_constant = time_constant;
}

bool Model::get_has_windkessel_bc() { return has_windkessel_bc; }

double Model::get_largest_windkessel_time_constant() {
  return largest_windkessel_time_constant;
}
