#include <iostream>
#include <mpi.h>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <cassert>
#include <math.h>
#include <chrono>

using namespace std;

int main ( int argc , char **argv )
{
  int pid, nprocs;  
  MPI_Init (&argc , &argv) ;
  MPI_Comm_rank(MPI_COMM_WORLD, &pid ) ;
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs ) ;
  
  chrono::time_point<chrono::system_clock> start, end;


  int n = atoi(argv[1]);
  int root = atoi(argv[2]);
  int is_init_tab_monotone = atoi(argv[3]);
  
  if (root < 0 || root >= nprocs) {
  	MPI_Finalize();
  	if (pid == 0) {
  		cout << "Impossible d'éxecuter sur cette racine : " << root << endl;
  	}
  	return 0;
  }
  
  //INIT SUITE (Tableau U)
  int * suite;
  if (pid==root) {
 	  suite = new int[n];
    if(is_init_tab_monotone == 1){
      for (int i=0; i<n; i++) {
          suite[i] = i;
        }
        
    }else{
      for (int i=0; i<n; i++) {
        suite[i] = (int) (rand()%100);
      }   
    }
  }

  if (pid==root) {
      start = chrono::system_clock::now();
  }

  int* sendcounts;
  int* displ;
  int n_local = n/nprocs;
  int reste = n%nprocs;
  if (pid==root){
    sendcounts = new int[nprocs];
    displ = new int[nprocs];
    int ptr = 0;
    for (int i = 0; i < reste; i++){
      sendcounts[i] = n_local+1;
      displ[i] = ptr;
      ptr += (n_local+1);
    }
    for(int i = reste; i < nprocs; i++){
      sendcounts[i] = n_local;
      displ[i] = ptr;
      ptr += n_local;
    }
  }

  
 
  if (pid<reste)
    n_local++;

  int * suite_local = new int[n_local];
  MPI_Scatterv(suite,sendcounts,displ,MPI_INT,suite_local,n_local,MPI_INT,root,MPI_COMM_WORLD);
  
  int is_croissant = 1;
  int is_decroissant = 1;
  int is_monotone = 1;
  for (int i=1; i<n_local; i++) {
    //si il est pas croissant
    if(suite_local[i-1] > suite_local[i]){
      is_croissant = 0;
    }
    // si il est pas decroissant
    if(suite_local[i-1] < suite_local[i]){
      is_decroissant = 0;
    }
    if(!is_decroissant && !is_croissant){
      is_monotone = 0;
      break;
    }
  }

  if(pid % 2 == 0){
        MPI_Ssend(&suite_local[n_local-1], 1, MPI_INT, pid+1, 0, MPI_COMM_WORLD);
  }else{
        int last_value_voisin_prec;
        MPI_Recv(&last_value_voisin_prec, 1, MPI_INT, pid-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if(is_decroissant == 1){
          //si decroissant, verifions si le voisin a une valeur inferieur dans ce cas il est non monotone
          if(last_value_voisin_prec < suite_local[0]){
            is_monotone = false;
          }
        }
        if(is_croissant == 1){
          //si croissant, verifions si le voisin a une valeur superieur dans ce cas il est non monotone
          if(last_value_voisin_prec > suite_local[0]){
            is_monotone = false;
          }
        }

  }

   // REDUCE sur monotonne avec un produit 
   int root_monotone;
   MPI_Reduce(&is_monotone, &root_monotone, 1, MPI_INT, MPI_PROD, root, MPI_COMM_WORLD);
   
   if (pid == root) {
    cout << "Tableau monotone : " << root_monotone << endl;
    end = chrono::system_clock::now();
    chrono::duration<double> elapsed_seconds = end-start;
    cout << "tps=" << elapsed_seconds.count() << endl;
   }
  
  
  MPI_Finalize() ;
  return 0 ;
}


// mpi run -np 8 ./vecteur 16 0
