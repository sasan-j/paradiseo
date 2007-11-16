/*
* <peoSyncMultiStart.h>
* Copyright (C) DOLPHIN Project-Team, INRIA Futurs, 2006-2007
* (C) OPAC Team, LIFL, 2002-2007
*
* Sebastien Cahon, Alexandru-Adrian Tantar
*
* This software is governed by the CeCILL license under French law and
* abiding by the rules of distribution of free software.  You can  use,
* modify and/ or redistribute the software under the terms of the CeCILL
* license as circulated by CEA, CNRS and INRIA at the following URL
* "http://www.cecill.info".
*
* As a counterpart to the access to the source code and  rights to copy,
* modify and redistribute granted by the license, users are provided only
* with a limited warranty  and the software's author,  the holder of the
* economic rights,  and the successive licensors  have only  limited liability.
*
* In this respect, the user's attention is drawn to the risks associated
* with loading,  using,  modifying and/or developing or reproducing the
* software by the user in light of its specific status of free software,
* that may mean  that it is complicated to manipulate,  and  that  also
* therefore means  that it is reserved for developers  and  experienced
* professionals having in-depth computer knowledge. Users are therefore
* encouraged to load and test the software's suitability as regards their
* requirements in conditions enabling the security of their systems and/or
* data to be ensured and,  more generally, to use and operate it in the
* same conditions as regards security.
* The fact that you are presently reading this means that you have had
* knowledge of the CeCILL license and that you accept its terms.
*
* ParadisEO WebSite : http://paradiseo.gforge.inria.fr
* Contact: paradiseo-help@lists.gforge.inria.fr
*
*/

#ifndef __peoSyncMultiStart_h
#define __peoSyncMultiStart_h

#include <utils/eoUpdater.h>
#include <moAlgo.h>

#include <eoSelect.h>
#include <eoReplacement.h>
#include <eoContinue.h>

#include "core/service.h"
#include "core/messaging.h"
#include "core/peo_debug.h"


extern int getNodeRank();


//! Class providing the basis for the synchronous multi-start model.

//! The peoSyncMultiStart class provides the basis for implementing the synchronous multi-start model,
//! for launching several solution-based algorithms in parallel on a specified initial population. As a simple
//! example, several hill climbing algorithms may be synchronously launched on the specified population, each
//! algorithm acting upon one individual only, the final result being integrated back in the population. A
//! peoSyncMultiStart object can be specified as checkpoint object for a classic ParadisEO evolutionary algorithm
//! thus allowing for simple hybridization schemes which combine the evolutionary approach with a local search approach,
//! for example, executed at the end of each generation.
template< class EOT > class peoSyncMultiStart : public Service, public eoUpdater
  {

  public:

    //! Constructor function - several simple parameters are required for defining the characteristics of the multi-start model.
    //!
    //! @param eoContinue< EOT >& __cont - defined for including further functionality - no semantics associated at this time;
    //! @param eoSelect< EOT >& __select - selection strategy for obtaining a subset of the initial population on which to apply the specified algorithm;
    //! @param eoReplacement< EOT >& __replace - replacement strategy for integrating the resulting individuals in the initial population;
    //! @param moAlgo< EOT >& __ls - algorithm to be applied on each of the selected individuals - a <b>moAlgo< EOT ></b>-derived object must be specified;
    //! @param eoPop< EOT >& __pop - the initial population from which the individuals are selected for applying the specified algorithm.
    peoSyncMultiStart(

      eoContinue< EOT >& __cont,
      eoSelect< EOT >& __select,
      eoReplacement< EOT >& __replace,
      moAlgo< EOT >& __ls,
      eoPop< EOT >& __pop
    );

    //! Operator which synchronously executes the specified	algorithm on the individuals selected from the initial population.
    //! There is no need to explicitly call the operator - automatically called as checkpoint operator.
    void operator()();

    //! Auxiliary function for transferring data between the process requesting the synchronous execution of the specified
    //! algorithm and the process which actually executes the algorithm. There is no need to explicitly call the function.
    void packData();

    //! Auxiliary function for transferring data between the process requesting the synchronous execution of the specified
    //! algorithm and the process which actually executes the algorithm. There is no need to explicitly call the function.
    void unpackData();

    //! Auxiliary function for actually executing the specified algorithm on one assigned individual. There is no need to
    //! explicitly call the function.
    void execute();

    //! Auxiliary function for transferring data between the process requesting the synchronous execution of the specified
    //! algorithm and the process which actually executes the algorithm. There is no need to explicitly call the function.
    void packResult();

    //! Auxiliary function for transferring data between the process requesting the synchronous execution of the specified
    //! algorithm and the process which actually executes the algorithm. There is no need to explicitly call the function.
    void unpackResult();

    //! Auxiliary function for notifications between the process requesting the synchronous multi-start execution
    //! and the processes that performs the actual execution phase. There is no need to explicitly call the function.
    void notifySendingData();

    //! Auxiliary function for notifications between the process requesting the synchronous multi-start execution
    //! and the processes that performs the actual execution phase. There is no need to explicitly call the function.
    void notifySendingAllResourceRequests();

  private:

    eoContinue< EOT >& cont;
    eoSelect< EOT >& select;
    eoReplacement< EOT >& replace;

    moAlgo< EOT >& ls;

    eoPop< EOT >& pop;
    eoPop< EOT > sel;
    eoPop< EOT > impr_sel;

    EOT sol;
    unsigned idx;
    unsigned num_term;
  };


template< class EOT > peoSyncMultiStart< EOT > :: peoSyncMultiStart(

  eoContinue < EOT >& __cont,
  eoSelect< EOT >& __select,
  eoReplacement< EOT >& __replace,
  moAlgo < EOT >& __ls,
  eoPop< EOT >& __pop

) : cont( __cont ), select( __select ), replace( __replace ), ls( __ls ), pop( __pop )
{}


template< class EOT > void peoSyncMultiStart< EOT > :: packData()
{

  :: pack( sel[ idx++ ] );
}


template< class EOT > void peoSyncMultiStart< EOT > :: unpackData()
{

  unpack( sol );
}


template< class EOT > void peoSyncMultiStart< EOT > :: execute()
{

  ls( sol );
}


template< class EOT > void peoSyncMultiStart< EOT > :: packResult()
{

  pack( sol );
}


template< class EOT > void peoSyncMultiStart< EOT > :: unpackResult()
{

  unpack( sol );
  impr_sel.push_back( sol );
  num_term++;

  if ( num_term == sel.size() )
    {

      getOwner()->setActive();
      replace( pop, impr_sel );

      printDebugMessage( "replacing the improved individuals in the population." );
      resume();
    }
}


template< class EOT > void peoSyncMultiStart< EOT > :: operator()()
{

  printDebugMessage( "performing the parallel multi-start hybridization." );
  select( pop, sel );
  impr_sel.clear();
  idx = num_term = 0;
  requestResourceRequest( sel.size() );
  stop();
}


template< class EOT > void peoSyncMultiStart< EOT > :: notifySendingData()
{}


template< class EOT > void peoSyncMultiStart< EOT > :: notifySendingAllResourceRequests()
{

  getOwner()->setPassive();
}


#endif
