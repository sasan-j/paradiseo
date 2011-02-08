//Init the number of threads per block
#define BLOCK_SIZE 256

#include <iostream>  
#include <stdlib.h> 
using namespace std;

// The general include for eo
#include <eo>
#include <ga.h>
// OneMax full eval function
#include <problems/eval/EvalOneMax.h>
// OneMax increment eval function
#include <eval/moCudaVectorEval.h>
#include <problems/eval/OneMaxIncrEval.h>
// One Max solution
#include <cudaType/moCudaBitVector.h>
// One Max neighbor
#include <neighborhood/moCudaBitNeighbor.h>
// One Max ordered neighborhood
#include <neighborhood/moCudaOrderNeighborhood.h>
// The Solution and neighbor comparator
#include <comparator/moNeighborComparator.h>
#include <comparator/moSolNeighborComparator.h>
// The continuator
#include <continuator/moTrueContinuator.h>
// Local search algorithm
#include <algo/moLocalSearch.h>
// Simple HC algorithm
#include <algo/moSimpleHC.h>
// The simple HC algorithm explorer
#include <explorer/moSimpleHCexplorer.h>
//To compute execution time
#include <performance/moCudaTimer.h>

//------------------------------------------------------------------------------------
// Define types of the representation solution, different neighbors and neighborhoods
//------------------------------------------------------------------------------------
// REPRESENTATION
typedef moCudaBitVector<eoMaximizingFitness> solution;
typedef moCudaBitNeighbor <solution> Neighbor;
typedef moCudaOrderNeighborhood<Neighbor> Neighborhood;

void main_function(int argc, char **argv)
{

  /* =========================================================
   *
   * Parameters
   *
   * ========================================================= */

  // First define a parser from the command-line arguments
  eoParser parser(argc, argv);

  // For each parameter, define Parameter, read it through the parser,
  // and assign the value to the variable

  // seed
  eoValueParam<uint32_t> seedParam(time(0), "seed", "Random number seed", 'S');
  parser.processParam( seedParam );
  unsigned seed = seedParam.value();

  // description of genotype
  eoValueParam<unsigned int> vecSizeParam(8, "vecSize", "Genotype size", 'V');
  parser.processParam( vecSizeParam, "Representation" );
  unsigned vecSize = vecSizeParam.value();

  // the name of the "status" file where all actual parameter values will be saved
  string str_status = parser.ProgramName() + ".status"; // default value
  eoValueParam<string> statusParam(str_status.c_str(), "status", "Status file");
  parser.processParam( statusParam, "Persistence" );

  // do the following AFTER ALL PARAMETERS HAVE BEEN PROCESSED
  // i.e. in case you need parameters somewhere else, postpone these
  if (parser.userNeedsHelp()) {
    parser.printHelp(cout);
    exit(1);
  }
  if (statusParam.value() != "") {
    ofstream os(statusParam.value().c_str());
    os << parser;// and you can use that file as parameter file
  }
  /* =========================================================
   *
   * Random seed
   *
   * ========================================================= */

  //reproducible random seed: if you don't change SEED above,
  // you'll aways get the same result, NOT a random run
  rng.reseed(seed);
  srand(seed);

  
  /* =========================================================
   *
   * Initilisation of the solution
   *
   * ========================================================= */
  
  solution sol(vecSize);

  /* =========================================================
   *
   * Eval fitness function
   *
   * ========================================================= */

  EvalOneMax<solution> eval;
  
  /* =========================================================
   *
   * Evaluation of a solution neighbor's
   *
   * ========================================================= */
  
  OneMaxIncrEval<Neighbor> incr_eval;
  moCudaVectorEval<Neighbor,OneMaxIncrEval<Neighbor> > cueval(vecSize,incr_eval);
  
  /* =========================================================
   *
   * Comparator of solutions and neighbors
   *
   * ========================================================= */

  moNeighborComparator<Neighbor> comparator;
  moSolNeighborComparator<Neighbor> solComparator;

  /* =========================================================
   *
   * a solution neighborhood
   *
   * ========================================================= */

  Neighborhood neighborhood(vecSize,cueval);

  /* =========================================================
   *
   * An explorer of solution neighborhood's
   *
   * ========================================================= */

  moSimpleHCexplorer<Neighbor> explorer(neighborhood, cueval,
					comparator, solComparator);

  /* =========================================================
   *
   * The local search algorithm
   *
   * ========================================================= */
  //True continuator <=> Always continue

  moTrueContinuator<Neighbor> continuator;

  moLocalSearch<Neighbor> localSearch(explorer,continuator, eval);

  /* =========================================================
   *
   * The simple Hill Climbing algorithm
   *
   * ========================================================= */

  moSimpleHC<Neighbor> simpleHC(neighborhood,eval,cueval);

  /* =========================================================
   *
   * Execute the local search from random sollution
   *
   * ========================================================= */
 
  //Can be eval here, else it will be done at the beginning of the localSearch
  eval(sol);

  std::cout << "initial: " << sol<< std::endl;
  // Create timer for timing CUDA calculation
  moCudaTimer timer;
  timer.start();
  localSearch(sol);
  timer.stop();
  printf("CUDA execution time = %f ms\n",timer.getTime());
  timer.deleteTimer();
  std::cout << "final:   " << sol << std::endl;

  /* =========================================================
   *
   * Execute the Simple Hill climbing from random sollution
   *
   * ========================================================= */
  cout<<endl;
  solution sol1(vecSize);
  eval(sol1);
  std::cout << "initial: " << sol1<< std::endl;
  // Create timer for timing CUDA calculation
  moCudaTimer timer1;
  timer1.start();
  simpleHC(sol1);
  timer1.stop();
  std::cout << "final:   " << sol1 << std::endl;
  printf("CUDA execution time = %f ms\n",timer1.getTime());
  timer1.deleteTimer();
}

// A main that catches the exceptions

int main(int argc, char **argv)
{
  try{
    main_function(argc, argv);
  }
  catch(exception& e){
    cout << "Exception: " << e.what() << '\n';
  }
  return 1;
}
