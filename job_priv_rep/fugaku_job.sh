#!/bin/bash
#PJM --rsc-list "node=1000"
#PJM --rsc-list "rscunit=rscunit_ft01"
#PJM --rsc-list "rscgrp=eap-large"
#PJM --rsc-list "elapse=05:00:00"
#PJM --mpi "max-proc-per-node=48"
#PJM -s

mpiexec -stdout-proc ./%/1000R/stdout -stderr-proc ./%/1000R/stderr ../main_private_rep_coop_prob.out _input.json DP_C1.P1.A1. DP_C1.P1.A2. DP_C1.P2.A1. DP_C1.P2.A2. DP_C2.P1.A1. DP_C2.P1.A2. DP_C2.P1.A3. DP_C2.P2.A1. DP_C2.P2.A2. DP_C2.P3.A1. DP_C4.P1.A2. DP_C4.P2.A1. DP_C4.P2.A2. DP_C5.P1.A1.

