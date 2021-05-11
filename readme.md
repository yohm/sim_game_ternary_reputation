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
> ./test_Game.out 140322844084
- GameID: 140322844084
  - RD_id, AR_id: 274068054, 436
- Prescriptions:

  | | | |
  |-|-|-|
  | (B->B) dB:B   | (B->N) dG:B   | (B->G) cG:B  |
  | (N->B) dG:B   | (N->N) cN:B   | (N->G) cG:B  |
  | (G->B) dG:B   | (G->N) cN:B   | (G->G) cG:B  |

- Cooperation Probability: 0.97
- Population:
  - h_B: 0.03
  - h_N: 0.13
  - h_G: 0.84
- Major Population Flow:
  - (G-c-G): 0.681
  - (G-c-N): 0.111
  - (N-c-G): 0.104
  - (G-d-G): 0.025
  - (B-c-G): 0.024
  - (N-c-N): 0.017
  - (G-d-B): 0.016
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
The second argument specifies the JSON file, which contains parameters such as the benefit-to-cost ratio and error rates.
The third argument specifies the chunk size. The inputs are divided into chunks of this size for distributing tasks.
The ESS pairs are printed to the file `ESS_ids`.

The program is parallelized using OpenMP and MPI.
Thus, the execution command should look like the following.

```shell
export OMP_NUM_THREADS=12
mpiexec ./main.out RD_list _input.json 1000
```

The format of the `ESS_ids` is the following.
Each column denotes the GameID, the cooperation level, and fractions of B, N, G players (h_B,h_N,h_G).
To inspect the details of each result, execute `test_Game.out` with a GameID as its argument.

```
75031462834 0.920569 0.0532529 0.455164 0.491583
74215169458 0.920569 0.0532529 0.455164 0.491583
74759365042 0.922872 0.0521416 0.468653 0.479205
75575658418 0.922872 0.0521416 0.468653 0.479205
81289712050 0.90205 0.0621889 0.362773 0.575038
82106005426 0.90205 0.0621889 0.362773 0.575038
75847756210 0.920569 0.0532529 0.455164 0.491583
82922298802 0.90205 0.0621889 0.362773 0.575038
73943071666 0.922872 0.0521416 0.468653 0.479205
82015306162 0.922872 0.0521416 0.468653 0.479205
....
```

Note that the labels of reputations are re-assigned according to the following criteria:

- The reputation having the largest fraction of players in equilibrium is labeled as `G`.
- When a `G` player defects against another `G` player, the donor gets a reputation other than `G` by nature of the cooperative ESSs. The reputation assigned in this case is `B`.
- The remaining one is labeled as `N`.

The format of `_input.json` is given as follows.

```json
{
  "mu_e": 0.02,
  "mu_a": 0.02,
  "benefit": 1.2,
  "cost": 1.0,
  "coop_prob_th": 0.9
}
```

A sample of the JSON file and the job script are in `job/` directory.

### calc_histo.out

Classify the ESS pairs according to the criteria mentioned in the paper. It should work with the ``core set'' but probably not with the others since it raises an exception when an unknown pattern is found.
Give `ESS_ids` as its argument. The histogram of the output is printed to stdout, while the strategies in each class are printed in files `DP_...` in the same format as `ESS_ids`.
It is parallelized using OpenMP.

```shell
./calc_histo.out ESS_ids > out_histo
```

You'll get an output like the following.

```
type: C1.P1.R1. GG:cG:B h_N<0.1 (C1: G dominant), GB:dG (P1: G punisher is justified), BG:cG (R1: B->G), 1242910
type: C1.P1.R2. GG:cG:B h_N<0.1 (C1: G dominant), GB:dG (P1: G punisher is justified), BG:*N (R2: B->N), 319317
type: C1.P2.R1. GG:cG:B h_N<0.1 (C1: G dominant), GB:dN (P2: G punisher becomes N), BG:cG (R1: B->G), 259656
type: C1.P2.R2. GG:cG:B h_N<0.1 (C1: G dominant), GB:dN (P2: G punisher becomes N), BG:*N (R2: B->N), 12348
type: C2.P1.R1. [GN][GN]:c[GN]:B (C2: GN cooperation, defector gets B), GB:d[GN] NB:d[GN] (P1: both GN punishers are justified), B[GN]:c[GN]:B (R1: B cooperates G&N), 7152
type: C2.P1.R2. [GN][GN]:c[GN]:B (C2: GN cooperation, defector gets B), GB:d[GN] NB:d[GN] (P1: both GN punishers are justified), BN:dB:B or BG:c[GN]:B (R2: B cooperates with G but not with N), 2394
type: C2.P1.R3. [GN][GN]:c[GN]:B (C2: GN cooperation, defector gets B), GB:d[GN] NB:d[GN] (P1: both GN punishers are justified), BN:c[GN]:B or BG:dB:B (R3: B cooperates with N but not with G), 1359
type: C2.P2.R1. [GN][GN]:c[GN]:B (C2: GN cooperation, defector gets B), GB:d[NG] NB:dB (P2: N pusniher is not justified), B[GN]:c[GN]:B (R1: B cooperates G&N), 657
type: C2.P2.R2. [GN][GN]:c[GN]:B (C2: GN cooperation, defector gets B), GB:d[NG] NB:dB (P2: N pusniher is not justified), BN:dB:B or BG:c[GN]:B (R2: B cooperates with G but not with N), 96
type: C2.P3.R1. [GN][GN]:c[GN]:B (C2: GN cooperation, defector gets B), GB:dB, NB:d[GN] (P3: G punisher is not justified), B[GN]:c[GN]:B (R1: B cooperates G&N), 357
type: C3.P1.R1. GG:cG:B [GN,NG]:c[GN]:B NN:dG (C3: NN defects and gets GN), GB:dG (P1: G punisher is justified), BG:cG (R1: B->G), 963
type: C3.P1.R2. GG:cG:B [GN,NG]:c[GN]:B NN:dG (C3: NN defects and gets GN), GB:dG (P1: G punisher is justified), BG:*N (R2: B->N), 3357
type: C3.P2.R1. GG:cG:B [GN,NG]:c[GN]:B NN:dG (C3: NN defects and gets GN), GB:dN (P2: G punisher becomes N), BG:cG (R1: B->G), 2097
type: C3.P2.R2. GG:cG:B [GN,NG]:c[GN]:B NN:dG (C3: NN defects and gets GN), GB:dN (P2: G punisher becomes N), BG:*N (R2: B->N), 3168
=================== TYPE C1.P1.R1. GG:cG:B h_N<0.1 (C1: G dominant), GB:dG (P1: G punisher is justified), BG:cG (R1: B->G)====================
       :        d       c|       B       N       G|       B       N       G
(B->B) :   996232  246678|  275250  304843  662817|  555772  503919  183219
(B->N) :  1014285  228625|  280631  321837  640442|  556197  502445  184268
(B->G) :        0 1242910|       0       0 1242910|  823058  419852       0
(N->B) :   988990  253920|  292168  326665  624077|  543239  525046  174625
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

In `result/` directory, there is the list of cooperative ESSs for b/c=1.1, mu_e=0.02, mu_a=0.02.
