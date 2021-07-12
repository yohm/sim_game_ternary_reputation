#!/bin/bash
#PJM --rsc-list "node=1000"
#PJM --rsc-list "rscunit=rscunit_ft01"
#PJM --rsc-list "rscgrp=large"
#PJM --rsc-list "elapse=05:00:00"
#PJM --mpi "max-proc-per-node=4"
#PJM -s

export OMP_NUM_THREADS=12

mpiexec -stdout-proc ./%/1000R/stdout -stderr-proc ./%/1000R/stderr ../main_search_ESS.out ../RD_list _input.json 1000
