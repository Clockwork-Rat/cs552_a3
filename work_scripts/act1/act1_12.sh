#!/bin/bash
#SBATCH --job-name=cs552_2
#SBATCH --output=/home/ijr44/cs552/a3/act1/run5.txt
#SBATCH --error=/home/ijr44/cs552/a3/act1/run5.txt
#SBATCH --time=100:00
#SBATCH --mem=100G
#SBATCH --nodes=1
#SBATCH --ntasks=12
#SBATCH --cpus-per-task=1
#SBATCH --exclusive

module load openmpi

mpicc -O3 ~/cs552/cs552_a3/distribution_sort_act1_raspet.c -lm -o ~/out_act1_5

srun ~/out_act1_5