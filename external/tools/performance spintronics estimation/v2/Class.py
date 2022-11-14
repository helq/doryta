import numpy as np


class Dev(object):
    def __init__(self):
        # device parameters
        self.Adev = 0   # area of a device
        self.Tdev = 0   # delay of a device
        self.Edev = 0   # energy of a device
        self.Idev = 0   # Current flowing in the device
        self.Vdev = 0   # Voltage applied in the device
        self.Cdev = 0   # intrinsic capacitance of the device
        self.Rondev = 0   # resistance fo the device


class Param(object):
    def __init__(self, pitch, resis_cal, cap_cal, liner=3e-9, ar=2, eps=2.55):
        # liner is the thickness of the liner
        # ar is the aspect ratio
        self.Vdd = 0
        self.pitch = pitch
        self.cperl = 5e-10/15e-9*pitch
        self.rho0 = 0.25e9
        self.rperl = 0
        self.liner = liner
        self.resis_cal = resis_cal
        self.cap_cal = cap_cal
        self.eps = eps  # permittivity of dielectric material
        self.wire_width = pitch/2-2*self.liner
        self.wire_height = pitch/2*ar-self.liner
        # resistivity of interconnect
        self.resis = self.resis_cal(d=self.wire_width, D=self.wire_height)
        self.rperl = self.resis/(self.wire_width*self.wire_height)
        # capacitance per unit length
        self.cperl = self.cap_cal(
            self.eps, self.pitch, 5e-9, self.pitch*ar, self.pitch, 'Four')
        self.factasyn = 2   # area factor of synapse
        self.factaneu = 2   # area factor of neuron
        self.factabun = 2   # area factor of bundle
        self.factachip = 2  # area factor of chip
        self.nlayers = np.array([1])
        self.neuperlayer = np.array([1])
        self.synperneu = np.array([1])
        self.inneuperlayer = np.array([1])
        self.integration = np.array([])
        self.fire = np.array([])
        self.shareact = np.array([])
        self.spikeAct = np.array([])


class arch_level(object):
    def __init__(self, neu, syn):
        # the Dev class for a neuron
        self.neu = neu
        # the Dev class for a synapse
        self.syn = syn
        # all the information are recorded in a list
        # calculate all the area/time/energy spent by neurons
        self.Tneu = []
        self.Aneu = []
        self.Eneu = []
        # calculatte all the area/time/energy spent by synapse
        self.Tsyn = []
        self.Asyn = []
        self.Esyn = []
        # local interconnect information
        self.Ticloc = []
        self.Aicloc = []
        self.Eicloc = []
        # gobal interconnect information
        self.Ticglo = []
        self.Aicglo = []
        self.Eicglo = []
        # parameters of the chip
        self.ACP = []
        self.TCP = []
        self.ECP = []


class layer_level(object):
    def __init__(self):
        # nueron numbers per core
        self.neupercore = 0
        self.neueffpercore = 0
        self.synpercore = 0
        self.syneffpercore = 0
        # core numbers in one layer
        self.ncores = 0
        self.shareact = 0
        self.neuperlayer = 0
        self.synperlayer = 0
        self.inneuperlayer = 0
        self.synperneu = 0
        # layer information
        self.Alayer = 0
        self.Tlayer = 0
        self.Elayer = 0
        # performance of all components
        # this is the performance of all synapses in one core/layer
        self.Asyncore = 0
        self.Tsyncore = 0
        self.Esyncore = 0
        self.Asynlayer = 0
        self.Tsynlayer = 0
        self.Esynlayer = 0
        # this is the performance of all neurons in one core/layer
        self.Aneucore = 0
        self.Tneucore = 0
        self.Eneucore = 0
        self.Aneulayer = 0
        self.Tneulayer = 0
        self.Eneulayer = 0
        # this is the performance of all local interconnects in one core/layer
        self.Aicloccore = 0
        self.Ticloccore = 0
        self.Eicloccore = 0
        self.Aicloclayer = 0
        self.Ticloclayer = 0
        self.Eicloclayer = 0
        # this is the performance of all global interconnects in one core/layer
        self.Aicglocore = 0
        self.Ticglocore = 0
        self.Eicglocore = 0
        self.Aicglolayer = 0
        self.Ticglolayer = 0
        self.Eicglolayer = 0
        # core information
        self.Acore = 0
        self.Tcore = 0
        self.Ecore = 0
        self.synpercore = 0
        # local interconnect information
        self.Aicloc = 0
        self.Eicloc = 0
        self.Ticloc = 0
        # global interconnect information
        self.Aicglo = 0
        self.Ticglo = 0
        self.Eicglo = 0
        self.spikeAct = np.array([])
