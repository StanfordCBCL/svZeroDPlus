
#include "Model.h"
#include "Node.h"

namespace zd_model {

Model::Model() {}

Model::~Model() {}

int Model::add_block(BlockType block_type, const std::vector<int> &block_param_ids, std::string_view name, bool internal) 
{
  //DEBUG_MSG("Adding block " << name << " with type " << block_type);

  Block* block{nullptr};


  switch (block_type) {
    case BlockType::BLOODVESSEL:
      block = new BloodVessel(block_count, block_param_ids, this);
    break;

    case BlockType::JUNCTION:
      block = new Junction(block_count, block_param_ids, this);
    break;

    case BlockType::BLOODVESSELJUNCTION:
      block = new BloodVesselJunction(block_count, block_param_ids, this);
    break;

    case BlockType::RESISTIVEJUNCTION:
      block = new ResistiveJunction(block_count, block_param_ids, this);
    break;

    case BlockType::FLOWBC:
      block = new FlowReferenceBC(block_count, block_param_ids, this); 
    break;

    case BlockType::RESISTANCEBC:
      block = new ResistanceBC(block_count, block_param_ids, this);
    break;

    case BlockType::WINDKESSELBC:
      block = new WindkesselBC(block_count, block_param_ids, this);
    break;

    case BlockType::PRESSUREBC:
      block = new PressureReferenceBC(block_count, block_param_ids, this);
    break;

    case BlockType::OPENLOOPCORONARYBC:
      block = new OpenLoopCoronaryBC(block_count, block_param_ids, this);
    break;

    case BlockType::CLOSEDLOOPCORONARYLEFTBC:
      block = new ClosedLoopCoronaryBC(block_count, block_param_ids, this, Side::LEFT);
          //new ClosedLoopCoronaryBC<double, MODEL::Side::LEFT>(
          //    block_count, block_param_ids, this));
    break;

    case BlockType::CLOSEDLOOPCORONARYRIGHTBC:
      block = new ClosedLoopCoronaryBC(block_count, block_param_ids, this, Side::RIGHT);
      //block = new ClosedLoopCoronaryBC<double, MODEL::Side::RIGHT>(
      //        block_count, block_param_ids, this));
    break;

    case BlockType::CLOSEDLOOPRCRBC:
      block = new ClosedLoopRCRBC(block_count, block_param_ids, this);
    break;

    case BlockType::CLOSEDLOOPHEARTPULMONARY:
      block = new ClosedLoopHeartPulmonary(block_count, block_param_ids, this);
    break;

    default:
      throw std::runtime_error(
          "Adding block to model failed: Invalid block type!");
  }

  auto name_string = static_cast<std::string>(name);

  if (internal) {
    hidden_blocks.push_back(std::shared_ptr<Block>(block));
  } else {
    blocks.push_back(std::shared_ptr<Block>(block));
  }

  block_types.push_back(block_type);
  block_index_map.insert({name_string, block_count});
  block_names.push_back(name_string);

  return block_count++;
}

Block *Model::get_block(std::string_view name) 
{
  auto name_string = static_cast<std::string>(name);

  if (block_index_map.find(name_string) == block_index_map.end()) {
    return nullptr;
  }

  return blocks[block_index_map[name_string]].get();
}

Block *Model::get_block(int block_id) 
{
  if (block_id >= blocks.size()) {
    return hidden_blocks[block_id - blocks.size()].get();
  }

  return blocks[block_id].get();
}

BlockType Model::get_block_type(std::string_view name) 
{
  auto name_string = static_cast<std::string>(name);

  if (block_index_map.find(name_string) == block_index_map.end()) {
    throw std::runtime_error("Could not find block with name " + name_string);
  }

  return block_types[block_index_map[name_string]];
}

std::string Model::get_block_name(int block_id) 
{
  return block_names[block_id];
}

int Model::add_node(const std::vector<Block*> &inlet_eles, const std::vector<Block *> &outlet_eles,
                       std::string_view name) 
{
  //DEBUG_MSG("Adding node " << name);
  auto node = std::shared_ptr<Node>( new Node(node_count, inlet_eles, outlet_eles, this));
  nodes.push_back(node);
  node_names.push_back(static_cast<std::string>(name));

  return node_count++;
}

std::string Model::get_node_name(int node_id) 
{
  return node_names[node_id];
}

int Model::add_parameter(double value) 
{
  parameters.push_back(Parameter(parameter_count, value));
  parameter_values.push_back(parameters.back().get(0.0));
  return parameter_count++;
}

int Model::add_parameter(const std::vector<double> &times, const std::vector<double> &values, bool periodic) 
{
  auto param = Parameter(parameter_count, times, values, periodic);
  if (periodic && (param.isconstant == false)) {
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

Parameter *Model::get_parameter(int param_id) {
  return &parameters[param_id];
}

double Model::get_parameter_value(int param_id) 
{
  return parameter_values[param_id];
}

void Model::update_parameter_value(int param_id, double param_value) 
{
  parameter_values[param_id] = param_value;
}

void Model::finalize() 
{
  //DEBUG_MSG("Setup degrees-of-freedom of nodes");
  for (auto &node : nodes) {
    node->setup_dofs(dofhandler);
  }
  //DEBUG_MSG("Setup degrees-of-freedom of blocks");
  for (auto &block : blocks) {
    block->setup_dofs(dofhandler);
  }
  //DEBUG_MSG("Setup model-dependent parameters");
  for (auto &block : blocks) {
    block->setup_model_dependent_params();
  }

  if (cardiac_cycle_period < 0.0) {
    cardiac_cycle_period = 1.0;
  }
}

int Model::get_num_blocks(bool internal) 
{
  int num_blocks = blocks.size();

  if (internal) {
    num_blocks += hidden_blocks.size();
  }

  return num_blocks;
}

void Model::update_constant(algebra::SparseSystem& system) 
{
  for (auto block : blocks) {
    block->update_constant(system, parameter_values);
  }
}


void Model::update_time(algebra::SparseSystem& system, double time) 
{
  this->time = time;

  for (auto &param : parameters) {
    parameter_values[param.id] = param.get(time);
  }

  for (auto block : blocks) {
    block->update_time(system, parameter_values);
  }

}

void Model::update_solution(algebra::SparseSystem& system, Eigen::Matrix<double, Eigen::Dynamic, 1> &y,
    Eigen::Matrix<double, Eigen::Dynamic, 1> &dy) 
{
  for (auto block : blocks) {
    block->update_solution(system, parameter_values, y, dy);
  }
}

void Model::to_steady() 
{
  for (auto &param : parameters) {
    param.to_steady();
  }

  for (size_t i = 0; i < get_num_blocks(true); i++) {
    get_block(i)->steady = true;
    if ((block_types[i] == BlockType::WINDKESSELBC) ||
        (block_types[i] == BlockType::CLOSEDLOOPRCRBC)) {
      int param_id_capacitance = blocks[i]->global_param_ids[1];
      double value = parameters[param_id_capacitance].get(0.0);
      param_value_cache.insert({param_id_capacitance, value});
      parameters[param_id_capacitance].update(0.0);
    }
  }
}

void Model::to_unsteady() 
{
  for (auto &param : parameters) {
    param.to_unsteady();
  }
  for (auto &[param_id_capacitance, value] : param_value_cache) {
    //DEBUG_MSG("Setting Windkessel capacitance back to " << value);
    parameters[param_id_capacitance].update(value);
  }
  for (size_t i = 0; i < get_num_blocks(true); i++) {
    get_block(i)->steady = false;
  }
}

std::map<std::string, int> Model::get_num_triplets() {
  std::map<std::string, int> num_triplets = {
      {"F", 0},
      {"E", 0},
      {"D", 0},
  };
  for (auto &elem : blocks) {
    for (auto &[key, value] : elem->get_num_triplets()) {
      num_triplets[key] += value;
    }
  }
  return num_triplets;
}

};  

