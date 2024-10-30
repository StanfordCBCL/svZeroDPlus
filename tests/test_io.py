import numpy as np

from .utils import run_test_case_by_name, RTOL_FLOW, RTOL_PRES

# use coarse absolute tolerances for gradient calculation
# we're comparing gradients from gen-alpha in svZeroDSolver with central differences in np.gradient
ATOL = {"f": 0.6, "p": 800.0}
ATOL_MEAN = {"f": 0.01, "p": 20.0}


def test_pulsatile_flow_r_rcr_mean():
    # time-dependent results
    res_time = run_test_case_by_name("pulsatileFlow_R_RCR")

    # time-averaged results
    res_mean = run_test_case_by_name("pulsatileFlow_R_RCR_mean")

    # compare all time-dependent results to their average
    tol = {"f": RTOL_FLOW, "p": RTOL_PRES}
    for k, res in res_time.items():
        assert np.isclose(np.mean(res[0]), res_mean[k][0][0], rtol=tol[k[0]])


def test_pulsatile_flow_r_rcr_mean_variable():
    # time-dependent results
    res_time = run_test_case_by_name(
        "pulsatileFlow_R_RCR_variable", output_variable_based=True
    )

    # time-averaged results
    res_mean = run_test_case_by_name(
        "pulsatileFlow_R_RCR_mean_variable", output_variable_based=True
    )

    # fields and segments to test
    fields = ["flow", "pressure"]
    segments = {"in": "INFLOW:branch0_seg0", "out": "branch0_seg0:OUT"}

    tol = {"f": RTOL_FLOW, "p": RTOL_PRES}
    for f in fields:
        for k, v in segments.items():
            rt = np.mean(res_time[f + "_" + k])
            rm = res_mean["y"][res_mean["name"] == f + ":" + v]
            assert np.isclose(rt, rm, rtol=tol[f[0]])


def test_pulsatile_flow_r_rcr_derivative():
    results = run_test_case_by_name(
        "pulsatileFlow_R_RCR_derivative", output_variable_based=True
    )

    # time step
    dt = results["time"][1] - results["time"][0]

    # fields to test
    fields = ["pressure_in", "pressure_out", "flow_in", "flow_out"]
    for f in fields:
        ref = np.gradient(results[f], dt)
        res = results["d_" + f]
        assert np.all(np.isclose(ref, res, atol=ATOL[f[0]]))


def test_pulsatile_flow_r_rcr_derivative_variable():
    results = run_test_case_by_name(
        "pulsatileFlow_R_RCR_derivative_variable", output_variable_based=True
    )

    # time step
    dt = results["time"][1] - results["time"][0]

    # fields and segments to test
    fields = ["flow", "pressure"]
    segments = ["INFLOW:branch0_seg0", "branch0_seg0:OUT"]
    for f in fields:
        for v in segments:
            ids = results["name"] == f + ":" + v
            ref = np.gradient(results.y[ids], dt)
            res = results.ydot[ids]
            assert np.all(np.isclose(ref, res, atol=ATOL[f[0]]))


def test_pulsatile_flow_r_rcr_mean_derivative():
    # time-dependent results
    res_time = run_test_case_by_name("pulsatileFlow_R_RCR", output_variable_based=True)

    # time-averaged results
    res_mean = run_test_case_by_name(
        "pulsatileFlow_R_RCR_mean_derivative", output_variable_based=True
    )

    # time step
    dt = res_time["time"][1] - res_time["time"][0]

    # fields to test
    fields = ["pressure_in", "pressure_out", "flow_in", "flow_out"]
    for f in fields:
        ref = np.mean(np.gradient(res_time[f], dt))
        res = res_mean["d_" + f]
        assert np.isclose(ref, res, atol=ATOL_MEAN[f[0]])


def test_pulsatile_flow_r_rcr_mean_derivative_variable():
    # time-dependent results
    res_time = run_test_case_by_name(
        "pulsatileFlow_R_RCR_variable", output_variable_based=True
    )

    # time-averaged results
    res_mean = run_test_case_by_name(
        "pulsatileFlow_R_RCR_mean_derivative_variable", output_variable_based=True
    )

    # time step
    dt = res_time["time"][1] - res_time["time"][0]

    # fields and segments to test
    fields = ["flow", "pressure"]
    segments = {"in": "INFLOW:branch0_seg0", "out": "branch0_seg0:OUT"}
    for f in fields:
        for k, v in segments.items():
            rt = np.mean(np.gradient(res_time[f + "_" + k], dt))
            rm = res_mean["ydot"][res_mean["name"] == f + ":" + v]
            assert np.isclose(rt, rm, atol=ATOL_MEAN[f[0]])


def test_time_dependent_block():
    # time-dependent results
    res_flow = run_test_case_by_name("timeDep_Flow", output_variable_based=True)
    res_press = run_test_case_by_name("timeDep_Pressure", output_variable_based=True)

    time = res_flow["time"][res_flow["name"] == "flow:INLET:branch0_seg0"].to_numpy()
    flow = res_flow["y"][res_flow["name"] == "flow:INLET:branch0_seg0"].to_numpy()
    pressure = res_press["y"][res_press["name"] == "pressure:INLET:branch0_seg0"].to_numpy()

    # compare time-dependent results to the calculated values
    for i, t in enumerate(time):
        # may be unsteady at the beginning
        if i < 15:
            continue
        calc_val = 2.0 * (4*np.arctan(1.)) * np.cos(2.0 * (4*np.arctan(1.)) * t)
        assert np.isclose(flow[i], calc_val, rtol=0.003)
        assert np.isclose(pressure[i], calc_val, rtol=0.003)