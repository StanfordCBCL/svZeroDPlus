/**
 * @file integrator.hpp
 * @brief ALGEBRA::Integrator source file
 */
#ifndef SVZERODSOLVER_ALGEBRA_INTEGRATOR_HPP_
#define SVZERODSOLVER_ALGEBRA_INTEGRATOR_HPP_

#include <map>
#include <Eigen/Dense>
#include <iostream>

#include "../model/model.hpp"
#include "state.hpp"

namespace ALGEBRA
{

    /**
     * @brief Generalized-alpha integrator
     *
     * This class handles the time integration scheme for solving 0D blood
     * flow system.
     *
     * Flow rate, pressure, and other hemodynamic quantities in 0D models of
     * vascular anatomies are governed by a system of nonlinear differential-algebraic equations (DAEs):
     *
     * \f[
     * \mathbf{E}(\mathbf{y}, t) \cdot \dot{\mathbf{y}}+\mathbf{F}(\mathbf{y}, t) \cdot \mathbf{y}+\mathbf{c}(\mathbf{y}, t)=\mathbf{0}
     * \f]
     *
     * Here, \f$y\f$ is the vector of solution quantities and \f$\dot{y}\f$ is the time derivative of \f$y\f$.
     * \f$N\f$ is the total number of equations and the total number of global unknowns. The DAE
     * system is solved implicitly using the generalized-\f$\alpha\f$ method \cite JANSEN2000305.
     *
     * @tparam T Scalar type (e.g. `float`, `double`)
     * @tparam S Type of System (e.g. `ALGEBRA::DenseSystem`, `ALGEBRA::SparseSystem`)
     */
    template <typename T, template <class> class S>
    class Integrator
    {
    private:
        T alpha_m;
        T alpha_f;
        T alpha_m_inv;
        T alpha_f_inv;
        T gamma;
        T gamma_inv;
        T time_step_size;
        T time_step_size_inv;
        T y_dot_coeff;
        T atol;
        int max_iter;
        int size;
        Eigen::Matrix<T, Eigen::Dynamic, 1> y_af;
        Eigen::Matrix<T, Eigen::Dynamic, 1> ydot_am;

        S<T> system;

    public:
        /**
         * @brief Construct a new Integrator object
         *
         * @param system System of equations to integrate
         * @param time_step_size Time step size for generalized-alpha step
         * @param rho Spectral radius for generalized-alpha step
         * @param atol Absolut tolerance for non-linear iteration termination
         * @param max_iter Maximum number of non-linear iterations
         */
        Integrator(S<T> &system, T time_step_size, T rho, T atol, int max_iter);

        /**
         * @brief Destroy the Integrator object
         *
         */
        ~Integrator();

        /**
         * @brief Perform a time step
         *
         * @param state Current state
         * @param time Current time
         * @param model The model
         * @return New state
         */
        State<T> step(State<T> &state, T time, MODEL::Model<T> &model);
    };

    template <typename T, template <class> class S>
    Integrator<T, S>::Integrator(S<T> &system, T time_step_size, T rho, T atol, int max_iter)
    {
        alpha_m = 0.5 * (3.0 - rho) / (1.0 + rho);
        alpha_f = 1.0 / (1.0 + rho);
        alpha_m_inv = alpha_m / 1.0;
        alpha_f_inv = alpha_f / 1.0;
        gamma = 0.5 + alpha_m - alpha_f;
        gamma_inv = 1.0 / gamma;

        this->system = system;
        this->time_step_size = time_step_size;
        this->atol = atol;
        this->max_iter = max_iter;
        time_step_size_inv = 1.0 / time_step_size;

        size = system.F.row(0).size();
        y_af = Eigen::Matrix<T, Eigen::Dynamic, 1>(size);
        ydot_am = Eigen::Matrix<T, Eigen::Dynamic, 1>(size);

        y_dot_coeff = alpha_m / (alpha_f * gamma) * time_step_size_inv;
    }

    template <typename T, template <class> class S>
    Integrator<T, S>::~Integrator()
    {
    }

    template <typename T, template <class> class S>
    State<T> Integrator<T, S>::step(State<T> &state, T time, MODEL::Model<T> &model)
    {

        // Create new state
        State<T> old_state = state;
        State<T> new_state;

        // Predictor step
        new_state.y = old_state.y + 0.5 * time_step_size * old_state.ydot;
        new_state.ydot = old_state.ydot * (gamma - 0.5) * gamma_inv;

        // Initiator step
        y_af = old_state.y + alpha_f * (new_state.y - old_state.y);
        ydot_am = old_state.ydot + alpha_m * (new_state.ydot - old_state.ydot);

        // Determine new time
        T new_time = time + alpha_f * time_step_size;

        // Update time-dependent element contributions in system
        model.update_time(system, new_time);

        for (size_t i = 0; i < max_iter; i++)
        {
            // Update solution-dependent element contribitions
            model.update_solution(system, y_af);

            // Update residuum and check termination criteria
            system.update_residual(y_af, ydot_am);
            if (system.residual.cwiseAbs().maxCoeff() < atol)
            {
                break;
            }

            // Abort if maximum number of non-linear iterations is reached
            else if (i == max_iter - 1)
            {
                throw std::runtime_error("Maxium number of non-linear iterations reached.");
            }

            // Determine jacobian
            system.update_jacobian(y_dot_coeff);

            // Solve system
            system.solve();

            // Add increment to solution
            y_af += system.dy;
            ydot_am += system.dy * y_dot_coeff;
        }

        // Set new state
        new_state.y = old_state.y + (y_af - old_state.y) * alpha_f_inv;
        new_state.ydot = old_state.ydot + (ydot_am - old_state.ydot) / alpha_m_inv;

        return new_state;
    }
} // namespace ALGEBRA

#endif // SVZERODSOLVER_ALGEBRA_INTEGRATOR_HPP_