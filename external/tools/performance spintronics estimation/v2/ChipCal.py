# from asyncio import constants
import numpy as np
from Class import layer_level


def network_param(Consts, workload, data):
    # Consts = Param()
    if workload == 'large_lenet' or workload == 'large_lenet_fashion':
        Consts.nlayers = np.array([1, 32, 32, 48, 48, 1, 1, 1])
        Consts.neuperlayer = np.array([784, 784, 196, 100, 25, 120, 84, 100])
        Consts.synperneu = np.array([1, 22.90, 4, 800, 4, 1200, 120, 84])
        Consts.inneuperlayer = np.array([1, 784, 784, 196, 100, 25, 120, 84])

    elif workload == 'small_lenet' or workload == 'small_lenet_fashion':
        Consts.nlayers = np.array([1, 6, 6, 16, 16, 1, 1, 1])
        Consts.neuperlayer = np.array([784, 784, 196, 100, 25, 120, 84, 100])
        Consts.synperneu = np.array([1, 22.90, 4, 150, 4, 400, 120, 84])
        Consts.inneuperlayer = np.array([1, 784, 784, 196, 100, 25, 120, 84])
    Consts.integration = data[:, 0]
    Consts.fire = data[:, 1]
    layer_number = len(Consts.synperneu)
    # number = sum(Consts.nlayers)
    Consts.shareact = np.empty_like(Consts.fire)
    Consts.spikeAct = np.empty_like(Consts.fire)
    for i in range(layer_number):
        for j in range(Consts.nlayers[i]):
            index = sum(Consts.nlayers[0:i])+j
            Consts.shareact[index] = Consts.integration[index] / \
                Consts.neuperlayer[i]
            Consts.spikeAct[index] = Consts.fire[index]/Consts.neuperlayer[i]
    return Consts


def Interconnect(Const, acir, Ron, Idev, Cload, type):
    # Const = Param()\
    Volt = Const.Vdd
    len_ic = np.sqrt(acir)
    Eic = Const.cperl*len_ic*Volt**2
    Ric = Const.rperl*len_ic
    Cic = Const.cperl*len_ic
    # print(Ric)
    # print(Cic)
    if type == 'local':
        Tic = 0.69*(Ric*Cic+Ron*Cic+Ric*Cload)
    elif type == 'global':
        Tic = Eic/Volt/Idev
    Aic = len_ic*Const.pitch*2
    return Aic, Tic, Eic


def Layer_cal(Arch, Consts, Layer):
    # calculate the performance of one single layer, especially calculate the interconnect related information
    # neuron information is extracted from arch class
    # Arch = arch_level()
    # Consts = Param()
    # Layer = layer_level()
    Neu = Arch.neu
    Syn = Arch.syn
    Layer.neueffpercore = Layer.neupercore + Layer.inneuperlayer
    Layer.syneffpercore = Layer.neupercore * Layer.synperneu
    # act_synapse = Layer.shareact * Layer.synperneu
    A_temp1 = (Layer.neueffpercore * Neu.Adev * Consts.factaneu + Layer.inneuperlayer *
               Layer.neupercore * Syn.Adev * Consts.factasyn) * Consts.factabun
    A_temp2 = (Layer.neueffpercore * Neu.Adev * Consts.factaneu + Layer.synperneu *
               Layer.neupercore * Syn.Adev*Consts.factasyn) * Consts.factabun
    Layer.Acore = max(A_temp1, A_temp2)
    Alayer_esti = Layer.Acore * Layer.ncores
    # print(Layer.ncores)
    # print(Alayer_esti)
    Asynef = Syn.Adev * Layer.syneffpercore
    Layer.Aicloc, Layer.Ticloc, Layer.Eicloc = Interconnect(
        Consts, Asynef, Syn.Rondev, Syn.Idev, Syn.Cdev, 'local')
    Layer.Aicglo, Layer.Ticglo, Layer.Eicglo = Interconnect(
        Consts, Alayer_esti, Neu.Rondev, Neu.Idev, Neu.Cdev, 'global')
    # calculate the performance of each component of each core
    # local interconnect
    Layer.Aicloccore = Layer.Aicloc * Layer.syneffpercore
    Layer.Eicloccore = Layer.Eicloc * Layer.syneffpercore * \
        Layer.shareact * Layer.spikeAct
    Layer.Ticloccore = Layer.Ticloc
    # global interconnect
    Layer.Aicglocore = Layer.Aicloc * Layer.neueffpercore
    Layer.Eicglocore = Layer.Eicglo * Layer.neueffpercore * Layer.spikeAct
    Layer.Ticglocore = Layer.Ticglo
    # synapse
    Layer.Esyncore = Layer.syneffpercore * Syn.Edev * Layer.shareact * Layer.spikeAct
    Layer.Tsyncore = Syn.Tdev
    # neuron
    Layer.Eneucore = Layer.neueffpercore * Neu.Edev * Layer.spikeAct
    Layer.Tneucore = Neu.Tdev

    # add up to get the performance of the core
    Layer.Ecore = Layer.Eicloccore + Layer.Eicglocore + Layer.Esyncore + Layer.Eneucore
    Layer.Tcore = Layer.Ticloccore + Layer.Ticglocore + Layer.Tsyncore + Layer.Tneucore
    # Layer.Acore = Layer.Aicloccore + Layer.Aicglocore + Layer.Acore

    # calculate the performance per Layer
    Layer.Asynlayer += Layer.Asyncore
    Layer.Aneulayer += Layer.Aneucore
    Layer.Aicglolayer += Layer.Aicglocore
    Layer.Aicloclayer += Layer.Aicloccore

    Layer.Esynlayer += Layer.Esyncore
    Layer.Eneulayer += Layer.Eneucore
    Layer.Eicglolayer += Layer.Eicglocore
    Layer.Eicloclayer += Layer.Eicloccore

    Layer.Tsynlayer = max(Layer.Tsyncore, Layer.Tsynlayer)
    Layer.Tneulayer = max(Layer.Tneucore, Layer.Tneulayer)
    Layer.Ticglolayer = max(Layer.Ticglocore, Layer.Ticglolayer)
    Layer.Ticloclayer = max(Layer.Ticloccore, Layer.Ticloclayer)

    Layer.Elayer += Layer.Ecore
    Layer.Alayer += Layer.Acore
    Layer.Tlayer = max(Layer.Tlayer, Layer.Tcore)

    # Layer.Ecore = (Layer.neueffpercore*(Neu.Edev+Layer.Eicglo)+Layer.syneffpercore*(Syn.Edev+Layer.Eicloc)*Layer.shareact) * Layer.spikeAct
    return Layer


def Chip_performance(Arch, Consts):
    # Arch = arch_level()
    # Consts = Param()
    layer_number = len(Consts.synperneu)
    for i in range(layer_number):
        Layer = layer_level()
        for j in range(Consts.nlayers[i]):
            # give some parameters from the data
            index = sum(Consts.nlayers[0:i]) + j
            Layer.synperneu = Consts.synperneu[i]
            Layer.neupercore = Consts.neuperlayer[i]
            Layer.ncores = Consts.nlayers[i]
            Layer.inneuperlayer = Consts.inneuperlayer[i]
            Layer.spikeAct = Consts.spikeAct[index]
            Layer.shareact = Consts.shareact[index]
            # calculate the performance of the specific core in layer i
            Layer = Layer_cal(Arch, Consts, Layer)
        # record some layer information
        # The general information
        Arch.ACP.append(Layer.Alayer)
        Arch.ECP.append(Layer.Elayer)
        Arch.TCP.append(Layer.Tlayer)
        # layer information of synapses, neurons, local interconnects, global interconnects
        Arch.Asyn.append(Layer.Asynlayer)
        Arch.Esyn.append(Layer.Esynlayer)
        Arch.Tsyn.append(Layer.Tsynlayer)

        Arch.Aneu.append(Layer.Aneulayer)
        Arch.Eneu.append(Layer.Eneulayer)
        Arch.Tneu.append(Layer.Tneulayer)

        Arch.Aicloc.append(Layer.Aicloclayer)
        Arch.Eicloc.append(Layer.Eicloclayer)
        Arch.Ticloc.append(Layer.Ticloclayer)

        Arch.Aicglo.append(Layer.Aicglolayer)
        Arch.Eicglo.append(Layer.Eicglolayer)
        Arch.Ticglo.append(Layer.Ticglolayer)
    return Arch
