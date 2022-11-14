#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import xlrd
import argparse
import pathlib

# Parameters to produce good looking LaTeX figures
matplotlib.rcParams['mathtext.fontset'] = 'cm'
matplotlib.rcParams['font.family'] = 'serif'


class Cop:
    # some parameters used in the benchmark
    def __init__(self):
        self.F = 15e-9  # DRAM metal half-pitch
        self.pitch = 4*self.F    # contacted gate pitch
        self.lic = 10*self.pitch   # typical length of an electrical interconnect
        self.clenic = 5e-10    # effective capacitance per interconnect length F/m
        self.Cicle = 9.2322e-11
        self.Cic = self.lic*self.Cicle
        self.Ric = 2000/3
        self.factic = 0.7  # from S. Rakheja paper, = 0.7 interconnect cap
        self.facEcap = 0.5  # normal capacitor
        self.factinv = 0.9
        self.facEinv = 1.4
        self.factnan = 2.0
        self.facEnan = 2.0
        self.isspiking = 'n'
        self.factasyn = 3   # synapse
        self.factaneu = 3   # neuron
        self.factabun = 2   # bundle
        self.factachip = 2   # chip
        self.nlayers = np.array([1])
        self.neuperlayer = np.array([1])
        self.synperneu = np.array([1])
        self.inneuperlayer = np.array([1])
        self.integration = np.array([])
        self.fire = np.array([])
        self.shareact = np.array([])
        self.spikeAct = np.array([])


eps = 0


class ArchLevel:
    # all the possible parameters used in the chip benchmark, some parameters are not used
    def __init__(self):
        # neuron
        self.aneu = 0
        self.tneu = eps
        self.Eneu = 0
        self.Pneu = 0
        self.Sneu = 0
        # synapse
        self.asyn = 0
        self.tsyn = eps
        self.Esyn = 0
        self.Psyn = 0
        self.Ssyn = 0
        self.Vsyn = 0
        self.Isyn = 0
        self.ticsyn = 0
        # global interconnect
        self.agic = 0
        self.tgic = eps
        self.Egic = 0
        self.Pgic = 0
        self.Sgic = 0
        self.lgic = 0
        # chip per timestep
        self.CPneu = 0
        self.CPsyn = 0
        self.CPmem = 0
        self.chneu = 0
        self.chsyn = 0
        self.chmem = 0
        self.ach = 0
        self.tch = eps
        self.Ech = 0
        self.Pch = 0
        self.Sch = 0
        # local interconnect
        self.alic = 0
        self.tlic = eps
        self.Elic = 0
        self.Plic = 0
        self.Slic = 0
        self.llic = 0
        # firing rate
        self.firerate = 0
        # share of activities
        self.shareact = 1
        # timestep frequency
        self.stepfreq = 0
        # synaptic throughput per chip
        self.Thruput = 0
        # throughput comp primitives
        self.Thruarea = 0
        # interconnect time
        self.tlocic = 0
        # neuron current
        self.Ineu = 0
        # voltage
        self.Vdev = 1
        # number of cores
        self.ncores = 1
        # neuron max fanin for cascading
        self.fanin = 32
        self.consecfanin = 0
        # computational primitive
        self.CPneu = 0
        self.CPsyn = 0
        self.aCP = 0
        self.tCP = 0
        self.ECP = 0
        self.PCP = 0
        self.Thruarea = 0


class Layer(object):
    # parameters of one layer, iterated in the simulation
    def __init__(self):
        self.neupercore = 0
        self.ncores = 0
        self.synperneu = 0
        self.inneuperlayer = 0
        self.shareact = 0
        self.chneu = 0
        self.chsyn = 0
        self.ach = 0
        self.tch = 0
        self.Ech = 0
        self.alic = 0
        self.tlic = 0
        self.Elic = 0
        self.agic = 0
        self.tgic = 0
        self.Egic = 0
        self.asyn = 0
        self.tsyn = 0
        self.Esyn = 0
        self.aneu = 0
        self.tneu = 0
        self.Eneu = 0
        self.Esyntot = 0
        self.Thruput = 0
        self.inneuperlayer = 0
        self.Pch = 0
        self.Ech = 0
        self.Thruput = 0
        self.spikeAct = 0


class bey(object):
    # Parameters for a single device. Both, neurons and synapses, use this
    def __init__(self):
        # intrinsic device
        self.aint = 0
        self.tint = 0
        self.Eint = 0
        # short interconnect
        self.aic = 0
        self.tic = 0
        self.Eic = 0
        # RAM
        self.aram = 0
        self.tram = 0
        self.Eram = 0
        # current, voltage, resistence
        self.Idev = 0
        self.Vdev = 0
        self.Ron = 0
        self.Q = 0
        self.Cload = 0


def neuIntercon(cop, arcir, volt, tshort, Ineu, inter_core):
    # parameters calculated for interconnects
    longlic = np.sqrt(arcir)
    flong = longlic/cop.lic
    Eic = cop.clenic*volt**2*longlic
    if inter_core:  # Is the interconnect inside a core
        tic = tshort*flong
    else:  # Is the interconnect between cores, ie, across a chip
        tic = Eic/volt/Ineu
    alic = longlic*4*cop.F
    return alic, tic, Eic, longlic


def Box_cal(beyA, layer, cop, method, device_cal):
    # detail: calculate the performance core by core
    if method == 'detail':
        layer.chneu = layer.neupercore + layer.inneuperlayer
        layer.chsyn = layer.neupercore * layer.synperneu
    # average:  calculate the performance layer by layer, average the integration and fire by layer
    elif method == 'average':
        layer.chneu = layer.neupercore * layer.ncores + layer.inneuperlayer
        layer.chsyn = layer.neupercore * layer.synperneu * layer.ncores
    act_synapse = layer.shareact * layer.synperneu

    a_temp1 = (layer.neupercore * beyA.aneu + layer.inneuperlayer *
               layer.neupercore*beyA.asyn)*cop.factabun
    a_temp2 = (layer.neupercore * beyA.aneu + layer.synperneu *
               layer.neupercore*beyA.asyn)*cop.factabun
    layer.ach = max(a_temp1, a_temp2)
    ach = layer.ach * layer.ncores
    #beyA.stepfreq = 1/beyA.tsyn/2
    #beyA.firerate = beyA.stepfreq/layer.synperneu
    asynef = beyA.asyn * layer.synperneu * layer.neupercore
    layer.alic, layer.tlic, layer.Elic, layer.llic = neuIntercon(
        cop, asynef, beyA.Vsyn, beyA.ticsyn, beyA.Isyn, inter_core=True)
    if device_cal != 'all' and device_cal != 'syn-ic':
        layer.Elic = 0
        layer.tlic = 0
    layer.agic, layer.tgic, layer.Egic, layer.lgic = neuIntercon(
        cop, ach, beyA.Vdev, beyA.tlocic, beyA.Ineu, inter_core=False)
    if device_cal != 'all' and device_cal != 'neu-ic':
        layer.Egic = 0
        layer.tgic = 0
    layer.tsyn = beyA.tsyn + layer.tlic
    layer.Esyn = beyA.Esyn + layer.Elic
    layer.tneu = beyA.tneu + layer.tgic
    layer.Eneu = beyA.Eneu + layer.Egic
    layer.tch = layer.tneu + layer.tsyn
    layer.Esyntot = layer.Esyn + layer.Eneu/act_synapse
    layer.Ech = (layer.chneu*layer.Eneu+layer.chsyn *
                 layer.Esyn*layer.shareact)*layer.spikeAct
    return beyA, layer


def workload_performance(beyA, cop, layer, method, device_cal):
    # the 'Energy' is an array containing layer detailed energy consumption
    if device_cal != 'all' and device_cal != 'neuron':
        beyA.Eneu = 0
        beyA.tneu = 0
    if device_cal != 'all' and device_cal != 'synapse':
        beyA.Esyn = 0
        beyA.tsyn = 0
    llic = []
    lgic = []
    Energy = []
    latency = []
    if method == 'detail':
        layer_number = len(cop.synperneu)
        for i in range(layer_number):
            t = []
            E_temp = 0
            llic_temp = []
            lgic_temp = []
            for j in range(cop.nlayers[i]):
                index = sum(cop.nlayers[0:i])+j
                layer.synperneu = cop.synperneu[i]
                layer.neupercore = cop.neuperlayer[i]
                layer.ncores = cop.nlayers[i]
                layer.inneuperlayer = cop.inneuperlayer[i]
                layer.spikeAct = cop.spikeAct[index]
                layer.shareact = cop.shareact[index]
                beyA, layer = Box_cal(beyA, layer, cop, method, device_cal)
                beyA.ECP = beyA.ECP + layer.Ech
                beyA.aCP = beyA.aCP + layer.ach
                t.append(layer.tch)
                E_temp = E_temp + layer.Ech
                llic_temp.append(layer.llic)
                lgic_temp.append(layer.lgic)
            Energy.append(E_temp)
            beyA.tCP = beyA.tCP + max(t)
            latency.append(max(t))
            llic.append(max(llic_temp))
            lgic.append(max(lgic_temp))
        beyA.PCP = beyA.ECP/beyA.tCP
        beyA.Thruarea = 1/beyA.tCP/beyA.aCP

    elif method == 'average':
        layer_number = len(cop.synperneu)
        for i in range(layer_number):
            layer.synperneu = cop.synperneu[i]
            layer.ncores = cop.nlayers[i]
            layer.neupercore = cop.neuperlayer[i]
            layer.inneuperlayer = cop.inneuperlayer[i]
            layer.spikeAct = cop.spikeAct[i]
            layer.shareact = cop.shareact[i]
            beyA, layer = Box_cal(beyA, layer, cop, method)
            beyA.ECP = beyA.ECP + layer.Ech
            beyA.tCP = beyA.tCP + layer.tch
            beyA.aCP = beyA.aCP + layer.ach*cop.nlayers[i]
        beyA.PCP = beyA.ECP/beyA.tCP
        beyA.Thruarea = 1/beyA.tCP/beyA.aCP
    return beyA, Energy, latency


# shareact:  integration / neuron per layer  # the active synapse per neuron
# spikeAct:  fire / neuron per layer         #  fire rate of neurons per integration

def network_param(method, workload):
    # for different benchmarking method and workload, calculate the shareact and spikeAct
    cop = Cop()
    if workload == 'large_lenet':
        cop.nlayers = np.array([1, 32, 32, 48, 48, 1, 1, 1])
        cop.neuperlayer = np.array([784, 784, 196, 100, 25, 120, 84, 100])
        cop.synperneu = np.array([1, 22.90, 4, 800, 4, 1200, 120, 84])
        cop.inneuperlayer = np.array([1, 784, 784, 196, 100, 25, 120, 84])
        if method == 'detail':
            cop.integration = large_lenet[:, 0]
            cop.fire = large_lenet[:, 1]
            layer_number = len(cop.synperneu)
            # number = sum(cop.nlayers)
            cop.shareact = np.empty_like(cop.fire)
            cop.spikeAct = np.empty_like(cop.fire)
            for i in range(layer_number):
                for j in range(cop.nlayers[i]):
                    index = sum(cop.nlayers[0:i])+j
                    cop.shareact[index] = cop.integration[index] / \
                        cop.neuperlayer[i]
                    cop.spikeAct[index] = cop.fire[index]/cop.neuperlayer[i]

        elif method == 'average':
            int1 = np.average(large_lenet[0, 0])
            int2 = np.average(large_lenet[1:33, 0])
            int3 = np.average(large_lenet[33:65, 0])
            int4 = np.average(large_lenet[65:113, 0])
            int5 = np.average(large_lenet[113:161, 0])
            int6 = np.average(large_lenet[161, 0])
            int7 = np.average(large_lenet[162, 0])
            int8 = np.average(large_lenet[163, 0])
            cop.integration = np.array(
                [int1, int2, int3, int4, int5, int6, int7, int8])
            fir1 = np.average(large_lenet[0, 1])
            fir2 = np.average(large_lenet[1:33, 1])
            fir3 = np.average(large_lenet[33:65, 1])
            fir4 = np.average(large_lenet[65:113, 1])
            fir5 = np.average(large_lenet[113:161, 1])
            fir6 = np.average(large_lenet[161, 1])
            fir7 = np.average(large_lenet[162, 1])
            fir8 = np.average(large_lenet[163, 1])
            cop.fire = np.array(
                [fir1, fir2, fir3, fir4, fir5, fir6, fir7, fir8])
            cop.shareact = cop.integration/cop.neuperlayer
            cop.spikeAct = cop.fire/cop.neuperlayer

    elif workload == 'large_lenet_fashion':
        cop.nlayers = np.array([1, 32, 32, 48, 48, 1, 1, 1])
        cop.neuperlayer = np.array([784, 784, 196, 100, 25, 120, 84, 100])
        cop.synperneu = np.array([1, 22.90, 4, 800, 4, 1200, 120, 84])
        cop.inneuperlayer = np.array([1, 784, 784, 196, 100, 25, 120, 84])
        if method == 'detail':
            cop.integration = large_lenet_fashion[:, 0]
            cop.fire = large_lenet_fashion[:, 1]
            layer_number = len(cop.synperneu)
            # number = sum(cop.nlayers)
            cop.shareact = np.empty_like(cop.fire)
            cop.spikeAct = np.empty_like(cop.fire)
            for i in range(layer_number):
                for j in range(cop.nlayers[i]):
                    index = sum(cop.nlayers[0:i])+j
                    cop.shareact[index] = cop.integration[index] / \
                        cop.neuperlayer[i]
                    cop.spikeAct[index] = cop.fire[index]/cop.neuperlayer[i]
        elif method == 'average':
            int1 = np.average(large_lenet_fashion[0, 0])
            int2 = np.average(large_lenet_fashion[1:33, 0])
            int3 = np.average(large_lenet_fashion[33:65, 0])
            int4 = np.average(large_lenet_fashion[65:113, 0])
            int5 = np.average(large_lenet_fashion[113:161, 0])
            int6 = np.average(large_lenet_fashion[161, 0])
            int7 = np.average(large_lenet_fashion[162, 0])
            int8 = np.average(large_lenet_fashion[163, 0])
            cop.integration = np.array(
                [int1, int2, int3, int4, int5, int6, int7, int8])
            fir1 = np.average(large_lenet_fashion[0, 1])
            fir2 = np.average(large_lenet_fashion[1:33, 1])
            fir3 = np.average(large_lenet_fashion[33:65, 1])
            fir4 = np.average(large_lenet_fashion[65:113, 1])
            fir5 = np.average(large_lenet_fashion[113:161, 1])
            fir6 = np.average(large_lenet_fashion[161, 1])
            fir7 = np.average(large_lenet_fashion[162, 1])
            fir8 = np.average(large_lenet_fashion[163, 1])
            cop.fire = np.array(
                [fir1, fir2, fir3, fir4, fir5, fir6, fir7, fir8])
            cop.shareact = cop.integration/cop.neuperlayer
            cop.spikeAct = cop.fire/cop.neuperlayer

    elif workload == 'small_lenet':
        cop.nlayers = np.array([1, 6, 6, 16, 16, 1, 1, 1])
        cop.neuperlayer = np.array([784, 784, 196, 100, 25, 120, 84, 100])
        cop.synperneu = np.array([1, 22.90, 4, 150, 4, 400, 120, 84])
        cop.inneuperlayer = np.array([1, 784, 784, 196, 100, 25, 120, 84])
        if method == 'detail':
            cop.integration = small_lenet[:, 0]
            cop.fire = small_lenet[:, 1]
            layer_number = len(cop.synperneu)
            # number = sum(cop.nlayers)
            cop.shareact = np.empty_like(cop.fire)
            cop.spikeAct = np.empty_like(cop.fire)
            for i in range(layer_number):
                for j in range(cop.nlayers[i]):
                    index = sum(cop.nlayers[0:i])+j
                    cop.shareact[index] = cop.integration[index] / \
                        cop.neuperlayer[i]
                    cop.spikeAct[index] = cop.fire[index]/cop.neuperlayer[i]
        elif method == 'average':
            int1 = np.average(small_lenet[0, 0])
            int2 = np.average(small_lenet[1:7, 0])
            int3 = np.average(small_lenet[7:13, 0])
            int4 = np.average(small_lenet[13:29, 0])
            int5 = np.average(small_lenet[29:45, 0])
            int6 = np.average(small_lenet[45, 0])
            int7 = np.average(small_lenet[46, 0])
            int8 = np.average(small_lenet[47, 0])
            cop.integration = np.array(
                [int1, int2, int3, int4, int5, int6, int7, int8])
            fir1 = np.average(small_lenet[0, 1])
            fir2 = np.average(small_lenet[1:7, 1])
            fir3 = np.average(small_lenet[7:13, 1])
            fir4 = np.average(small_lenet[13:29, 1])
            fir5 = np.average(small_lenet[29:45, 1])
            fir6 = np.average(small_lenet[45, 1])
            fir7 = np.average(small_lenet[46, 1])
            fir8 = np.average(small_lenet[47, 1])
            cop.fire = np.array(
                [fir1, fir2, fir3, fir4, fir5, fir6, fir7, fir8])
            cop.shareact = cop.integration/cop.neuperlayer
            cop.spikeAct = cop.fire/cop.neuperlayer
    elif workload == 'small_lenet_fashion':
        cop.nlayers = np.array([1, 6, 6, 16, 16, 1, 1, 1])
        cop.neuperlayer = np.array([784, 784, 196, 100, 25, 120, 84, 100])
        cop.synperneu = np.array([1, 22.90, 4, 150, 4, 400, 120, 84])
        cop.inneuperlayer = np.array([1, 784, 784, 196, 100, 25, 120, 84])
        if method == 'detail':
            cop.integration = small_lenet_fashion[:, 0]
            cop.fire = small_lenet_fashion[:, 1]
            layer_number = len(cop.synperneu)
            # number = sum(cop.nlayers)
            cop.shareact = np.empty_like(cop.fire)
            cop.spikeAct = np.empty_like(cop.fire)
            for i in range(layer_number):
                for j in range(cop.nlayers[i]):
                    index = sum(cop.nlayers[0:i])+j
                    cop.shareact[index] = cop.integration[index] / \
                        cop.neuperlayer[i]
                    cop.spikeAct[index] = cop.fire[index]/cop.neuperlayer[i]
        elif method == 'average':
            int1 = np.average(small_lenet_fashion[0, 0])
            int2 = np.average(small_lenet_fashion[1:7, 0])
            int3 = np.average(small_lenet_fashion[7:13, 0])
            int4 = np.average(small_lenet_fashion[13:29, 0])
            int5 = np.average(small_lenet_fashion[29:45, 0])
            int6 = np.average(small_lenet_fashion[45, 0])
            int7 = np.average(small_lenet_fashion[46, 0])
            int8 = np.average(small_lenet_fashion[47, 0])
            cop.integration = np.array(
                [int1, int2, int3, int4, int5, int6, int7, int8])
            fir1 = np.average(small_lenet_fashion[0, 1])
            fir2 = np.average(small_lenet_fashion[1:7, 1])
            fir3 = np.average(small_lenet_fashion[7:13, 1])
            fir4 = np.average(small_lenet_fashion[13:29, 1])
            fir5 = np.average(small_lenet_fashion[29:45, 1])
            fir6 = np.average(small_lenet_fashion[45, 1])
            fir7 = np.average(small_lenet_fashion[46, 1])
            fir8 = np.average(small_lenet_fashion[47, 1])
            cop.fire = np.array(
                [fir1, fir2, fir3, fir4, fir5, fir6, fir7, fir8])
            cop.shareact = cop.integration/cop.neuperlayer
            cop.spikeAct = cop.fire/cop.neuperlayer
    return cop


def load_xls_data(path):
    rows = []
    xls_data = xlrd.open_workbook(path)
    table = xls_data.sheet_by_index(0)
    for i in range(table.nrows):
        line = table.row_values(i)
        rows.append(line)
    return np.array(rows)


parser = argparse.ArgumentParser()
parser.add_argument('--path-ll', type=pathlib.Path, default="large_lenet.xls",
                    help='Path to Large LeNet XLS file')
parser.add_argument('--path-sl', type=pathlib.Path, default="small_lenet.xls",
                    help='Path to Small LeNet XLS file')
parser.add_argument('--path-llf', type=pathlib.Path, default="large_lenet_fashion.xls",
                    help='Path to Large LeNet Fashion XLS file')
parser.add_argument('--path-slf', type=pathlib.Path, default="small_lenet_fashion.xls",
                    help='Path to Small LeNet Fashion XLS file')
args = parser.parse_args()

# Importing all workloads
small_lenet = load_xls_data(args.path_sl)
large_lenet = load_xls_data(args.path_ll)
small_lenet_fashion = load_xls_data(args.path_slf)
large_lenet_fashion = load_xls_data(args.path_llf)

# Loading parameters for each architecture
bey_CMOSdsyn = bey()
bey_CMOSdsyn.aint = 0.00000000000138240000
bey_CMOSdsyn.tint = 0.00000000064416087938
bey_CMOSdsyn.Eint = 0.00000000000017057941
bey_CMOSdsyn.aic = 1.8e-14
bey_CMOSdsyn.tic = 3.563205636979483e-13
bey_CMOSdsyn.Eic = 1.772584254034601e-17
bey_CMOSdsyn.Vdev = 0.8
bey_CMOSdsyn.Idev = 9.890410958904109e-05
bey_CMOSdsyn.Ron = 8.088642659279780e+03

bey_CMOSdneu = bey()
bey_CMOSdneu.aint = 0.00000000011077560000
bey_CMOSdneu.tint = 0.00000000063234184301
bey_CMOSdneu.Eint = 0.00000000000013612376
bey_CMOSdneu.aic = 1.8e-14
bey_CMOSdneu.tic = 3.563205636979483e-13
bey_CMOSdneu.Eic = 1.772584254034601e-17
bey_CMOSdneu.Vdev = 0.8
bey_CMOSdneu.Idev = 9.890410958904109e-05
bey_CMOSdneu.Ron = 8.088642659279780e+03

bey_CMOSasyn = bey()
bey_CMOSasyn.aint = 0.00000000000016875000
bey_CMOSasyn.tint = 0.00000000001893119296
bey_CMOSasyn.Eint = 0.00000000000000192964
bey_CMOSasyn.aic = 1.8e-14
bey_CMOSasyn.tic = 2.070356721124175e-13
bey_CMOSasyn.Eic = 1.772584254034601e-17
bey_CMOSasyn.Vdev = 0.8
bey_CMOSasyn.Idev = 3.956164383561644e-04
bey_CMOSasyn.Ron = 2.022160664819945e+03

bey_CMOSaneu = bey()
bey_CMOSaneu.aint = 0.00000000000069120000
bey_CMOSaneu.tint = 0.00000000198848784409
bey_CMOSaneu.Eint = 0.00000000000013828333
bey_CMOSaneu.aic = 1.8e-14
bey_CMOSaneu.tic = 2.070356721124175e-13
bey_CMOSaneu.Eic = 1.772584254034601e-17
bey_CMOSaneu.Vdev = 0.8
bey_CMOSaneu.Idev = 3.956164383561644e-04
bey_CMOSaneu.Ron = 2.022160664819945e+03

cop = Cop()

bey_spinres = bey()
bey_spinres.aint = 4.5e-15
bey_spinres.tint = 2.68e-13
bey_spinres.Eint = 8.1e-17
bey_spinres.Ron = 6.073e3
bey_spinres.Idev = 1.8e-4
bey_spinres.Vdev = 1.125
Cload = 4.5e-15*9.8*8.8541878128e-12/1.8e-9
t1 = 0.38*cop.Ric*cop.Cic
t2 = cop.factic*bey_spinres.Ron*cop.Cic
t3 = cop.factic*cop.Ric*Cload
bey_spinres.aic = 1.8e-14
bey_spinres.tic = 1.059232558370846e-12
bey_spinres.Eic = 2.492696607236157e-18
bey_spinres.tic = t1+t2+t3
bey_spinres.Eic = cop.facEcap*cop.Cic*bey_spinres.Vdev**2
bey_spinres.aic = cop.lic*2*cop.F

bey_Mn3Ir = bey()
bey_Mn3Ir.aint = 20*(15e-9)**2
bey_Mn3Ir.tint = 1/(435e9)
bey_Mn3Ir.Eint = 1.55e-15
bey_Mn3Ir.Ron = 33.3
bey_Mn3Ir.Idev = 4.5e-3
bey_Mn3Ir.Vdev = 0.15

cop.clenic*bey_Mn3Ir.Vdev*1e-6/bey_Mn3Ir.Idev

bey_NiO = bey()
bey_NiO.aint = 20*(15e-9)**2
bey_NiO.tint = 1/(20e9)
bey_NiO.Eint = 1.5e-14
bey_NiO.Ron = 1111
bey_NiO.Idev = 9e-4
bey_NiO.Vdev = 1

cop.clenic*bey_NiO.Vdev*1e-6/bey_NiO.Idev

# Data to include all the results
method = 'detail'
neuron_name0 = ['Mn3Ir', 'NiO', 'CMOSana', 'CMOSdig']
workload0 = ['small_lenet', 'small_lenet_fashion',
             'large_lenet', 'large_lenet_fashion']
device_cal0 = ['all', 'neuron', 'synapse', 'neu-ic', 'syn-ic']
Energy_mat = np.empty([len(device_cal0), len(neuron_name0), len(workload0), 8])
latency_mat = np.empty_like(Energy_mat)
data = np.empty([len(device_cal0), len(neuron_name0), len(workload0), 3])

for i in range(len(device_cal0)):
    device_cal = device_cal0[i]
    for j in range(len(neuron_name0)):
        neuron_name = neuron_name0[j]
        for k in range(len(workload0)):
            workload = workload0[k]
            cop = network_param(method, workload)
            layer = Layer()
            beyA = ArchLevel()
            if neuron_name == 'CMOSana':
                synapse_name = 'CMOSana'
            elif neuron_name == 'CMOSdig':
                synapse_name = 'CMOSdig'
            else:
                synapse_name = 'spinres'
            if synapse_name == 'CMOSdig':
                beyA.asyn = bey_CMOSdsyn.aint
                beyA.tsyn = bey_CMOSdsyn.tint
                beyA.Esyn = bey_CMOSdsyn.Eint
                beyA.Isyn = bey_CMOSdsyn.Idev
                beyA.Vsyn = bey_CMOSdsyn.Vdev
                beyA.ticsyn = bey_CMOSdsyn.tic
            elif synapse_name == 'spinres':
                beyA.asyn = bey_spinres.aint
                beyA.tsyn = bey_spinres.tint
                beyA.Esyn = bey_spinres.Eint
                beyA.Isyn = bey_spinres.Idev
                beyA.Vsyn = bey_spinres.Vdev
                beyA.ticsyn = bey_spinres.tic
            elif synapse_name == 'CMOSana':
                beyA.asyn = bey_CMOSasyn.aint
                beyA.tsyn = bey_CMOSasyn.tint
                beyA.Esyn = bey_CMOSasyn.Eint
                beyA.Isyn = bey_CMOSasyn.Idev
                beyA.Vsyn = bey_CMOSasyn.Vdev
                beyA.ticsyn = bey_CMOSasyn.tic

            if neuron_name == 'Mn3Ir':
                beyA.aneu = bey_Mn3Ir.aint
                beyA.tneu = bey_Mn3Ir.tint
                beyA.Eneu = bey_Mn3Ir.Eint
                beyA.Vdev = bey_Mn3Ir.Vdev
                beyA.Ineu = bey_Mn3Ir.Idev
                beyA.tlocic = bey_Mn3Ir.tic
            elif neuron_name == 'NiO':
                beyA.aneu = bey_NiO.aint
                beyA.tneu = bey_NiO.tint
                beyA.Eneu = bey_NiO.Eint
                beyA.Vdev = bey_NiO.Vdev
                beyA.Ineu = bey_NiO.Idev
                beyA.tlocic = bey_NiO.tic
            elif neuron_name == 'CMOSana':
                beyA.aneu = bey_CMOSaneu.aint
                beyA.tneu = bey_CMOSaneu.tint
                beyA.Eneu = bey_CMOSaneu.Eint
                beyA.Vdev = bey_CMOSaneu.Vdev
                beyA.Ineu = bey_CMOSaneu.Idev
                beyA.tlocic = bey_CMOSaneu.tic
            elif neuron_name == 'CMOSdig':
                beyA.aneu = bey_CMOSdneu.aint
                beyA.tneu = bey_CMOSdneu.tint
                beyA.Eneu = bey_CMOSdneu.Eint
                beyA.Vdev = bey_CMOSdneu.Vdev
                beyA.Ineu = bey_CMOSdneu.Idev
                beyA.tlocic = bey_CMOSdneu.tic
            beyA, Energy_temp, latency_temp = workload_performance(
                beyA, cop, layer, method, device_cal)
            data[i, j, k, :] = np.array([beyA.aCP, beyA.tCP, beyA.ECP])
            Energy_mat[i, j, k, :] = np.array(Energy_temp)
            latency_mat[i, j, k, :] = np.array(latency_temp)


# different color for different neurons
color = ['blue', 'red', 'black', 'violet']
marker = ['s', 'o', '<', '>']  # different marker for different workload
fig = plt.figure(figsize=(16, 10))
axes = fig.subplots(nrows=2, ncols=2)
# plt.subplot(2,2,2)
for i in np.arange(0, 2, 1):
    for j in range(4):
        axes[0, 1].scatter(np.arange(1, 9, 1), Energy_mat[0, i, j, :],
                           marker=marker[j], c='none', edgecolors=color[i], s=200)
axes[0, 1].set_xlabel('layer index \n (b)', fontsize=24)
axes[0, 1].set_ylabel('Energy (J)', fontsize=24)
axes[0, 1].tick_params(labelsize=26)
# axes[0,1].tick_params(labelsize= 26)
axes[0, 1].set_yscale('log')

# plt.subplot(2,2,1)
for i in np.arange(0, 2, 1):
    for j in range(4):
        axes[0, 0].scatter(np.arange(1, 5, 1), data[1:, i, j, -1],
                           marker=marker[j], c='none', edgecolors=color[i], s=200)

axes[0, 0].set_xlabel('\n (a)', fontsize=24)
axes[0, 0].set_ylabel('Energy (J)', fontsize=24)
axes[0, 0].set_xticklabels(
    ['0', 'neuron', 'synapse', 'neu-ic', 'syn-ic'], fontsize=26)
axes[0, 0].tick_params(labelsize=26)
axes[0, 0].set_yscale('log')


# plt.subplot(2,2,3)
for i in np.arange(0, 2, 1):
    for j in range(4):
        axes[1, 0].scatter(np.arange(1, 5, 1), data[1:, i, j, 1],
                           marker=marker[j], c='none', edgecolors=color[i], s=200)

axes[1, 0].set_xlabel('\n (c)', fontsize=24)
axes[1, 0].set_ylabel('Latency (s)', fontsize=24)
axes[1, 0].set_xticklabels(
    ['0', 'neuron', 'synapse', 'neu-ic', 'syn-ic'], fontsize=26)
axes[1, 0].tick_params(labelsize=26)
axes[1, 0].set_yscale('log')

for i in np.arange(0, 2, 1):
    for j in range(4):
        axes[1, 1].scatter(np.arange(1, 9, 1), latency_mat[0, i, j, :],
                           marker=marker[j], c='none', edgecolors=color[i], s=200)
axes[1, 1].set_xlabel('layer index \n (d)', fontsize=24)
axes[1, 1].set_ylabel('Latency (s)', fontsize=24)
axes[1, 1].tick_params(labelsize=26)
axes[1, 1].set_yscale('log')

fig.legend([r'Mn$_3$Ir SL', r'Mn$_3$Ir SLF', r'Mn$_3$Ir LL', r'Mn$_3$Ir LLF', r'NiO SL',
           r'NiO SLF', r'NiO LL', r'NiO LLF'], ncol=4, loc=8, bbox_to_anchor=(0.5, -0.15),
           fontsize=25)
plt.tight_layout()
plt.savefig('combine.eps', dpi=600, format='eps', bbox_inches='tight')
# plt.show()
