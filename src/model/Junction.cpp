
#include "Junction.h"

namespace zd_model {

void Junction::setup_dofs(DOFHandler &dofhandler) 
{
  // Set number of equations of a junction block based on number of
  // inlets/outlets. Must be set before calling parent constructor
  num_inlets = this->inlet_nodes.size();
  num_outlets = this->outlet_nodes.size();
  Block::setup_dofs_(dofhandler, num_inlets + num_outlets, {});
  num_triplets["F"] =
      (num_inlets + num_outlets - 1) * 2 + num_inlets + num_outlets;
}

void Junction::update_constant(algebra::SparseSystem& system, std::vector<double> &parameters) 
{
  // Pressure conservation
  for (size_t i = 0; i < (num_inlets + num_outlets - 1); i++) {
    system.F.coeffRef(this->global_eqn_ids[i], this->global_var_ids[0]) = 1.0;
    system.F.coeffRef(this->global_eqn_ids[i],
                      this->global_var_ids[2 * i + 2]) = -1.0;
  }

  // Mass conservation
  for (size_t i = 1; i < num_inlets * 2; i = i + 2) {
    system.F.coeffRef(this->global_eqn_ids[num_inlets + num_outlets - 1],
                      this->global_var_ids[i]) = 1.0;
  }
  for (size_t i = (num_inlets * 2) + 1; i < (num_inlets + num_outlets) * 2;
       i = i + 2) {
    system.F.coeffRef(this->global_eqn_ids[num_inlets + num_outlets - 1],
                      this->global_var_ids[i]) = -1.0;
  }
}

void Junction::update_gradient(Eigen::SparseMatrix<double> &jacobian,
                               Eigen::Matrix<double, Eigen::Dynamic, 1> &residual,
                               Eigen::Matrix<double, Eigen::Dynamic, 1> &alpha,
                               std::vector<double> &y, std::vector<double> &dy) 
{
  // Pressure conservation
  residual(this->global_eqn_ids[0]) =
      y[this->global_var_ids[0]] - y[this->global_var_ids[2]];

  residual(this->global_eqn_ids[1]) =
      y[this->global_var_ids[1]] - y[this->global_var_ids[3]];
}

std::map<std::string, int> Junction::get_num_triplets() {
  return num_triplets;
}

};

