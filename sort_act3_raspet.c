#include <stdio.h>
#include <unistd.h>
#include <mpi.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>



void generateData(int * data, int SIZE);


int compfn (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

size_t sum(int size, int *data) {
  size_t ret = 0;
  
  for (int i = 0; i < size; ++i) {
    ret += data[i];
  }

  return ret;
}

//Do not change the seed
#define SEED 72
#define MAXVAL 1000000

//Total input size is N, divided by nprocs
//Doesn't matter if N doesn't evenly divide nprocs
#define N 1000000000
//#define N 1000

int main(int argc, char **argv) {

  int my_rank, nprocs;

  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);

  //seed rng do not modify
  srand(SEED+my_rank);


  //local input size N/nprocs
  const unsigned int localN=N/nprocs;

  //All ranks generate data
  int * data=(int*)malloc(sizeof(int)*localN);

  generateData(data, localN);

  int * sendDataSetBuffer=(int*)malloc(sizeof(int)*localN); //most that can be sent is localN elements
  int * recvDatasetBuffer=(int*)malloc(sizeof(int)*localN); //most that can be received is localN elements
  int * myDataSet=(int*)malloc(sizeof(int)*N); //upper bound size is N elements for the rank


  //Write code here

    // need this for every rank

  unsigned int n_fake_buckets = nprocs * 4;

  unsigned int hist[n_fake_buckets][3]; // this is low, high, counter
  unsigned int bucket_ranges[nprocs][3];
  unsigned int step_size = 4;

  double sdtime = MPI_Wtime();

  // generate and distribute ranges
  if ( my_rank == 0 && nprocs > 1) {
    // create fake buckets
    for (size_t i = 0; i < n_fake_buckets; ++i) {
      //Write code here
      hist[i][0] = i * (MAXVAL / n_fake_buckets);

      if (i == n_fake_buckets - 1) {
        hist[i][1] = MAXVAL;
      }

      else {
        hist[i][1] = (i + 1) * (MAXVAL / n_fake_buckets);
      }
    }

    //assign value to eahc in hist
    for (int n = 0; n < localN; ++n) {
      for (int i = 0; i < n_fake_buckets; ++i) {
        if (data[n] >= hist[i][0] && data[n] < hist[i][1]) {
          ++hist[i][2];
          break;
        }
      }
    }

    int current_low = 0;
    size_t proc = 0;
    size_t current_b = 0;
    const size_t b =  localN/nprocs;
    int valid = 0;

    for (int i = 0; i < n_fake_buckets - 1 && proc < nprocs; ++i) {
      // assign values
      // if this is a subsequent run, adjust the bucket for rank 1 by one
      if (current_b >= b) {
        bucket_ranges[proc][0] = current_low;
        bucket_ranges[proc][1] = hist[i][0];
        current_low = hist[i][0];
        //check if second to last -- we can guaruntee that there are at least
        //2 procs
        if (i == n_fake_buckets - 2) {
          valid = 1;
          // if this doesn't work, earlier buckets need to be rearranged
        }
        ++proc;
      } else {
        current_b += hist[i][2];
      }
    }
    //set last bucket
    bucket_ranges[nprocs - 1][0] = bucket_ranges[nprocs - 2][1];
    bucket_ranges[nprocs - 1][1] = MAXVAL;
  } else {
    bucket_ranges[0][0] = 0;
    bucket_ranges[0][1] = MAXVAL;
  }
  
  MPI_Bcast(&bucket_ranges, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

  // count and collect bucket
  int recv_size;
  size_t recv_start = 0; //this is the same as total receive
  size_t my_bucket_size = 0;

  // move waits to after all sends are attempted -- this way all can be done at the same time
  // the time complexity for the sends is n * p which is better than n^2 but I would not know how
  // to lower the time complexity for this

  // this code should also protect against bucket overflow although this is not currently possible

  // this would require a precount, if the value of one bucket is larger than N, the bucket ranges should be adjusted
  // this is not efficient and would need to be repeated
  // for now this receives should just be stopped, possibly discarded although this is not specified in the assignment it is
  // possible to just start the count over and overwrite previous data. In either of these approaches data is lost

  // go through each rank -- each taking a turn as a reciever
  for (size_t i = 0; i < nprocs; ++i) {
    if (my_rank == i) {
      for (size_t send_rank = 0; send_rank < nprocs; ++send_rank) {
        if (send_rank == my_rank) {
          // collect own bucket --
          for( int n = 0; n < localN; ++n ) {
            if(data[n] >= bucket_ranges[my_rank][0] && data[n] < bucket_ranges[my_rank][1]) {
              myDataSet[recv_start] = data[n];
              ++recv_start;
            }
          }
        } else {
          // get size
          MPI_Request req1;
          MPI_Request req2;
          MPI_Irecv(&recv_size, 1, MPI_INT, send_rank, 0, MPI_COMM_WORLD, &req1);
          MPI_Wait(&req1, MPI_STATUS_IGNORE);
          // get numbers
          MPI_Irecv(&myDataSet[recv_start], recv_size, MPI_INT, send_rank, 0, MPI_COMM_WORLD, &req2);
          MPI_Wait(&req2, MPI_STATUS_IGNORE);
          recv_start += recv_size;
        }
      }
    } else {
      // collect and send bucket
      MPI_Request req1;
      MPI_Request req2;

      // collect 
      int index = 0;
      for ( int n = 0; n < localN; ++n ) {
        if(data[n] >= bucket_ranges[i][0] && data[n] < bucket_ranges[i][1]) {
          sendDataSetBuffer[index] = data[n];
          ++index;
        }
      }

      // send the number to collect
      if (index > localN) index = localN; // cap received elements -- with the way the code is set up this is impossible to
      // overflow, as data only contains localn elements
      MPI_Isend(&index, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &req1);
      MPI_Wait(&req1, MPI_STATUS_IGNORE);

      // send the bucket
      MPI_Isend(sendDataSetBuffer, index, MPI_INT, i, 0, MPI_COMM_WORLD, &req2);
      MPI_Wait(&req2, MPI_STATUS_IGNORE);
    }
  }

  double edtime = MPI_Wtime();
  
  size_t pre_sort_sum = sum(recv_start, myDataSet);
  
  double sstime = MPI_Wtime();

  // sort dataset
  qsort(myDataSet, recv_start, sizeof(int), compfn);

  MPI_Barrier(MPI_COMM_WORLD);

  double estime = MPI_Wtime();

  int same_size = pre_sort_sum == sum(recv_start, myDataSet);

  if (same_size) {
    printf("%d, True\n", my_rank);
  } else {
    printf("%d, False\n", my_rank);
  }

  size_t global_sum;

  MPI_Reduce(&pre_sort_sum, &global_sum, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

  if (my_rank == 0) {
    printf("Global Sum: %u\n", global_sum);
    printf("Total distribution time: %f\n", edtime - sdtime);
    printf("Total sort time: %f\n", estime - sstime);
  }

  // debug print dataset
  //printf("%d: ", my_rank);
  //for(int i = 0; i < recv_start; ++i) {
  //  printf("%d, ", myDataSet[i]);
  //}
  //printf("\n");


  //free
  free(data); 
  free(sendDataSetBuffer); 
  free(recvDatasetBuffer); 
  free(myDataSet);

  MPI_Finalize();
  return 0;
}


double randomExponential(double lambda){
    double u = rand() / (RAND_MAX + 1.0);
    return -log(1- u) / lambda;
}

//generates data [0,1000000)
void generateData(int * data, int SIZE)
{
  for (int i=0; i<SIZE; i++)
  {
    double tmp=0; 
    
    //generate value between 0-1 using exponential distribution
    do{
    tmp=randomExponential(4.0);
    // printf(nrnd: %f,tmp);
    }while(tmp>=1.0);
    
    data[i]=tmp*MAXVAL;
    
  }

  
}

