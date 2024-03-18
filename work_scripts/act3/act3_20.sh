#!/bin/bash
#SBATCH --job-name=cs552_2
#SBATCH --output=/home/ijr44/cs552/a3/act3/run7.txt
#SBATCH --error=/home/ijr44/cs552/a3/act3/run7.txt
#SBATCH --time=100:00
#SBATCH --mem=100G
#SBATCH --nodes=1
#SBATCH --ntasks=20
#SBATCH --cpus-per-task=1
#SBATCH --exclusive

module load openmpi

mpicc -O3 ~/cs552/cs552_a3/sort_act3_raspet.c -lm -o ~/out_act3_7

srun ~/out_act3_7