# %%
import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl
mpl.rcParams.update(mpl.rcParamsDefault)

# %%
plt.rcParams['font.family'] ='sans-serif'
plt.rcParams["figure.subplot.left"] = 0.2
plt.rcParams["figure.subplot.right"] = 0.95
plt.rcParams["figure.subplot.bottom"] = 0.20
plt.rcParams["figure.subplot.top"] = 0.95
plt.rcParams['xtick.direction'] = 'in'
plt.rcParams['ytick.direction'] = 'in'
plt.rcParams['xtick.major.width'] = 1.0
plt.rcParams['ytick.major.width'] = 1.0
plt.rcParams['axes.labelsize'] = 30
plt.rcParams['font.size'] = 22
plt.rcParams['axes.linewidth'] = 1.2
plt.rcParams['savefig.dpi'] = 200
plt.rcParams['figure.facecolor'] = 'white'

#%%
c1_scaling = [
    [ 0.01,0.0145894,0.00524401,0.980167,0.980167],
    [ 0.001,0.00149615,0.000503108,0.998001,0.998001],
    [ 0.0001,0.000150347,5.06218e-05,0.999799,0.999799],
    [ 1e-05,1.53938e-05,5.6007e-06,0.999979,0.999979]
]
c1 = np.array(c1_scaling)
c1

#%%
c2_scaling = [
    [0.01,0.0147052,0.0614825,0.923812,0.985295],
    [0.001,0.00149701,0.0212435,0.977259,0.998503],
    [0.0001,0.00014997,0.00701995,0.99283,0.99985],
    [1e-05,1.49997e-05,0.00243709,0.997548,0.999985]
]
c2 = np.array(c2_scaling)
c2

#%%
c3_scaling = [
    [0.01,0.0147053,0.485682,0.499613,0.985295],
    [0.001,0.00149715,0.498506,0.499997,0.998503],
    [0.0001,0.000150113,0.499849,0.500001,0.99985],
    [1e-05,1.51429e-05,0.499984,0.500001,0.999985]
]
c3 = np.array(c3_scaling)
c3

#%%
cmap = plt.get_cmap("tab10")


# %%
plt.clf()

#plt.ylim((0,10))
#plt.xlim((1e-5,1e-1))
#plt.xticks([1,3,5,7,9])
plt.xscale('log')
plt.yscale('log')
plt.xlabel(r'$\mu$')
plt.ylabel(r'$h_N^{\ast}$')
#plt.plot(c1[:,0], c1[:,1], 'o-')
plt.plot(c1[:,0], c1[:,2], 'o', label='C1', color=cmap(0))
plt.plot(c2[:,0], c2[:,2], 'o', label='C2', color=cmap(1))
plt.plot(c3[:,0], c3[:,2], 'o', label='C3', color=cmap(2))
plt.plot(np.array([1e-5,1e-2]), 0.56 * np.array([1e-5,1e-2]), '--' )
plt.plot(np.array([1e-5,1e-2]), 0.7 * np.array([10**(-2.5),1e-1]), '--' )
plt.plot(np.array([1e-5,1e-2]), 0.5 * np.array([1,1]), '--' )
plt.text(1e-3, 1.5e-4, "C1")
plt.text(1e-3, 6e-3, "C2")
plt.text(1e-3, 1.5e-1, "C3")
# plt.plot(c1[:,0], 1 - c1[:,4], 'x-.', color=cmap(0))
# plt.plot(c2[:,0], 1 - c2[:,4], 'x-.', color=cmap(1))
# plt.plot(c3[:,0], 1 - c3[:,4], 'x-.', color=cmap(2))
# plt.legend()
#plt.show()
plt.savefig("hN_scsaling.pdf")

# %%
