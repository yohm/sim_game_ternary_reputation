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
dat = np.loadtxt("num_ESS.dat")

# %%
plt.clf()

plt.ylim((0,10))
plt.xlim((1,10.1))
plt.xticks([1,3,5,7,9])
plt.xlabel(r'benefit-to-cost ratio')
plt.ylabel(r'# of CESS ($\times 10^6$)')
plt.plot(dat[:,0], dat[:,1]/1000000, 'o-')
x = np.array([0,10])
y = np.array([2.166764, 2.166764])
plt.plot(x, y, '--')
#plt.show()
plt.savefig("num_ESS.pdf")

# %%
