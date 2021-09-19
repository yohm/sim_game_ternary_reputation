#%%
import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl

# %%
mpl.rcParams.update(mpl.rcParamsDefault)
plt.rcParams['font.family'] ='sans-serif'
plt.rcParams["figure.subplot.left"] = 0.22
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
# %%
a = np.loadtxt("core_ESS_ids")

# %%
plt.clf()
n_bins = 100
plt.hist(a[:,2], bins=n_bins, range=(0.0,1.0), log=True, alpha=0.9, density=False, label=r"$h_B^{\ast}$")
plt.hist(a[:,4], bins=n_bins, range=(0.0,1.0), log=True, alpha=0.9, density=False, label=r"$h_G^{\ast}$")
plt.xlim((0.0,1.0))
plt.xticks([0.0,0.5,1.0])
plt.xlabel(r"$h_G^{\ast}, h_B^{\ast}$")
plt.ylabel("frequency")
plt.legend()
# plt.show()
plt.savefig("h_distribution.pdf")
# %%
