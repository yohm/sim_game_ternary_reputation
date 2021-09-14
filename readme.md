# indirect reciprocity with ternary reputation model

## How to build

- Clone the repository with the submodules.
    - `git clone --recursive git@github.com:yohm/sim_game_ternary_reputation.git`
- build binary using cmake.
    - `mkdir build; cd build; cmake ..; make`
    - Some of them requires [Eigen](http://eigen.tuxfamily.org/index.php), OpenMP and/or MPI as prerequisites.
    - On macOS, `brew install openmpi` and `brew install libomp` are required.
- On supercomputer Fugaku, `./fugaku_build.sh` to build `main_search_ESS.out`.

## Executables

### test_Game.out

Unit tests for `Game` class. (`Game` contains `ActionRule` and `ReputationDynamics`, i.e., a pair of behavioral rule and a reputation dynamics.)
If you run it without an argument, the unit tests were executed.
If you run it with GameID as an argument, it will print the details of the Game. Here is an example.

```shell
> ./test_Game.out 82377856438
- GameID: 82377856438
  - RD_id, AR_id: 160894250, 438
  - mu_a, mu_e: 0.001 0.001
- Prescriptions:

  | | | |
  |-|-|-|
  | (B->B) dG:G   | (B->N) cG:B   | (B->G) cN:B  |
  | (N->B) dN:G   | (N->N) cG:B   | (N->G) cG:B  |
  | (G->B) dG:N   | (G->N) cG:B   | (G->G) cN:B  |

- Cooperation Probability: 0.9985
- Population:
  - h_B: 0.0015
  - h_N: 0.3816
  - h_G: 0.6169
- Continuation payoff (w=0.5, b/c=2.0):
  - v_B: 0.0005
  - v_N: 0.9995
  - v_G: 0.9995
- ESS b range: [1.00251, 1.79769e+308]
- Major Population Flow: (threshold=0.0002, implementation error: *, assignment error: +)
  - (GG:cN)  : 0.3798
  - (GN:cG)  : 0.2349
  - (NG:cG)  : 0.2349
  - (NN:cG)  : 0.1453
  - (GB:dG)  : 0.0009
  - (BG:cN)  : 0.0009
  - (NB:dN)  : 0.0006
  - (BN:cG)  : 0.0006
  - (GG:dB)* : 0.0004
  - (GN:dB)* : 0.0002
  - (NG:dB)* : 0.0002
```

### print_normalized_RD.out

Print all the IDs of *independent* reputation dynamics with ternary reputation taking into account the permutation symmetry between reputations.
This is a pre-process of `main_search_ESS.out`.

```shell
./print_normalized_RD.out > RD_list
```

The content of `RD_list` looks like the following.

```
129631876
129632234
129711337
129711695
129713153
130045228
130045496
130045577
....
```


### main_search_ESS.out

Executes the search of all the ESS pairs. Specify the file of reputation dynamics IDs (`RD_list`) as an argument.
The second argument specifies the JSON file, which contains parameters such as the search range of benefit-to-cost ratio and error rates.
The third argument specifies the chunk size. The inputs are divided into chunks of this size for distributing tasks.
The ESS pairs are printed to the file `ESS_ids`.

The format of `_input.json` is the following:
```json
{
  "mu_e": 0.001,
  "mu_a": 0.001,
  "benefit_lower_max": 10.0,
  "benefit_upper_min": 1.0,
  "coop_prob_th": 0.99
}
```
where `mu_e` and `mu_a` represent the probability of implementation and assignment error, respectively.
The norms are printed when they form ESS in `[benefit_upper_min, benefit_lower_max]`.
`coop_prob_th` is the threshold for the cooperation level. If the cooperation level of the norm is below this threshold, it is excluded from the output.
A sample of the input JSON file and the job script are in `job/` directory.

The program is parallelized using OpenMP and MPI.
Thus, the execution command should look like the following.

```shell
export OMP_NUM_THREADS=12
mpiexec ./main.out RD_list _input.json 1000
```

The format of the `ESS_ids` is the following.
Each column denotes the GameID, the cooperation level, and fractions of B, N, G players `(h_B,h_N,h_G)`.
The last columns denote the range of benefit-to-cost ratio between which the norm is ESS.
To inspect the details of each result, execute `test_Game.out` with a GameID as its argument.

```
74212345264 0.994066 0.00297165 0.498141 0.498887 2.00874 205741
73669035440 0.994057 0.00297601 0.498144 0.49888 2.00875 206386
73671292338 0.995535 0.00297897 0.498136 0.498885 1.5075 345.183
73670528440 0.995535 0.00297894 0.498142 0.498879 4.01603 418153
73671274928 0.994057 0.00297601 0.498144 0.49888 2.00875 206386
73671296944 0.994057 0.00297603 0.498138 0.498886 2.00874 496.331
74757468080 0.997009 0.0014965 0.498879 0.499625 2.00502 1.79769e+308
74215326640 0.997009 0.0014965 0.499251 0.499252 2.00502 1.79769e+308
74756721584 0.994065 0.00297168 0.498146 0.498883 2.00948 324.843
74758961080 0.997755 0.00149724 0.498878 0.499625 4.01003 1.79769e+308
....
```

Note that the labels of reputations are re-assigned according to the following criteria:

- The reputation having the largest fraction of players in equilibrium is labeled as `G`.
- When a `G` player defects against another `G` player, the donor gets a reputation other than `G` by nature of the cooperative ESSs. The reputation assigned in this case is `B`.
- The remaining one is labeled as `N`.

### find_core_ESS.out

Find the "core set" of CESS from the list of `ESS_ids`.
The core set is the set of the CESS that are ESS whenever benefit-to-cost ratio is in between `[1.1, 10]`.
The output is printed to a file named `core_ESS_ids`.

```shell
./find_core_ESS.out ESS_ids
```

### main_classify_ESS.out

Classify the ESS pairs according to the criteria mentioned in the paper. It should work with the ``core set'' but may not work with others containing unknown types.
Give `core_ESS_ids` as its argument.
The histogram of the output is printed to stdout, while the strategies in each class are printed in files `DP_...` in the same format as `ESS_ids`.
It is parallelized using OpenMP.

```shell
./main_classify_ESS.out core_ESS_ids > out_histo
```

You'll get an output like the following.

```
type: C1.P1.R11. h_N=O(mu),h_G=O(mu) (C1: G dominant), GB:dG (P1: G punisher is justified), BG:cG NG:*G (R11: B->G,N->G), 610471
type: C1.P1.R12. h_N=O(mu),h_G=O(mu) (C1: G dominant), GB:dG (P1: G punisher is justified), BG:cG NG:*B (R12: N->B->G), 456420
type: C1.P1.R21. h_N=O(mu),h_G=O(mu) (C1: G dominant), GB:dG (P1: G punisher is justified), BG:cN NG:cG (R21: B->N->G,cc), 137781
type: C1.P1.R22. h_N=O(mu),h_G=O(mu) (C1: G dominant), GB:dG (P1: G punisher is justified), BG:cN NG:dG (R22: B->N->G,cd), 110592
type: C1.P1.R23. h_N=O(mu),h_G=O(mu) (C1: G dominant), GB:dG (P1: G punisher is justified), BG:dN NG:cG (R23: B->N->G,dc), 147456
type: C1.P21.R11. h_N=O(mu),h_G=O(mu) (C1: G dominant), GB:dN GN:dG (P21: G punisher becomes N, N is punished by G), BG:cG NG:*G (R11: B->G,N->G), 275562
type: C1.P21.R21. h_N=O(mu),h_G=O(mu) (C1: G dominant), GB:dN GN:dG (P21: G punisher becomes N, N is punished by G), BG:cN NG:cG (R21: B->N->G,cc), 78576
type: C1.P21.R22. h_N=O(mu),h_G=O(mu) (C1: G dominant), GB:dN GN:dG (P21: G punisher becomes N, N is punished by G), BG:cN NG:dG (R22: B->N->G,cd), 73494
type: C1.P21.R23. h_N=O(mu),h_G=O(mu) (C1: G dominant), GB:dN GN:dG (P21: G punisher becomes N, N is punished by G), BG:dN NG:cG (R23: B->N->G,dc), 98088
type: C1.P22.R11. h_N=O(mu),h_G=O(mu) (C1: G dominant), GB:dN GN:cG (P22: G punisher becomes N, N is cooperated by G), BG:cG NG:*G (R11: B->G,N->G), 96453
type: C1.P22.R21. h_N=O(mu),h_G=O(mu) (C1: G dominant), GB:dN GN:cG (P22: G punisher becomes N, N is cooperated by G), BG:cN NG:cG (R21: B->N->G,cc), 19632
type: C21.P1.R1. h_N=O(mu^1/2),h_B=O(mu) (C2: N~sqrt(mu), GG:cG,GN:cG,NG:cN,NN:*G), GB:dG (P1: G punisher is justified), BG:cG (R1: B->G), 2340
type: C21.P1.R2. h_N=O(mu^1/2),h_B=O(mu) (C2: N~sqrt(mu), GG:cG,GN:cG,NG:cN,NN:*G), GB:dG (P1: G punisher is justified), BG:cN (R2: B->N), 6795
type: C21.P2.R1. h_N=O(mu^1/2),h_B=O(mu) (C2: N~sqrt(mu), GG:cG,GN:cG,NG:cN,NN:*G), GB:dN (P21: G punisher becomes N), BG:cG (R1: B->G), 2127
type: C21.P2.R2. h_N=O(mu^1/2),h_B=O(mu) (C2: N~sqrt(mu), GG:cG,GN:cG,NG:cN,NN:*G), GB:dN (P21: G punisher becomes N), BG:cN (R2: B->N), 4446
type: C22.P1.R1. h_N=O(mu^1/2),h_B=O(mu) (C2: N~sqrt(mu), GG:cG,GN:cN,NG:cG,NN:*G), GB:dG (P1: G punisher is justified), BG:cG (R1: B->G), 8748
type: C22.P1.R2. h_N=O(mu^1/2),h_B=O(mu) (C2: N~sqrt(mu), GG:cG,GN:cN,NG:cG,NN:*G), GB:dG (P1: G punisher is justified), BG:cN (R2: B->N), 8748
type: C22.P2.R1. h_N=O(mu^1/2),h_B=O(mu) (C2: N~sqrt(mu), GG:cG,GN:cN,NG:cG,NN:*G), GB:dN (P21: G punisher becomes N), BG:cG (R1: B->G), 8532
type: C22.P2.R2. h_N=O(mu^1/2),h_B=O(mu) (C2: N~sqrt(mu), GG:cG,GN:cN,NG:cG,NN:*G), GB:dN (P21: G punisher becomes N), BG:cN (R2: B->N), 7440
type: C23.P1.R1. h_N=O(mu^1/2),h_B=O(mu) (C2: N~sqrt(mu), GG:cG,GN:cN,NG:cG,NN:*B), GB:dG (P1: G punisher is justified), BG:cG (R1: B->G), 2187
type: C3.P1.R1. h_N=O(1),h_G=O(mu) (C3: GN dominant), GB:d[GN] NB:d[GN] (P1: both GN punishers are justified), B[GN]:c[GN]:B (R1: B cooperates G&N), 4686
type: C3.P1.R21. h_N=O(1),h_G=O(mu) (C3: GN dominant), GB:d[GN] NB:d[GN] (P1: both GN punishers are justified), BN:dB:B or BG:c[GN]:B (R21: B cooperates with G but not with N), 2340
type: C3.P1.R22. h_N=O(1),h_G=O(mu) (C3: GN dominant), GB:d[GN] NB:d[GN] (P1: both GN punishers are justified), BN:c[GN]:B or BG:dB:B (R22: B cooperates with N but not with G), 2073
type: C3.P21.R1. h_N=O(1),h_G=O(mu) (C3: GN dominant), GB:d[NG] NB:dB (P21: N pusniher is not justified), B[GN]:c[GN]:B (R1: B cooperates G&N), 720
type: C3.P21.R21. h_N=O(1),h_G=O(mu) (C3: GN dominant), GB:d[NG] NB:dB (P21: N pusniher is not justified), BN:dB:B or BG:c[GN]:B (R21: B cooperates with G but not with N), 132
type: C3.P22.R1. h_N=O(1),h_G=O(mu) (C3: GN dominant), GB:dB, NB:d[GN] (P22: G punisher is not justified), B[GN]:c[GN]:B (R1: B cooperates G&N), 813
=================== TYPE C1.P1.R11. h_N=O(mu),h_G=O(mu) (C1: G dominant), GB:dG (P1: G punisher is justified), BG:cG NG:*G (R11: B->G,N->G)====================
       :        d       c|       B       N       G|       B       N       G
(B->B) :   466110  144361|  109394  185221  315856|  293797  230962   85712
(B->N) :   476262  134209|  111309  183527  315635|  290858  234351   85262
(B->G) :        0  610471|       0       0  610471|  413641  196830       0
(N->B) :   467515  142956|  113559  185733  311179|  294851  231414   84206
(N->N) :   467287  143184|  112207  186679  311585|  295716  230233   84522
(N->G) :   198028  412443|       0       0  610471|  283331  259422   67718
(G->B) :   610471       0|       0       0  610471|  205199  200073  205199
(G->N) :   523260   87211|  162378       0  448093|  290125  200052  120294
(G->G) :        0  610471|       0       0  610471|  610471       0       0
...
```

### sort_ESS.out

Sort lines in `ESS_ids` or `DP_...` files in ascending order by GameID.

```
./sort_ESS.out DP_C1.P1.R1
```

### diff_ESS.out

Print the difference between two ID files.

```
./diff_ESS.out ESS_ids ESS_ids2
```

## Results

You can find the `core_ESS_ids` file for `mu_a = mu_e = 1e-3`, `coop_prob_th = 0.99` in `result/` directory.
