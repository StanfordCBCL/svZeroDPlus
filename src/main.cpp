// #include <json/value.h>
#include <fstream>
#include <iostream>
#include <string>
#include "json.h"
#include <vector>
#include <map>
#include <list>
#include <variant>
#include "Eigen/Dense"

#include "junction.hpp"
#include "bloodvessel.hpp"
#include "rcrblockwithdistalpressure.hpp"
#include "flowreference.hpp"
#include "node.hpp"
#include "integrator.hpp"
#include "model.hpp"
#include "system.hpp"
#include "parameter.hpp"

#include <stdexcept>

typedef double T;

TimeDependentParameter<T> get_time_dependent_parameter(Json::Value &json_times, Json::Value &json_values)
{
    std::vector<T> times;
    std::vector<T> values;
    if (json_values.isDouble())
    {
        values.push_back(json_values.asDouble());
    }
    else
    {
        for (auto it = json_times.begin(); it != json_times.end(); it++)
        {
            times.push_back(it->asDouble());
        }
        for (auto it = json_values.begin(); it != json_values.end(); it++)
        {
            values.push_back(it->asDouble());
        }
    }
    return TimeDependentParameter<T>(times, values);
}

Model<T> create_model(Json::Value &config)
{
    // Create blog mapping
    Model<T> model;

    // Create list to store block connections while generating blocks
    std::vector<std::tuple<std::string, std::string>> connections;

    // Create junctions
    int num_junctions = config["junctions"].size();
    std::cout << "Number of junctions: " << num_junctions << std::endl;
    for (int i = 0; i < num_junctions; i++)
    {
        if ((config["junctions"][i]["junction_type"].asString() == "NORMAL_JUNCTION") || (config["junctions"][i]["junction_type"].asString() == "internal_junction"))
        {
            std::string name = config["junctions"][i]["junction_name"].asString();
            model.blocks.insert(std::make_pair(name, Junction<T>(name)));
            std::cout << "Created junction " << name << std::endl;
            for (int j = 0; j < config["junctions"][i]["inlet_vessels"].size(); j++)
            {
                auto connection = std::make_tuple("V" + config["junctions"][i]["inlet_vessels"][j].asString(), name);
                connections.push_back(connection);
                std::cout << "Found connection " << std::get<0>(connection) << "/" << std::get<1>(connection) << std::endl;
            }
            for (int j = 0; j < config["junctions"][i]["outlet_vessels"].size(); j++)
            {
                auto connection = std::make_tuple(name, "V" + config["junctions"][i]["outlet_vessels"][j].asString());
                connections.push_back(connection);
                std::cout << "Found connection " << std::get<0>(connection) << "/" << std::get<1>(connection) << std::endl;
            }
        }
        else
        {
            throw std::invalid_argument("Unknown junction type " + config["junctions"][i]["junction_type"].asString());
        }
    }

    // Create vessels
    int num_vessels = config["vessels"].size();
    std::cout << "Number of vessels: " << num_vessels << std::endl;
    for (int i = 0; i < num_vessels; i++)
    {
        if ((config["vessels"][i]["zero_d_element_type"].asString() == "BloodVessel"))
        {
            Json::Value vessel_values = config["vessels"][i]["zero_d_element_values"];
            std::string name = "V" + config["vessels"][i]["vessel_id"].asString();
            T R = vessel_values["R_poiseuille"].asDouble();
            T C = vessel_values.get("C", 0.0).asDouble();
            T L = vessel_values.get("L", 0.0).asDouble();
            T stenosis_coefficient = vessel_values.get("stenosis_coefficient", 0.0).asDouble();
            model.blocks.insert(std::make_pair(name, BloodVessel<T>(R = R, C = C, L = L, stenosis_coefficient = stenosis_coefficient, name = name)));
            std::cout << "Created vessel " << name << std::endl;

            if (config["vessels"][i].isMember("boundary_conditions"))
            {
                if (config["vessels"][i]["boundary_conditions"].isMember("inlet"))
                {
                    std::string bc_name = "BC" + config["vessels"][i]["vessel_id"].asString() + "_inlet";
                    std::string bc_handle = config["vessels"][i]["boundary_conditions"]["inlet"].asString();
                    for (int j = 0; j < config["boundary_conditions"].size(); j++)
                    {
                        if (config["boundary_conditions"][j]["bc_name"].asString() == bc_handle)
                        {
                            Json::Value bc_values = config["boundary_conditions"][j]["bc_values"];
                            if (config["boundary_conditions"][j]["bc_type"].asString() == "RCR")
                            {
                                T Rp = bc_values["Rp"].asDouble();
                                T C = bc_values["C"].asDouble();
                                T Rd = bc_values["Rd"].asDouble();
                                T Pd = bc_values["Pd"].asDouble();
                                model.blocks.insert(std::make_pair(bc_name, RCRBlockWithDistalPressure<T>(Rp = Rp, C = C, Rd = Rd, Pd = Pd, bc_name)));
                                std::cout << "Created boundary condition " << bc_name << std::endl;
                            }
                            else if (config["boundary_conditions"][j]["bc_type"].asString() == "FLOW")
                            {
                                Json::Value Q_json = bc_values["Q"];
                                Json::Value t_json = bc_values.get("t", 0.0);
                                auto Q = get_time_dependent_parameter(t_json, Q_json);
                                if (Q.isconstant == false)
                                {
                                    config["simulation_parameters"]["cardiac_cycle_period"] = Json::Value(Q.cycle_period);
                                }
                                model.blocks.insert(std::make_pair(bc_name, FlowReference<T>(Q = Q, bc_name)));
                                std::cout << "Created boundary condition " << bc_name << std::endl;
                            }
                            else
                            {
                                throw std::invalid_argument("Unknown boundary condition type " + config["boundary_conditions"][j]["bc_type"].asString());
                            }
                            break;
                        }
                    }
                    auto connection = std::make_tuple(bc_name, name);
                    connections.push_back(connection);
                    std::cout << "Found connection " << std::get<0>(connection) << "/" << std::get<1>(connection) << std::endl;
                }
                if (config["vessels"][i]["boundary_conditions"].isMember("outlet"))
                {
                    std::string bc_name = "BC" + config["vessels"][i]["vessel_id"].asString() + "_outlet";
                    std::string bc_handle = config["vessels"][i]["boundary_conditions"]["outlet"].asString();
                    for (int j = 0; j < config["boundary_conditions"].size(); j++)
                    {
                        if (config["boundary_conditions"][j]["bc_name"].asString() == bc_handle)
                        {
                            Json::Value bc_values = config["boundary_conditions"][j]["bc_values"];
                            if (config["boundary_conditions"][j]["bc_type"].asString() == "RCR")
                            {
                                T Rp = bc_values["Rp"].asDouble();
                                T C = bc_values["C"].asDouble();
                                T Rd = bc_values["Rd"].asDouble();
                                T Pd = bc_values["Pd"].asDouble();
                                model.blocks.insert(std::make_pair(bc_name, RCRBlockWithDistalPressure<T>(Rp = Rp, C = C, Rd = Rd, Pd = Pd, bc_name)));
                                std::cout << "Created boundary condition " << bc_name << std::endl;
                            }
                            else if (config["boundary_conditions"][j]["bc_type"].asString() == "FLOW")
                            {
                                Json::Value Q_json = bc_values["Q"];
                                Json::Value t_json = bc_values.get("t", 0.0);
                                auto Q = get_time_dependent_parameter(t_json, Q_json);
                                if (Q.isconstant == false)
                                {
                                    config["simulation_parameters"]["cardiac_cycle_period"] = Json::Value(Q.cycle_period);
                                }
                                model.blocks.insert(std::make_pair(bc_name, FlowReference<T>(Q = Q, bc_name)));
                                std::cout << "Created boundary condition " << bc_name << std::endl;
                            }
                            else
                            {
                                throw std::invalid_argument("Unknown boundary condition type " + config["boundary_conditions"][j]["bc_type"].asString());
                            }
                            break;
                        }
                    }
                    auto connection = std::make_tuple(name, bc_name);
                    connections.push_back(connection);
                    std::cout << "Found connection " << std::get<0>(connection) << "/" << std::get<1>(connection) << std::endl;
                }
            }
        }
        else
        {
            throw std::invalid_argument("Unknown vessel type " + config["vessels"][i]["vessel_type"].asString());
        }
    }

    // Create Connections
    for (auto &connection : connections)
    {
        for (auto &[key, elem1] : model.blocks)
        {
            std::visit([&](auto &&ele1)
                       {
                        for (auto &[key, elem2] : model.blocks)
                        {
                            std::visit([&](auto &&ele2)
                                    {if ((ele1.name == std::get<0>(connection)) && (ele2.name == std::get<1>(connection))){ model.nodes.push_back(Node(ele1.name + "_" + ele2.name)); std::cout << "Created node " << model.nodes.back().name << std::endl; ele1.outlet_nodes.push_back(&model.nodes.back()); ele2.inlet_nodes.push_back(&model.nodes.back()); model.nodes.back().setup_dofs(model.dofhandler); } },
                                    elem2);
                        } },
                       elem1);
        }
    }
    for (auto &[key, elem] : model.blocks)
    {
        std::visit([&](auto &&block)
                   { block.setup_dofs(model.dofhandler); },
                   elem);
    }
    return model;
}

bool startsWith(const std::string &str, const std::string &prefix)
{
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

void write_json(std::string path, std::vector<T> times, std::vector<State<T>> states, Model<T> model)
{
    Json::Value output;
    Json::Value json_times(Json::arrayValue);
    for (auto time : times)
    {
        json_times.append(Json::Value(time));
    }

    Json::Value json_names(Json::arrayValue);
    Json::Value json_flow_in(Json::arrayValue);
    Json::Value json_flow_out(Json::arrayValue);
    Json::Value json_pres_in(Json::arrayValue);
    Json::Value json_pres_out(Json::arrayValue);

    for (auto &[key, elem] : model.blocks)
    {
        std::string name = "NoName";
        unsigned int inflow_dof;
        unsigned int outflow_dof;
        unsigned int inpres_dof;
        unsigned int outpres_dof;
        std::visit([&](auto &&block)
                   { if (startsWith(block.name, "V")){name = block.name; inflow_dof = block.inlet_nodes[0]->flow_dof; outflow_dof = block.outlet_nodes[0]->flow_dof; inpres_dof = block.inlet_nodes[0]->pres_dof; outpres_dof = block.outlet_nodes[0]->pres_dof;} },
                   elem);

        if (name != "NoName")
        {
            json_names.append(name);
            Json::Value json_flow_in_i(Json::arrayValue);
            Json::Value json_flow_out_i(Json::arrayValue);
            Json::Value json_pres_in_i(Json::arrayValue);
            Json::Value json_pres_out_i(Json::arrayValue);
            for (auto state : states)
            {
                json_flow_in_i.append(state.y[inflow_dof]);
                json_flow_out_i.append(state.y[outflow_dof]);
                json_pres_in_i.append(state.y[inpres_dof]);
                json_pres_out_i.append(state.y[outpres_dof]);
            }
            json_flow_in.append(json_flow_in_i);
            json_flow_out.append(json_flow_out_i);
            json_pres_in.append(json_pres_in_i);
            json_pres_out.append(json_pres_out_i);
        }
    }

    output["time"] = json_times;
    output["names"] = json_names;
    output["flow_in"] = json_flow_in;
    output["flow_out"] = json_flow_out;
    output["pressure_in"] = json_pres_in;
    output["pressure_out"] = json_pres_out;

    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "   ";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ofstream outputfilestream(path);
    writer->write(output, &outputfilestream);
}

int main(int argc, char *argv[])
{
    std::cout << "Starting svZeroDSolver" << std::endl;
    std::cout << "Reading configuration from " << argv[1] << std::endl;
    std::ifstream file_input(argv[1]);
    Json::Reader reader;
    Json::Value config;
    reader.parse(file_input, config);

    // Create the blocks
    std::cout << "Creating model" << std::endl;
    auto model = create_model(config);

    Json::Value sim_params = config["simulation_parameters"];
    T cardiac_cycle_period = sim_params.get("cardiac_cycle_period", 1.0).asDouble();
    T num_cycles = sim_params["number_of_cardiac_cycles"].asDouble();
    T num_pts_per_cycle = sim_params["number_of_time_pts_per_cardiac_cycle"].asDouble();
    T time_step_size = cardiac_cycle_period / (num_pts_per_cycle - 1);
    int num_time_steps = (num_pts_per_cycle - 1) * num_cycles + 1;

    std::cout << "Setup simulutation" << std::endl;
    std::cout << "Number of timesteps: " << num_time_steps << std::endl;
    std::cout << "Time step size:      " << time_step_size << std::endl;
    std::cout << "Size of system:      " << model.dofhandler.size() << std::endl;

    int max_iter = 30;

    bool steady_inital = true;

    State<T> state = State<T>::Zero(model.dofhandler.size());
    if (steady_inital)
    {
        std::cout << "Calculating steady initial condition" << std::endl;
        T time_step_size_steady = cardiac_cycle_period / 10.0;
        int num_time_steps_steady = 31;
        auto model_steady = create_model(config);
        model_steady.to_steady();
        System<T> system_steady(model_steady.dofhandler.size());
        model_steady.update_constant(system_steady);
        Integrator<T> integrator_steady(system_steady, time_step_size_steady, 0.1);
        State<T> state_steady = State<T>::Zero(model_steady.dofhandler.size());
        T time_steady = 0.0;

        for (size_t i = 0; i < num_time_steps_steady; i++)
        {
            time_steady = time_step_size_steady * T(i);
            state_steady = integrator_steady.step(state_steady, time_steady, model_steady, max_iter);
        }
        state = state_steady;
    }

    std::cout << "Starting simulation" << std::endl;
    System<T> system(model.dofhandler.size());
    model.update_constant(system);

    Integrator<T> integrator(system, time_step_size, 0.1);

    T time = 0.0;

    std::vector<State<T>> states;
    std::vector<T> times;

    states.push_back(state);
    times.push_back(0.0);

    int output_interval = 1;
    int interval_counter = 0;

    for (size_t i = 0; i < num_time_steps; i++)
    {
        state = integrator.step(state, time, model, max_iter);
        time = time + time_step_size;
        interval_counter += 1;
        if (interval_counter == output_interval)
        {
            times.push_back(time);
            states.push_back(state);
            interval_counter = 0;
        }
    }
    std::cout << "Simulation completed" << std::endl;

    write_json(argv[2], times, states, model);

    return 0;
}
