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
a = np.loadtxt("../job_priv_rep/DP_C1.P1.A1._priv")
a

# %%
plt.clf()
n_bins = 100
plt.hist(a[:,1], bins=n_bins, range=(0.0,1.0), log=False, alpha=0.9, density=False, label="Coop_level")
plt.xlim((0.0,1.0))
plt.xticks([0.0,0.5,1.0])
plt.xlabel("cooperation level")
plt.ylabel("frequency")
# plt.legend()
plt.show()
# %%
# s = a[np.argsort(a[:,1])][::-1]
# s = np.sort(a[:,1])