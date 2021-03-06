"""@package rad
Python radial attention model classes.

The file contains three basic classes; namely a grid, a variable and a model
class. These classes provide are used as a bridge between the c applications
and the Python Jupyter notebook. The c applications solve the radial attention
model, generate solution approximation data and store them in the file system.
These classes load the data and provide an interface for them to be used in a
Python Jupyter notebook. The notebook functionality creates the tables and the
figures that are used in the article.
"""

import glob
import os.path
import struct
from string import Template

import matplotlib.pyplot as plt
import numpy as np
import plotly.graph_objs as go
import plotly.offline as py
import rad_conf
from mpl_toolkits.mplot3d import Axes3D
from scipy.ndimage.filters import gaussian_filter1d


class Grid:
    """Grid wrapper class.

    Loads and holds the binary data created by the c grid_t structure.

    Attributes:
        datafile (str): The filename of the binary grid data file.
        data (ndarray): The grid data.
        weight (double): The grid point weighting.
    """

    datafile = None
    data = None
    weight = None

    def __init__(self, datafile):
        """Constructor.

        Args:
            datafile (str): The filename of the binary grid data file.
        """

        self.datafile = datafile
        with open(datafile, "rb") as file_handle:
            byte = file_handle.read(2)
            self.data = np.zeros(int.from_bytes(byte, byteorder="little"))
            byte = file_handle.read(8)
            self.weight = struct.unpack("d", byte)[0]
            for i in range(0, self.data.size):
                byte = file_handle.read(8)
                self.data[i] = struct.unpack("d", byte)[0]

    def __str__(self):
        """String representation of the grid object.

        Returns:
            A string representation of the object.
        """
        return "{}={{.n={}, .m={}, .M={}, .weight={}}}".format(
            self.datafile, self.data.size, self.data[0], self.data[-1], self.weight,
        )

    def eqpart(self, size):
        """Get indices approximating equi-partition.

        The function returns a set of indices, the values of which approximate
        a grid's equipartition. The function calculates the values of an
        equipartition of the passed size for the continuous domain of the grid.
        For each calculated value, it locates the index that corresponds to the
        lowest upper bound of this value in the discretized grid's domain. The
        set of these indices are returned.

        Args:
            size (int): The number of points in the partition.

        Returns:
            An array with the indices approximating the partition points.
        """

        idc = np.zeros(size, dtype=np.int)
        idc[-1] = len(self.data) - 1
        step = (self.data[-1] - self.data[0]) / (size - 1)
        for k in range(1, size - 1):
            idc[k] = np.min(np.where(self.data > k * step))
        return idc

    def lower_bound_index(self, values):
        """Get the index of the lower bracket value.

        For each element of the passed values, it finds and returns the minimum
        among the indices with values that are greater than the value.

        Args:
            values (ndarray): Array with values the lower bound indices of
                              which are to be located.

        Returns:
            An array with the indices of the discretized grid's lower bound.
        """

        out = np.zeros(len(values), dtype=int)
        for idx, val in enumerate(values):
            valid_idx = np.where(self.data >= val)[0]
            out[idx] = valid_idx[self.data[valid_idx].argmin()]
        return out


class Variable:
    """Variable class.

    Loads and holds the binary data that approximate the optimal controls and
    the value functions generated by the c applications.

    Attributes:
        datafile (str): The filename of the binary variable data file.
        x_grid (Grid): The wealth grid data.
        r_grid (Grid): The radius grid data.
        data (ndarray): The variable data.
    """

    datafile = None
    x_grid = None
    r_grid = None
    data = None

    def __init__(self, xgrid, rgrid, datafile=None, zvar=None):
        """Constructor.

        Args:
            xgrid (Grid): The wealth grid data.
            rgrid (Grid): The radius grid data.

        Kwargs:
            datafile (str): The filename of the binary variable data file.
            zvar (ndarray): Variable data array.
        """

        self.x_grid = xgrid
        self.r_grid = rgrid
        if datafile is not None:
            self.datafile = datafile
            with open(datafile, "rb") as file_handle:
                byte = file_handle.read(2)
                x_size = int.from_bytes(byte, byteorder="little")
                if x_size != self.x_grid.data.size:
                    raise "Invalid 1st dimension size"
                byte = file_handle.read(2)
                r_size = int.from_bytes(byte, byteorder="little")
                if r_size != self.r_grid.data.size:
                    raise "Invalid 2nd dimension size"
                self.data = np.zeros((x_size, r_size))
                for i in range(0, x_size):
                    for j in range(0, r_size):
                        byte = file_handle.read(8)
                        self.data[j, i] = struct.unpack("d", byte)[0]
        elif zvar is not None:
            if zvar.shape != (self.x_grid.data.size, self.r_grid.data.size):
                raise "Invalid shape"
            self.data = zvar
        # print(self)

    def __str__(self):
        """String representation of the variable object.

        Returns:
            A string representation of the object.
        """
        return "{}={{.d1={}, .d2={}, .shape={}}}".format(
            self.datafile, self.x_grid, self.r_grid, self.data.shape
        )

    def surf_visual(self):
        """Interactive surface visualization for Python Jupyter notebook."""

        surface = go.Surface(x=self.x_grid.data, y=self.r_grid.data, z=self.data)
        data = [surface]

        layout = go.Layout(
            title=self.datafile.split("/")[-1],
            scene=dict(
                xaxis=dict(
                    title="x",
                    gridcolor="rgb(255, 255, 255)",
                    zerolinecolor="rgb(255, 255, 255)",
                    showbackground=True,
                    backgroundcolor="rgb(230, 230,230)",
                ),
                yaxis=dict(
                    title="r",
                    gridcolor="rgb(255, 255, 255)",
                    zerolinecolor="rgb(255, 255, 255)",
                    showbackground=True,
                    backgroundcolor="rgb(230, 230,230)",
                ),
                zaxis=dict(
                    gridcolor="rgb(255, 255, 255)",
                    zerolinecolor="rgb(255, 255, 255)",
                    showbackground=True,
                    backgroundcolor="rgb(230, 230,230)",
                ),
            ),
        )

        fig = go.Figure(data=data, layout=layout)
        py.iplot(fig, filename="datafile")

    def save_figs(self, fig_data):
        """Create, show and save in png format a variable surface, its wealth
           and radius sections.

        Args:
            fig_data (dict): Figure data. Allowed keys are {angle, prefix,
                             zlabel, rsections, rsections_points,
            xsections, xsections_points}
        """

        fig = plt.figure()
        axes = fig.gca(projection=Axes3D.name)
        axes.set_zlabel(fig_data["zlabel"])
        x_mesh, r_mesh = np.meshgrid(self.x_grid.data, self.r_grid.data)
        axes.set_xlabel("x")
        axes.set_ylabel("r")
        axes.plot_surface(
            x_mesh,
            r_mesh,
            self.data,
            cmap=rad_conf.RAD_FIG_CM,
            linewidth=0,
            antialiased=False,
        )
        axes.view_init(30, fig_data["angle"])
        fig.savefig(
            rad_conf.RAD_TEMP_DIR + "/" + fig_data["prefix"] + "_surf.png",
            **rad_conf.RAD_FIG_PROPERTIES,
        )

        if "rsections" not in fig_data:
            fig_data["rsections"] = self.r_grid.eqpart(4)
        else:
            fig_data["rsections"] = self.r_grid.lower_bound_index(fig_data["rsections"])

        fig = plt.figure()
        axes = fig.gca()
        axes.set_prop_cycle(rad_conf.RAD_FIG_CYCLE)
        axes.set_ylabel(fig_data["zlabel"])
        axes.set_xlabel("x")
        for r_idx, r_val in enumerate(fig_data["rsections"]):
            smoothed = gaussian_filter1d(self.data[r_val, :], sigma=1)
            axes.plot(
                self.x_grid.data,
                smoothed,
                label="$r_{}={:.2f}$".format(r_idx, self.r_grid.data[r_val]),
            )
            if "rsections_points" in fig_data:
                plt.plot(
                    fig_data["rsections_points"][r_idx][1][0],
                    fig_data["rsections_points"][r_idx][1][1],
                    linestyle="None",
                    marker=".",
                    color="black",
                    markersize=5,
                )
                axes.annotate(
                    fig_data["rsections_points"][r_idx][0],
                    fig_data["rsections_points"][r_idx][1],
                    xytext=(-18, 12),
                    va="top",
                    textcoords="offset points",
                )
        plt.legend(**rad_conf.RAD_LEGEND_PROPERTIES)
        fig.savefig(
            rad_conf.RAD_TEMP_DIR + "/" + fig_data["prefix"] + "_rsections.png",
            **rad_conf.RAD_FIG_PROPERTIES,
        )

        if "xsections" not in fig_data:
            fig_data["xsections"] = self.x_grid.eqpart(4)
        else:
            fig_data["xsections"] = self.x_grid.lower_bound_index(fig_data["xsections"])

        fig = plt.figure()
        axes = fig.gca()
        axes.set_prop_cycle(rad_conf.RAD_FIG_CYCLE)
        axes.set_ylabel(fig_data["zlabel"])
        axes.set_xlabel("r")
        for x_idx, x_val in enumerate(fig_data["xsections"]):
            smoothed = gaussian_filter1d(self.data[:, x_val], sigma=1)
            axes.plot(
                self.r_grid.data,
                smoothed,
                label="$x_{}={:.2f}$".format(x_idx, self.x_grid.data[x_val]),
            )
            if "xsections_points" in fig_data:
                plt.plot(
                    fig_data["xsections_points"][x_idx][1][0],
                    fig_data["xsections_points"][x_idx][1][1],
                    linestyle="None",
                    marker=".",
                    color="black",
                    markersize=5,
                )
                axes.annotate(
                    fig_data["xsections_points"][x_idx][0],
                    fig_data["xsections_points"][x_idx][1],
                    xytext=(-10, 12),
                    va="top",
                    textcoords="offset points",
                )
        plt.legend(**rad_conf.RAD_LEGEND_PROPERTIES)
        fig.savefig(
            rad_conf.RAD_TEMP_DIR + "/" + fig_data["prefix"] + "_xsections.png",
            **rad_conf.RAD_FIG_PROPERTIES,
        )


class RadialAttentionModel:
    """Radial attention Model class.

    Loads binary data of the c setup structure. The format of the binary data
    is specified in the documentation of the c applications.

    Attributes:
        __ltbl_prototype__ (str): Latex table template string.

        data_path (str): Model setup save directory

        parameters (dict): Model's parameters.
        specification (dict): Model's functional specification.
        grids (dict): Model's grids.
        variables (dict): Model's variables.
    """

    __ltbl_prototype__ = r"""\begin{subtable}[t]{.53\textwidth}
\centering
\footnotesize
\begin{tabularx}{\textwidth}[t]{Xllll}\toprule
\multicolumn{5}{c}{\textbf{Grid Parameterization}} \\\midrule
\textbf{Grid} & \textbf{Points} & \textbf{Min} & \textbf{Max} & \textbf{Weight} \\\midrule
wealth & $x_size & $xmin & $xmax & $xc \\\hdashline
radius & $r_size & $rmin & $rmax & $rc \\\hdashline
\multirow{ 2}{*}{quantity} & \multirow{ 2}{*}{$qn} & \multirow{ 2}{*}{$qmin} & $qmax &
    \multirow{2}{*}{$qc} \\
&  & & (120000) &  \\\hdashline
\multirow{ 2}{*}{effort} & \multirow{ 2}{*}{$sn} & \multirow{ 2}{*}{$smin} & $smax &
    \multirow{2}{*}{$sc} \\
&  & & (1.0) &  \\\bottomrule\bottomrule
\end{tabularx}
\end{subtable}\hfill
\begin{subtable}[t]{.45\textwidth}
\centering
\footnotesize
\begin{tabularx}{\textwidth}[t]{Xl}\toprule
\multicolumn{2}{c}{\textbf{Model Parameterization}} \\\midrule
\textbf{Parameter} & \textbf{Value} \\\midrule
discount factor ($$\beta$$) & $beta \\\hdashline
radius persistence ($$\delta$$) & $delta \\\hdashline
attentional costs ($$\alpha$$) & $alpha \\\hdashline
complementarities factor ($$\gamma$$) & $gamma \\\hdashline
returns ($$R$$) & $$\frac{1}{\beta}$$ \\\bottomrule\bottomrule
\end{tabularx}
\end{subtable}"""

    data_path = None

    parameters = {}
    specification = {}
    grids = {}
    variables = {}

    def __parse_fnc__(self, spec):
        """Prepare functional specification string."""

        output_str = spec.split("=")[1]
        output_str = output_str.replace(
            "v->m->alpha", "{}".format(self.parameters["alpha"])
        )
        output_str = output_str.replace(
            "v->m->gamma", "{}".format(self.parameters["gamma"])
        )
        output_str = output_str.replace(
            "v->m->delta", "{}".format(self.parameters["delta"])
        )
        output_str = output_str.replace("v->m->R", "{}".format(self.parameters["R"]))
        output_str = output_str.replace("exp", "np.exp")
        output_str = output_str.replace("v->q", "q")
        output_str = output_str.replace("v->r", "r")
        output_str = output_str.replace("v->s", "s")
        output_str = output_str.replace("v->x", "x")
        return output_str.strip()

    def __init__(self, save_dir):
        """Constructor.

        Args:
            save_dir (str): Model setup save directory.
        """

        self.data_path = rad_conf.RAD_DATA_DIR + "/" + save_dir

        def construct_if_data_exist(data_path):
            if os.path.isfile(data_path):
                return Grid(data_path)
            else:
                raise RuntimeError(
                    "Data path '{}' does not exist. Consider executing 'rad_msol'.".format(
                        data_path
                    )
                )

        self.grids["x"] = construct_if_data_exist(self.data_path + "/xg")
        self.grids["r"] = construct_if_data_exist(self.data_path + "/rg")
        self.grids["q"] = construct_if_data_exist(self.data_path + "/qg")
        self.grids["s"] = construct_if_data_exist(self.data_path + "/sg")

        self.variables["v0"] = Variable(
            self.grids["x"], self.grids["r"], datafile=self.data_path + "/v0"
        )
        self.variables["v1"] = Variable(
            self.grids["x"], self.grids["r"], datafile=self.data_path + "/v1"
        )
        self.variables["s"] = Variable(
            self.grids["x"], self.grids["r"], datafile=self.data_path + "/spol"
        )
        self.variables["q"] = Variable(
            self.grids["x"], self.grids["r"], datafile=self.data_path + "/qpol"
        )

        with open(self.data_path + "/model", "rb") as file_handle:
            self.parameters["alpha"] = struct.unpack("d", file_handle.read(8))[0]
            self.parameters["beta"] = struct.unpack("d", file_handle.read(8))[0]
            self.parameters["delta"] = struct.unpack("d", file_handle.read(8))[0]
            self.parameters["gamma"] = struct.unpack("d", file_handle.read(8))[0]
            self.parameters["R"] = struct.unpack("d", file_handle.read(8))[0]

        with open(self.data_path + "/fncs", "r") as file_handle:
            while True:
                line = file_handle.readline()
                if not line:
                    break
                if line.startswith("util"):
                    self.specification["util"] = {"str": self.__parse_fnc__(line[:-1])}
                    self.specification["util"]["fnc"] = eval(
                        "lambda q, r, s: " + self.specification["util"]["str"]
                    )
                elif line.startswith("cost"):
                    self.specification["cost"] = {"str": self.__parse_fnc__(line[:-1])}
                    self.specification["cost"]["fnc"] = eval(
                        "lambda r, s: " + self.specification["cost"]["str"]
                    )
                elif line.startswith("radt"):
                    self.specification["radt"] = {"str": self.__parse_fnc__(line[:-1])}
                    self.specification["radt"]["fnc"] = eval(
                        "lambda r, s: " + self.specification["radt"]["str"]
                    )
                elif line.startswith("wltt"):
                    self.specification["wltt"] = {"str": self.__parse_fnc__(line[:-1])}
                    self.specification["wltt"]["fnc"] = eval(
                        "lambda q, r, s, x: " + self.specification["wltt"]["str"]
                    )

    def model_string(self):
        """Get a brace-nested, string description of the model object."""

        mask = """model={{
                .alpha={}, .beta={}, .delta={}, .gamma={}, .R={}, .util={}, .cost={},
                .radt={}, .wltt={}}}"""
        return mask.format(
            self.parameters["alpha"],
            self.parameters["beta"],
            self.parameters["delta"],
            self.parameters["gamma"],
            self.parameters["R"],
            self.specification["util"]["str"],
            self.specification["cost"]["str"],
            self.specification["radt"]["str"],
            self.specification["wltt"]["str"],
        )

    def get_radius_dynamics(self):
        """Calculate the radius transition and return it as a Variable object."""

        radt = np.zeros((self.grids["x"].data.size, self.grids["r"].data.size))
        for x_idx in range(0, len(self.grids["x"].data)):
            for r_idx, r_val in enumerate(self.grids["r"].data):
                radt[r_idx, x_idx] = self.specification["radt"]["fnc"](
                    r_val, self.variables["s"].data[r_idx, x_idx]
                )
        return Variable(self.grids["x"], self.grids["r"], zvar=radt)

    def get_wealth_dynamics(self):
        """Calculate the wealth transition and return it as a Variable object."""

        wltt = np.zeros((self.grids["x"].data.size, self.grids["r"].data.size))
        for x_idx, x_val in enumerate(self.grids["x"].data):
            for r_idx, r_val in enumerate(self.grids["r"].data):
                wltt[r_idx, x_idx] = self.specification["wltt"]["fnc"](
                    self.variables["q"].data[r_idx, x_idx],
                    r_val,
                    self.variables["s"].data[r_idx, x_idx],
                    x_val,
                )
        return Variable(self.grids["x"], self.grids["r"], zvar=wltt)

    def save_latex_table(self, filename):
        """Create and save a latex table with the model and solution's parameterizations."""

        with open(rad_conf.RAD_TEMP_DIR + "/" + filename, "w") as file_handle:
            file_handle.write(
                Template(self.__ltbl_prototype__).substitute(
                    x_size=len(self.grids["x"].data),
                    xmin="{:g}".format(self.grids["x"].data[0]),
                    xmax="{:g}".format(self.grids["x"].data[-1]),
                    xc="{:g}".format(self.grids["x"].weight),
                    r_size=len(self.grids["r"].data),
                    rmin="{:g}".format(self.grids["r"].data[0]),
                    rmax="{:g}".format(self.grids["r"].data[-1]),
                    rc="{:g}".format(self.grids["r"].weight),
                    qn=len(self.grids["q"].data),
                    qmin="{:g}".format(self.grids["q"].data[0]),
                    qmax="{:g}".format(self.grids["q"].data[-1]),
                    qc="{:g}".format(self.grids["q"].weight),
                    sn=len(self.grids["s"].data),
                    smin="{:g}".format(self.grids["s"].data[0]),
                    smax="{:g}".format(self.grids["s"].data[-1]),
                    sc="{:g}".format(self.grids["s"].weight),
                    beta=self.parameters["beta"],
                    delta=self.parameters["delta"],
                    alpha=self.parameters["alpha"],
                    gamma=self.parameters["gamma"],
                )
            )

    def save_learning_rsections(self, rsections=None):
        """Create plot and save as png the learning dynamics' r sections."""

        if rsections is None:
            rsections = np.linspace(0, 1, num=5)
        sdom = np.linspace(0, 3, num=100)

        fig = plt.figure()
        axes = fig.gca()
        axes.set_prop_cycle(rad_conf.RAD_FIG_CYCLE)
        axes.plot(sdom, np.ones(100), linestyle=":", color="black", linewidth=1)
        for r_idx, r_val in enumerate(rsections):
            axes.plot(
                sdom,
                self.specification["radt"]["fnc"](r_val, sdom),
                label="$r_{}={:.2f}$".format(r_idx, r_val),
            )
        plt.legend(**rad_conf.RAD_LEGEND_PROPERTIES)
        axes.set_xlabel("s")
        axes.set_ylabel("r'")
        filename = rad_conf.RAD_TEMP_DIR + "/l_rsections.png"
        fig.savefig(filename, **rad_conf.RAD_FIG_PROPERTIES)

        return filename

    def save_cost_rsections(self, rsections=None):
        """Create plot and save as png the cost functions' r sections."""

        if rsections is None:
            rsections = np.linspace(0, 1, num=5)
        sdom = np.linspace(0, 3, num=100)

        fig = plt.figure()
        axes = fig.gca()
        axes.set_prop_cycle(rad_conf.RAD_FIG_CYCLE)
        for r_idx, r_val in enumerate(rsections):
            axes.plot(
                sdom,
                self.specification["cost"]["fnc"](r_val, sdom),
                label="$r_{}={:.2f}$".format(r_idx, r_val),
            )
        plt.legend(**rad_conf.RAD_LEGEND_PROPERTIES)
        axes.set_xlabel("s")
        axes.set_ylabel("c")
        filename = rad_conf.RAD_TEMP_DIR + "/c_rsections.png"
        fig.savefig(filename, **rad_conf.RAD_FIG_PROPERTIES)

        return filename


class ParameterDependence:
    """Parameter dependence class.

    Comprises of functionality relevant for parameter dependence analysis of the radial attention
    model.

    Attributes:
        parameter_string (str): Parameter name.
        data_files (list): List of data filenames.
    """

    parameter_string = None
    data_files = None

    def __init__(self, parameter_string):
        """Constructor.

        Args:
            parameter_string (str): Parameter name.
        """

        self.parameter_string = parameter_string

        self.data_files = [
            file.replace(rad_conf.RAD_DATA_DIR + "/", "")
            for file in glob.glob(
                rad_conf.RAD_DATA_DIR
                + "/{}/{}*".format(self.parameter_string, self.parameter_string)
            )
        ]
        self.data_files.sort()

    def save_fig(self, domain_data, ylab, yval):
        """Create, show and save in png format the figures with the responses of the value function
        and the optimal controls on the particular parameter described by the passed the domain
        data.
        """

        fig = plt.figure()
        axes = fig.gca()
        axes.set_prop_cycle(rad_conf.RAD_FIG_CYCLE)
        axes.set_xlabel("$\\{}$".format(self.parameter_string))
        axes.set_ylabel(ylab[0])
        for r_idx in range(0, len(domain_data["r_indices"])):
            for x_idx in range(0, len(domain_data["x_indices"])):
                g_idx = r_idx * len(domain_data["x_indices"]) + x_idx
                smoothed = gaussian_filter1d(yval[g_idx, :], sigma=1)
                axes.plot(
                    domain_data["param_grid"],
                    smoothed,
                    label="$r_{}={:.2f},x_{}={:.2f}$".format(
                        r_idx,
                        domain_data["r_points"][g_idx],
                        x_idx,
                        domain_data["x_points"][g_idx],
                    ),
                )
        plt.legend(**rad_conf.RAD_LEGEND_PROPERTIES)
        fig.savefig(
            rad_conf.RAD_TEMP_DIR + "/{}_on_{}.png".format(self.parameter_string, ylab),
            **rad_conf.RAD_FIG_PROPERTIES,
        )

    def save_figs(self, r_indices=None, x_indices=None):
        """Create, show and save in png format the figures with the responses of the value function
        and the optimal controls on value changes of each parameter.
        """

        if r_indices is None or x_indices is None:
            model = RadialAttentionModel(self.data_files[0])
            if r_indices is None:
                r_indices = model.grids["r"].eqpart(4)[1:3]
            if x_indices is None:
                x_indices = model.grids["x"].eqpart(4)[1:3]

        index_count = len(r_indices) * len(x_indices)
        point_count = len(self.data_files)

        domain_data = {
            "param_grid": np.zeros(point_count),
            "r_indices": r_indices,
            "r_points": np.zeros(index_count),
            "x_indices": x_indices,
            "x_points": np.zeros(index_count),
        }
        range_data = {
            "spol": np.zeros((index_count, point_count)),
            "qpol": np.zeros((index_count, point_count)),
            "v": np.zeros((index_count, point_count)),
        }

        for p_idx, p_val in enumerate(self.data_files):
            model = RadialAttentionModel(p_val)
            domain_data["param_grid"][p_idx] = model.parameters[self.parameter_string]
            for r_idx, r_val in enumerate(r_indices):
                for x_idx, x_val in enumerate(x_indices):
                    g_idx = r_idx * len(x_indices) + x_idx
                    domain_data["r_points"][g_idx] = model.grids["r"].data[r_val]
                    domain_data["x_points"][g_idx] = model.grids["x"].data[x_val]
                    range_data["spol"][g_idx, p_idx] = model.variables["s"].data[
                        r_val, x_val
                    ]
                    range_data["qpol"][g_idx, p_idx] = model.variables["q"].data[
                        r_val, x_val
                    ]
                    range_data["v"][g_idx, p_idx] = model.variables["v0"].data[
                        r_val, x_val
                    ]

        for key, value in range_data.items():
            self.save_fig(domain_data, key, value)
