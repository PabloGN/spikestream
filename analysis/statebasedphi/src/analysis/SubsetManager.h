#ifndef SUBSETMANAGER_H
#define SUBSETMANAGER_H

//SpikeStream includes
#include "DBInfo.h"
#include "AnalysisInfo.h"
#include "NetworkDao.h"
#include "ArchiveDao.h"
#include "PhiCalculator.h"
#include "StateBasedPhiAnalysisDao.h"
#include "Subset.h"
using namespace spikestream;

//Qt includes
#include <QObject>

namespace spikestream {

    class SubsetManager : public QObject {
	Q_OBJECT

	public:
	    SubsetManager(const DBInfo& netDBInfo, const DBInfo& archDBInfo, const DBInfo& anaDBInfo, const AnalysisInfo& anaInfo, unsigned int timeStep);
	    ~SubsetManager();
	    void buildSubsetList();
	    void identifyComplexes();
	    void runCalculation(const bool * const stop);

	signals:
	    void complexFound();
	    void progress(unsigned int timeStep, unsigned int stepsCompleted, unsigned int totalSteps);


	private:
	    //========================  VARIABLES  ========================
	    /*! Wrapper around the network database */
	    NetworkDao* networkDao;

	    /*! Wrapper around the archive database */
	    ArchiveDao* archiveDao;

	    /*! Wrapper around the analysis database */
	    StateBasedPhiAnalysisDao* stateDao;

	    /*! Information about the analysis being run */
	    AnalysisInfo analysisInfo;

	    /*! The time step that is being analyzed by this thread */
	    int timeStep;

	    /*! Pointer to the stop variable in the controlling thread. */
	    const bool* stop;

	    /*! List of all of the neuron IDs in the network */
	    QList<unsigned int> neuronIDList;

	    /*! Complete list of possible subsets
		FIXME: COULD BE MADE MUCH MORE EFFICIENT IF THIS WAS ONLY THE CONNECTED SUBSETS */
	    QList<Subset*> subsetList;

	    /*! Class that carries out the phi calculations */
	    PhiCalculator* phiCalculator;

	    //========================  METHODS  ============================
	    void addSubset(bool subsetSelectionArray[], int arrayLength);
	    void calculateSubsetsPhi();
	    void deleteSubsets();
    };

}

#endif//SUBSETMANAGER_H
