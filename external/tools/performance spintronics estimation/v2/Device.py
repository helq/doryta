# from inspect import isclass
# from turtle import rt
import numpy as np
from collections import defaultdict


def resistivity_cal(d, D, rho0=1.67e-8, Lambda=39.5e-9, p=0.5, R=0.3):
    # d is the wire width
    # D is the wire height
    return rho0+rho0*Lambda*3*(1-p)/4/d+rho0*Lambda*3*R/2/D/(1-R)


def Single_Cap(eps, W, H, T):
    # eps is the permittivity of dielectric material
    # W is the width of the interconnects
    # H is the distance from the wire to the ground
    # T is the thickness of the wire
    # if (0.3 <W/H < 30) != True:
    #     warnings.warn('W/H ratio not in 0.3-30')
    # if (0.3 <T/H < 30) != True:
    #     warnings.warn('T/H ratio not in 0.3-30')
    return eps*8.855e-12*(1.5*(W/H)+2.8*(T/H)**0.222)


def Couple_Cap(eps, W, H, T, S):
    # S is the distance between two lines
    # the cross section of the two lines are T*L
    # if (0.3 <W/H < 10) != True:
    #     warnings.warn('W/H ratio not in 0.3-10')
    # if (0.3 <T/H < 10) != True:
    #     warnings.warn('T/H ratio not in 0.3-10')
    # if (0.5 <S/H < 10) != True:
    #     warnings.warn('S/H ratio not in 0.3-10')
    return eps*8.855e-12*(0.03*(W/H)+0.83*(T/H)-0.07*(T/H)**0.222)*(S/H)**(-1.34)


def Cap(eps, W, H, T, S=0, Config='Single'):
    if Config != 'Single' and S == 0:
        raise ValueError('S should be nonzero for NoneSingle Configuration')
    if Config == 'Single':
        return Single_Cap(eps, W, H, T)
    elif Config == 'Two':
        return Single_Cap(eps, W, H, T)+2*Couple_Cap(eps, W, H, T, S)
    elif Config == 'Four':
        return Single_Cap(eps, W, H, T)+2*Couple_Cap(eps, W, H, T, S)+2*Couple_Cap(eps, T, W, W, H)


def Delay_cal(J, t_pulse, name):
    if name == 'Mn3Sn':
        JJ = defaultdict(lambda: defaultdict(lambda: None))
        JJ[1e10][10e-12] = 32e-12
        JJ[1e10][20e-12] = 55e-12
        JJ[2e10][5e-12] = 12e-12
        JJ[2e10][10e-12] = 21e-12
        JJ[2e10][20e-12] = 40e-12
        JJ[3e10][5e-12] = 9e-12
        JJ[3e10][10e-12] = 18e-12
        JJ[3e10][20e-12] = 34e-12
        JJ[4e10][5e-12] = 8e-12
        JJ[4e10][10e-12] = 15e-12
        JJ[4e10][20e-12] = 30e-12
        JJ[5e10][5e-12] = 7e-12
        JJ[5e10][10e-12] = 14e-12
        JJ[5e10][20e-12] = 28e-12
        for J_temp in JJ:
            if np.isclose(J, J_temp, rtol=1e-5, atol=1e3) == True:
                for t_temp in JJ[J_temp]:
                    if np.isclose(t_pulse, t_temp, rtol=1e-5, atol=1e-12) == True:
                        return JJ[J_temp][t_temp]
    elif name == 'NiO':
        JJ = defaultdict(lambda: defaultdict(lambda: None))
        JJ[2e11][5e-12] = 15e-12
        JJ[3e11][5e-12] = 12e-12
        JJ[4e11][5e-12] = 11e-12
        JJ[5e11][5e-12] = 10e-12
        for J_temp in JJ:
            if np.isclose(J, J_temp, rtol=1e-5, atol=1e3) == True:
                for t_temp in JJ[J_temp]:
                    if np.isclose(t_pulse, t_temp, rtol=1e-5, atol=1e-12) == True:
                        return JJ[J_temp][t_temp]


def neuron_cal(Consts, Dev, t_pulse, J_density, name, thick=4e-9):
    # rho_mn3sn = 1133e-8 # resistivity of mn3sn
    if name == 'Mn3Sn':
        # rho_Hall = 6.7e-9  # Hall resistivity
        # pitch = Consts.pitch
        W = 40e-9
        L = 120e-9
        Dev.Adev = L*W
        Dev.Rondev = 64
        Idev = J_density*10*W*5e-9
        Dev.Vdev = Idev*64
        Dev.Cdev = Dev.Adev*9.8*8.8541878128e-12/2e-9
        Dev.Tdev = Delay_cal(J_density, t_pulse, 'Mn3Sn')
        Dev.Edev = Idev**2*Dev.Rondev*t_pulse*np.sqrt(np.pi)
        Dev.Idev = Idev  # Output current of CMOS
    elif name == 'NiO':
        W = 40e-9
        L = 120e-9
        Dev.Adev = L*W
        Dev.Rondev = 64
        Idev = J_density*10*W*5e-9
        Dev.Vdev = Idev*64
        Dev.Cdev = L*thick*11.9*8.8541878128e-12/W
        Dev.Tdev = Delay_cal(J_density, t_pulse, 'NiO')
        Dev.Edev = Idev**2*Dev.Rondev*t_pulse*np.sqrt(np.pi)
        Dev.Idev = Idev

    return Dev


def synapse_cal(Consts, Dev, Idev, t_pulse):
    RA_max = 27e-12
    RA_min = 8e-12
    # pitch = Consts.pitch
    L = 30e-9
    W = 300e-9
    Dev.Adev = L*W
    Dev.Rondev = (RA_max+RA_min)/Dev.Adev/2
    Dev.Vdev = Idev*(RA_min/Dev.Adev)
    Consts.Vdd = Dev.Vdev
    Dev.Idev = Dev.Vdev/Dev.Rondev
    Dev.Cdev = Dev.Adev*9.8*8.8541878128e-12/2e-9
    Lic = np.sqrt(Dev.Adev)
    Ric = Lic*Consts.rperl
    Cic = Lic*Consts.cperl
    Dev.Tdev = 0.69*(Ric*Dev.Cdev+Cic*Dev.Rondev)
    Dev.Edev = Idev**2*Dev.Rondev*Dev.Tdev*np.sqrt(np.pi)
    return Dev, Consts


def CMOS_cal(Neu, Syn, type):
    if type == 'digit':
        Neu.Adev = 0.00000000011077560000
        Neu.Tdev = 0.00000000063234184301
        Neu.Edev = 0.00000000000013612376
        Neu.Idev = 9.890410958904109e-05
        Neu.Rondev = 8.088642659279780e+03
        Neu.Cdev = 1.032e-9*np.sqrt(Neu.Adev)
        Neu.Vdev = 0.8

        Syn.Adev = 0.00000000000138240000
        Syn.Tdev = 0.00000000064416087938
        Syn.Edev = 0.00000000000017057941
        Syn.Idev = 9.890410958904109e-05
        Syn.Rondev = 8.088642659279780e+03
        Syn.Cdev = 1.032e-9*np.sqrt(Syn.Adev)
        Syn.Vdev = 0.8

    if type == 'Analog':
        Neu.Adev = 0.00000000000069120000
        Neu.Tdev = 0.00000000198848784409
        Neu.Edev = 0.00000000000013828333
        Neu.Idev = 3.956164383561644e-04
        Neu.Rondev = 2.022160664819945e+03
        Neu.Cdev = 1.032e-9*np.sqrt(Neu.Adev)
        Neu.Vdev = 0.8

        Syn.Adev = 0.00000000000016875000
        Syn.Tdev = 0.00000000001893119296
        Syn.Edev = 0.00000000000000192964
        Syn.Idev = 3.956164383561644e-04
        Syn.Rondev = 2.022160664819945e+03
        Syn.Cdev = 6.1389e-16
        Syn.Vdev = 0.8
    return Neu, Syn
