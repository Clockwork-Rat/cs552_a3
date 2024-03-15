#!/bin/bash
#SBATCH --job-name=cs552_2
#SBATCH --output=/home/ijr44/cs552/a3/act2/run2.txt
#SBATCH --error=/home/ijr44/cs552/a3/act2/run2.txt
#SBATCH --time=100:00
#SBATCH --mem=100G
#SBATCH --nodes=1
#SBATCH --ntasks=2
#SBATCH --cpus-per-task=1
#SBATCH --exclusive

module load openmpi

mpicc -O3 ~/cs552/cs552_a3/distribution_sort_act2_raspet.c -lm -o ~/out_act2_2

srun ~/out_act2_2