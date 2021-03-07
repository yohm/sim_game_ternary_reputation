# %%
import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl
mpl.rcParams.update(mpl.rcParamsDefault)

# %%
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
import subprocess

nls = []
bs = ['1.1', '1.2', '1.5', '2.0', '2.5', '3.0', '4.0', '5.0']
for b in bs:
    nl = int(subprocess.run(['wc', '-l', f"../,result/job_b{b}/ESS_ids"], capture_output=True).stdout.split()[0])
    nls.append(nl)

nls
# %%
bs = [float(x) for x in bs]
plt.ylim((0,7))
plt.xlim((1,5.1))
plt.xticks([1,2,3,4,5])
plt.xlabel(r'$b/c$')
plt.ylabel(r'# of ESS ($\times 10^6$)')
plt.plot(bs, np.array(nls)/1e6, 'o-')
plt.savefig("num_ESS.png")
# %%
n_mu_a = int(subprocess.run(['wc', '-l', f"../,result/job_mu_a_2e-3/ESS_ids"], capture_output=True).stdout.split()[0])
n_mu_e = int(subprocess.run(['wc', '-l', f"../,result/job_mu_e_2e-3/ESS_ids"], capture_output=True).stdout.split()[0])
n_mu_e_mu_a = int(subprocess.run(['wc', '-l', f"../,result/job_mu_a_mu_e_2e-3/ESS_ids"], capture_output=True).stdout.split()[0])
n_mu_a, n_mu_e, n_mu_e_mu_a
# %%
