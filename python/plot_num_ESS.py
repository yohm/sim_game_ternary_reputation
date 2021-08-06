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

# %%
import subprocess
from subprocess import PIPE
import os,sys
sys.path.append( os.environ['OACIS_ROOT'] )
import oacis

#%%
sim = oacis.Simulator.find_by_name('ternary_reputation_search')
base_ps = sim.find_parameter_set( {'benefit':1.1, 'cost':1.0, 'mu_e':0.001, 'mu_a':0.001, 'coop_prob_th':0.99} )
ps_list = base_ps.parameter_sets_with_different('benefit')
list(ps_list)

#%%
nls = []
bs = []
for ps in ps_list:
    run_dir = ps.runs().first().dir()
    print(f"{run_dir}/ESS_ids")
    nl = int( subprocess.run(['wc', '-l', f"{run_dir}/ESS_ids"], stdout=PIPE).stdout.split()[0] )
    nls.append(nl)
    bs.append(ps.v()['benefit'])

bs,nls
# %%
plt.clf()
bs = [float(x) for x in bs]
plt.ylim((0,9))
plt.xlim((1,5.1))
plt.xticks([1,2,3,4,5])
plt.xlabel(r'benefit-to-cost ratio')
plt.ylabel(r'# of ESSs ($\times 10^6$)')
plt.plot(bs, np.array(nls)/1e6, 'o-')
#plt.show()
plt.savefig("num_ESS.pdf")
# %%
