
#include "Node.h"
#include "Block.h"
#include "Model.h"

namespace zd_model {

Node::Node(int id, const std::vector<Block *> &inlet_eles, const std::vector<Block *> &outlet_eles,
    Model *model) 
{
  this->id = id;
  this->inlet_eles = inlet_eles;
  this->outlet_eles = outlet_eles;
  this->model = model;

  for (auto &inlet_ele : inlet_eles) {
    inlet_ele->outlet_nodes.push_back(this);
  }

  for (auto &outlet_ele : outlet_eles) {
    outlet_ele->inlet_nodes.push_back(this);
  }
}

Node::~Node() {}

std::string Node::get_name() 
{
  return this->model->get_node_name(this->id);
}

void Node::setup_dofs(DOFHandler& dofhandler) 
{
  flow_dof = dofhandler.register_variable("flow:" + get_name());
  pres_dof = dofhandler.register_variable("pressure:" + get_name());
}

};

