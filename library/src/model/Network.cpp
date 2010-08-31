//SpikeStream includes
#include "GlobalVariables.h"
#include "Network.h"
#include "SpikeStreamException.h"
using namespace spikestream;

//Other includes
#include <iostream>
using namespace std;

/*! Constructor that creates a new network and adds it to the database */
Network::Network(const QString& name, const QString& description, const DBInfo& networkDBInfo, const DBInfo& archiveDBInfo){
    //Store information
    this->info.setName(name);
    this->info.setDescription(description);
	this->networkDBInfo = networkDBInfo;
	this->archiveDBInfo = archiveDBInfo;

    //Create network dao threads for heavy operations
	neuronNetworkDaoThread = new NetworkDaoThread(networkDBInfo);
    connect (neuronNetworkDaoThread, SIGNAL(finished()), this, SLOT(neuronThreadFinished()));
	connectionNetworkDaoThread = new NetworkDaoThread(networkDBInfo);
    connect (connectionNetworkDaoThread, SIGNAL(finished()), this, SLOT(connectionThreadFinished()));

    //Initialize variables
    currentNeuronTask = -1;
    currentConnectionTask = -1;
	saved = false;
    clearError();

    //Create new network in database. ID will be stored in the network info
	NetworkDao networkDao(networkDBInfo);
	networkDao.addNetwork(info);
}


/*! Constructor when using an existing network */
Network::Network(const NetworkInfo& networkInfo, const DBInfo& networkDBInfo, const DBInfo& archiveDBInfo){
    //Store information
    this->info = networkInfo;
	this->networkDBInfo = networkDBInfo;
	this->archiveDBInfo = archiveDBInfo;

    //Check that network ID is valid
    if(info.getID() == INVALID_NETWORK_ID){
		throw SpikeStreamException ("Attempting to load an invalid network");
    }

    //Create network dao threads for heavy operations
	neuronNetworkDaoThread = new NetworkDaoThread(networkDBInfo);
    connect (neuronNetworkDaoThread, SIGNAL(finished()), this, SLOT(neuronThreadFinished()));
	connectionNetworkDaoThread = new NetworkDaoThread(networkDBInfo);
    connect (connectionNetworkDaoThread, SIGNAL(finished()), this, SLOT(connectionThreadFinished()));

    //Initialize variables
    currentNeuronTask = -1;
    currentConnectionTask = -1;
	saved = true;
    clearError();

    //Load up basic information about the neuron and connection groups
    loadNeuronGroupsInfo();
    loadConnectionGroupsInfo();
}


/*! Destructor */
Network::~Network(){
    if(neuronNetworkDaoThread != NULL){
		neuronNetworkDaoThread->stop();
		neuronNetworkDaoThread->wait();
		delete neuronNetworkDaoThread;
    }
    if(connectionNetworkDaoThread != NULL){
		connectionNetworkDaoThread->stop();
		connectionNetworkDaoThread->wait();
		delete connectionNetworkDaoThread;
    }

    //Empties all data stored in the class
    deleteConnectionGroups();
    deleteNeuronGroups();
}



/*--------------------------------------------------------- */
/*-----                PUBLIC METHODS                 ----- */
/*--------------------------------------------------------- */

/*! Adds connection groups to the network. */
void Network::addConnectionGroups(QList<ConnectionGroup*>& connectionGroupList){
	if(!prototypeMode && hasArchives())//Check if network is editable or not
		throw SpikeStreamException("Cannot add connection groups to a locked network.");

	//In prototype mode, we add connection groups to network and store them in a list for later
	if(prototypeMode){
		foreach(ConnectionGroup* conGrp, connectionGroupList){
			unsigned tmpID = getTemporaryConGrpID();//Get an ID for the connection groups
			connGrpMap[tmpID] = conGrp;//Add connection group to network
			newConnectionGroups.append(conGrp);//Store connection group in list for adding to database later
		}
		savedStateChanged(false);
	}
	//In normal mode, connection groups are saved to the database
	else{
		//Store the list of connection groups to be added
		newConnectionGroups = connectionGroupList;

		//Start thread that adds connection groups to database
		clearError();
		connectionNetworkDaoThread->prepareAddConnectionGroups(getID(), connectionGroupList);
		currentConnectionTask = ADD_CONNECTIONS_TASK;
		connectionNetworkDaoThread->start();
	}
}


/*! Adds neuron groups to the network */
void Network::addNeuronGroups(QList<NeuronGroup*>& neuronGroupList){
	if(!prototypeMode && hasArchives())//Check if network is editable or not
		throw SpikeStreamException("Cannot add neuron groups to a locked network.");

	//In prototype mode, we add connection groups to network and store them in a list for later
	if(prototypeMode){
		foreach(NeuronGroup* neurGrp, neuronGroupList){
			unsigned tmpID = getTemporaryNeurGrpID();//Get an ID for the connection groups
			neurGrpMap[tmpID] = neurGrp;//Add connection group to network
			newNeuronGroups.append(neurGrp);//Store connection group in list for adding to database later
		}
		savedStateChanged(false);
	}
	//In normal mode, connection groups are saved to the database
	else{
		//Store the list of neuron groups to be added later when the thread has finished and we have the correct ID
		newNeuronGroups = neuronGroupList;

		//Start thread that adds neuron groups to database
		clearError();
		neuronNetworkDaoThread->prepareAddNeuronGroups(getID(), neuronGroupList);
		currentNeuronTask = ADD_NEURONS_TASK;
		neuronNetworkDaoThread->start();
	}
}


/*! Cancels thread-based operations that are in progress */
void Network::cancel(){
    neuronNetworkDaoThread->stop();
    connectionNetworkDaoThread->stop();
    currentNeuronTask = -1;
    currentConnectionTask = -1;
}


/*! Clears the error state */
void Network::clearError(){
	error = false;
	errorMessage = "";
}


/*! Returns true if a neuron with the specified ID is in the network */
bool Network::containsNeuron(unsigned int neurID){
    //Need to check each neuron group to see if it contains the neuron.
    for(QHash<unsigned int, NeuronGroup*>::iterator iter = neurGrpMap.begin(); iter != neurGrpMap.end(); ++iter){
		if(iter.value()->contains(neurID)){
			return true;
		}
    }
    return false;
}


/*! Returns true if the network contains the neuron group with the specified ID. */
bool  Network::containsNeuronGroup(unsigned int neuronGroupID){
	if(neurGrpMap.contains(neuronGroupID))
		return true;
	return false;
}


/*! Removes the specified connection groups from the network and database.
	Throws an exception if the connection groups cannot be found */
void Network::deleteConnectionGroups(QList<unsigned int>& conGrpIDList){
	//Check if network is editable or not
	if(hasArchives())
		throw SpikeStreamException("Cannot delete connection groups from a locked network.");

	//Check that network is not currently busy with some other task
	if(connectionNetworkDaoThread->isRunning())
		throw SpikeStreamException("Network is busy with another connection-related task.");

	//Check that connection group ids exist in network
	foreach(unsigned int conGrpID, conGrpIDList){
		if(!connGrpMap.contains(conGrpID))
			throw SpikeStreamException("Connection group ID " + QString::number(conGrpID) + " cannot be found in the current network.");
	}

	//Store the list of neuron groups to be added later when the thread has finished and we have the correct ID
	deleteConnectionGroupIDs = conGrpIDList;

	//Start thread that deletes connection groups from database
	clearError();
	connectionNetworkDaoThread->prepareDeleteConnectionGroups(getID(), conGrpIDList);
	currentConnectionTask = DELETE_CONNECTIONS_TASK;
	connectionNetworkDaoThread->start();
}


/*! Removes the specified neuron groups from the network and database.
	Throws an exception if the neuron groups cannot be found */
void Network::deleteNeuronGroups(QList<unsigned int>& neurGrpIDList){
	//Check if network is editable or not
	if(hasArchives())
		throw SpikeStreamException("Cannot delete neuron groups from a locked network.");

	//Check that network is not currently busy with some other task
	if(neuronNetworkDaoThread->isRunning())
		throw SpikeStreamException("Network is busy with another neuron-related task.");

	//Check that neuron group ids exist in network
	foreach(unsigned int neurGrpID, neurGrpIDList){
		if(!neurGrpMap.contains(neurGrpID))
			throw SpikeStreamException("Neuron group ID " + QString::number(neurGrpID) + " cannot be found in the current network.");
	}

	//Store the list of neuron groups to be added later when the thread has finished and we have the correct ID
	deleteNeuronGroupIDs = neurGrpIDList;

	//Start thread that deletes neuron groups from database
	clearError();
	neuronNetworkDaoThread->prepareDeleteNeuronGroups(getID(), neurGrpIDList);
	currentNeuronTask = DELETE_NEURONS_TASK;
	neuronNetworkDaoThread->start();

	/* Delete connections to or from the neuron groups being deleted from memory
		The database deletion will be done automatically by the database */
	QList<unsigned int> deleteConGrpIDs;
	for(QHash<unsigned int, ConnectionGroup*>::iterator iter = connGrpMap.begin(); iter != connGrpMap.end(); ++iter){
		foreach(unsigned int neurGrpID, neurGrpIDList){
			if(iter.value()->getFromNeuronGroupID() == neurGrpID || iter.value()->getToNeuronGroupID() == neurGrpID){
				deleteConGrpIDs.append(iter.key());
			}
		}
	}
	//Do the deletion outside of the map iteration
	foreach(unsigned tmpConGrpID, deleteConGrpIDs){
		if(!connGrpMap.contains(tmpConGrpID))
			throw SpikeStreamException("Connection group ID " + QString::number(tmpConGrpID) + " cannot be found in network.");
		delete connGrpMap[tmpConGrpID];
		connGrpMap.remove(tmpConGrpID);
	}
}


/*! Returns a complete list of connection groups */
QList<ConnectionGroup*> Network::getConnectionGroups(){
	QList<ConnectionGroup*> tmpList;
	for(QHash<unsigned int, ConnectionGroup*>::iterator iter = connGrpMap.begin(); iter != connGrpMap.end(); ++iter)
		tmpList.append(iter.value());
	return tmpList;
}


/*! Returns a complete list of connection group infos */
QList<ConnectionGroupInfo> Network::getConnectionGroupsInfo(){
    QList<ConnectionGroupInfo> tmpList;
    for(QHash<unsigned int, ConnectionGroup*>::iterator iter = connGrpMap.begin(); iter != connGrpMap.end(); ++iter)
		tmpList.append(iter.value()->getInfo());
    return tmpList;
}


/*! Returns a list of connection group infos filtered by the specified synapse type ID */
QList<ConnectionGroupInfo> Network::getConnectionGroupsInfo(unsigned int synapseTypeID){
	QList<ConnectionGroupInfo> tmpList;
	for(QHash<unsigned int, ConnectionGroup*>::iterator iter = connGrpMap.begin(); iter != connGrpMap.end(); ++iter){
		if(iter.value()->getInfo().getSynapseTypeID() == synapseTypeID)
			tmpList.append(iter.value()->getInfo());
	}
	return tmpList;
}


/*! Returns a list of pointers to connections that are appropriate for the connection mode */
QList<Connection*> Network::getConnections(unsigned connectionMode, unsigned singleNeuronID, unsigned toNeuronID){
	QList<Connection*> conList;//List of connections to return
	QList<ConnectionGroup*> conGrpList;//List of connection groups that contain the single neuron

	//Return empty list if connection mode is disabled
	if( !(connectionMode & CONNECTION_MODE_ENABLED) )
		return conList;

	//Get connection groups that include the single neuron ID
	for(QHash<unsigned int, ConnectionGroup*>::iterator iter = connGrpMap.begin(); iter != connGrpMap.end(); ++iter){
		if(iter.value()->contains(singleNeuronID))
			conGrpList.append(iter.value());
	}

	//Showing connections FROM singleNeuronID TO toNeuronID
	if(connectionMode & SHOW_BETWEEN_CONNECTIONS){
		foreach(ConnectionGroup* tmpConGrp, conGrpList){//Work through all connection groups including this neuron
			QList<Connection*> tmpConList = tmpConGrp->getFromConnections(singleNeuronID);
			for(int i=0; i<tmpConList.size(); ++i){
				Connection* tmpCon = tmpConList.at(i);
				if(!filterConnection(tmpCon, connectionMode)){
					if(tmpCon->toNeuronID == toNeuronID)
						conList.append(tmpCon);
				}
			}
		}
		//Return list of between connections
		return conList;
	}

	//Filter by FROM or TO in single neuron connection mode
	if(connectionMode & SHOW_FROM_CONNECTIONS){
		//Collect connections FROM the single neuron ID
		foreach(ConnectionGroup* tmpConGrp, conGrpList){//Work through all connection groups including this neuron
			QList<Connection*> tmpConList = tmpConGrp->getFromConnections(singleNeuronID);
			for(int i=0; i<tmpConList.size(); ++i){
				Connection* tmpCon = tmpConList.at(i);
				if(!filterConnection(tmpCon, connectionMode))
					conList.append(tmpCon);
			}
		}
	}
	else if(connectionMode & SHOW_TO_CONNECTIONS){
		//Collect connections TO the single neuron ID
		foreach(ConnectionGroup* tmpConGrp, conGrpList){//Work through all connection groups including this neuron
			QList<Connection*> tmpConList = tmpConGrp->getToConnections(singleNeuronID);
			for(int i=0; i<tmpConList.size(); ++i){
				Connection* tmpCon = tmpConList.at(i);
				if(!filterConnection(tmpCon, connectionMode))
					conList.append(tmpCon);
			}
		}
	}
	else {//Collect connections TO and FROM the single neuron ID
		foreach(ConnectionGroup* tmpConGrp, conGrpList){//Work through all connection groups including this neuron
			QList<Connection*> tmpConList = tmpConGrp->getFromConnections(singleNeuronID);
			for(int i=0; i<tmpConList.size(); ++i){
				Connection* tmpCon = tmpConList.at(i);
				if(!filterConnection(tmpCon, connectionMode))
					conList.append(tmpCon);
			}
			tmpConList = tmpConGrp->getToConnections(singleNeuronID);
			for(int i=0; i<tmpConList.size(); ++i){
				Connection* tmpCon = tmpConList.at(i);
				if(!filterConnection(tmpCon, connectionMode))
					conList.append(tmpCon);
			}
		}
	}

	//Return the list
	return conList;
}


/*! Returns a list of the neuron groups in the network. */
QList<NeuronGroup*> Network::getNeuronGroups(){
	QList<NeuronGroup*> tmpList;
	for(QHash<unsigned int, NeuronGroup*>::iterator iter = neurGrpMap.begin(); iter != neurGrpMap.end(); ++iter)
		tmpList.append(iter.value());
	return tmpList;
}


/*! Returns a complete list of neuron group infos */
QList<NeuronGroupInfo> Network::getNeuronGroupsInfo(){
    QList<NeuronGroupInfo> tmpList;
    for(QHash<unsigned int, NeuronGroup*>::iterator iter = neurGrpMap.begin(); iter != neurGrpMap.end(); ++iter)
		tmpList.append(iter.value()->getInfo());
    return tmpList;
}


/*! Returns a list of neuron group infos filtered by the specified neuron type ID */
QList<NeuronGroupInfo> Network::getNeuronGroupsInfo(unsigned int neuronTypeID){
	QList<NeuronGroupInfo> tmpList;
	for(QHash<unsigned int, NeuronGroup*>::iterator iter = neurGrpMap.begin(); iter != neurGrpMap.end(); ++iter){
		if(iter.value()->getInfo().getNeuronTypeID() == neuronTypeID)
			tmpList.append(iter.value()->getInfo());
	}
	return tmpList;
}


/*! Returns the number of neurons that connect to the specified neuron */
int Network::getNumberOfToConnections(unsigned int neuronID){
    //Check neuron id is in the network
    if(!containsNeuron(neuronID))
		throw SpikeStreamException("Request for number of connections to a neuron that is not in the network.");

    //Count up the number of connections to this neuron in each connection group
    int toConCount = 0;
    for(QHash<unsigned int, ConnectionGroup*>::iterator iter = connGrpMap.begin(); iter != connGrpMap.end(); ++iter){
		//Get the number of connections to the neuron in this connection group
		toConCount += iter.value()->getToConnections(neuronID).size();
    }

    //Return final count
    return toConCount;
}


/*! Returns true if heavy thread-based operations are in progress */
bool Network::isBusy() {
    if(neuronNetworkDaoThread->isRunning() || connectionNetworkDaoThread->isRunning())
		return true;
    return false;
}


/*! Returns a list of the neuron group ids in the network */
QList<unsigned int> Network::getNeuronGroupIDs(){
    return neurGrpMap.keys();
}


/*! Returns a list of the connection group ids in the network */
QList<unsigned int> Network::getConnectionGroupIDs(){
    return connGrpMap.keys();
}


/*! Returns a box that encloses the network */
Box Network::getBoundingBox(){
    /* Neurons are not directly linked with a neuron id, so need to work through
       each neuron group and create a box that includes all the other boxes. */
    Box networkBox;
    bool firstTime = true;
    QList<NeuronGroup*> tmpNeurGrpList = neurGrpMap.values();
    for(QList<NeuronGroup*>::iterator iter = tmpNeurGrpList.begin(); iter != tmpNeurGrpList.end(); ++iter){
		if(firstTime){//Take box enclosing first neuron group as a starting point
			networkBox = (*iter)->getBoundingBox();
			firstTime = false;
		}
		else{//Expand box to include subsequent neuron groups
			Box neurGrpBox = (*iter)->getBoundingBox();
			if(neurGrpBox.x1 < networkBox.x1)
				networkBox.x1 = neurGrpBox.x1;
			if(neurGrpBox.y1 < networkBox.y1)
				networkBox.y1 = neurGrpBox.y1;
			if(neurGrpBox.z1 < networkBox.z1)
				networkBox.z1 = neurGrpBox.z1;

			if(neurGrpBox.x2 > networkBox.x2)
				networkBox.x2 = neurGrpBox.x2;
			if(neurGrpBox.y2 > networkBox.y2)
				networkBox.y2 = neurGrpBox.y2;
			if(neurGrpBox.z2 > networkBox.z2)
				networkBox.z2 = neurGrpBox.z2;
		}
    }
    return networkBox;
}


/*! Returns a box that encloses the specified neuron group */
Box Network::getNeuronGroupBoundingBox(unsigned int neurGrpID){
	return getNeuronGroup(neurGrpID)->getBoundingBox();
}


/*! Returns the neuron group with the specified id.
    An exception is thrown if the neuron group id is not in the network.
    Lazy loading used, so that neuron groups are loaded only if requested. */
NeuronGroup* Network::getNeuronGroup(unsigned int id){
    checkNeuronGroupID(id);
    return neurGrpMap[id];
}


/*! Returns information about the neuron group with the specified id */
NeuronGroupInfo Network::getNeuronGroupInfo(unsigned int id){
    checkNeuronGroupID(id);
    return neurGrpMap[id]->getInfo();
}


/*! Returns the connection group with the specified id.
    An exception is thrown if the connection group id is not in the network.*/
ConnectionGroup* Network::getConnectionGroup(unsigned int id){
    checkConnectionGroupID(id);
    return connGrpMap[id];
}


/*! Returns true if the connection group in the network matches the connection group in the datatabase */
bool Network::connectionGroupIsLoaded(unsigned int id){
    checkConnectionGroupID(id);
    return connGrpMap[id]->isLoaded();
}


/*! Returns true if the neuron group in the network matches the neuron group in the database */
bool Network::neuronGroupIsLoaded(unsigned int id){
    checkNeuronGroupID(id);
    return neurGrpMap[id]->isLoaded();
}


/*! Returns information about the connection group with the specified id */
ConnectionGroupInfo Network::getConnectionGroupInfo(unsigned int id){
    checkConnectionGroupID(id);
    return connGrpMap[id]->getInfo();
}


/*! Returns the message associated with an error */
QString Network::getErrorMessage(){
	return errorMessage;
}


/*! Returns the number of steps that have been completed so far during a heavy operation. */
int Network::getNumberOfCompletedSteps(){
	int numSteps = 0;
	if(neuronNetworkDaoThread->isRunning())
		numSteps += neuronNetworkDaoThread->getNumberOfCompletedSteps();
	if(connectionNetworkDaoThread->isRunning())
		numSteps += connectionNetworkDaoThread->getNumberOfCompletedSteps();
	return numSteps;
}


/*! Returns the number of steps involved in the current tasks */
int Network::getTotalNumberOfSteps(){
	int total = 0;
	if(neuronNetworkDaoThread->isRunning())
		total += neuronNetworkDaoThread->getTotalNumberOfSteps();
	if(connectionNetworkDaoThread->isRunning())
		total += connectionNetworkDaoThread->getTotalNumberOfSteps();
	return total;
}


/*! Returns true if the network is not editable because it is associated with archives */
bool Network::hasArchives(){
	ArchiveDao archiveDao(archiveDBInfo);
	return archiveDao.networkHasArchives(getID());
}


/*! Returns true if the network matches the database */
bool Network::isSaved(){
	if(prototypeMode && !saved)
		return false;
	return true;
}


/*! Loads up the network from the database using separate threads */
void Network::load(){
    clearError();

    //Load up all neurons
    neuronNetworkDaoThread->prepareLoadNeurons(neurGrpMap.values());
    currentNeuronTask = LOAD_NEURONS_TASK;
    neuronNetworkDaoThread->start();

	//Load all connection groups
	connectionNetworkDaoThread->prepareLoadConnections(connGrpMap.values());
    currentConnectionTask = LOAD_CONNECTIONS_TASK;
    connectionNetworkDaoThread->start();
}



/*! Loads up the network from the database without using separate threads.
	Only returns when load is complete. Mainly used for testing */
void Network::loadWait(){
	clearError();

	//Load up all neurons
	neuronNetworkDaoThread->prepareLoadNeurons(neurGrpMap.values());
	currentNeuronTask = LOAD_NEURONS_TASK;
	neuronNetworkDaoThread->start();
	neuronNetworkDaoThread->wait();
	neuronThreadFinished();

	//Load all connection groups
	connectionNetworkDaoThread->prepareLoadConnections(connGrpMap.values());
	currentConnectionTask = LOAD_CONNECTIONS_TASK;
	connectionNetworkDaoThread->start();
	connectionNetworkDaoThread->wait();
	connectionThreadFinished();
}


/*! Saves the network.
	Throws an exception if network is not in prototyping mode. */
void Network::save(){
	if(!prototypeMode)
		throw SpikeStreamException("Network should not be saved unless it is in prototype mode.");

	//--------------------------------------------------
	//     HANDLE ADDED NEURON AND CONNECTION GROUPS
	//--------------------------------------------------
	//Remove connection and neuron groups from network - they will be added later with the correct IDs.
	foreach(ConnectionGroup* tmpConGrp, newConnectionGroups)
		connGrpMap.remove(tmpConGrp->getID());
	foreach(NeuronGroup* tmpNeurGrp, newNeuronGroups)
		neurGrpMap.remove(tmpNeurGrp->getID());

	//Start thread that adds connection groups to database
	clearError();
	connectionNetworkDaoThread->prepareAddConnectionGroups(getID(), newConnectionGroups);
	currentConnectionTask = ADD_CONNECTIONS_TASK;
	connectionNetworkDaoThread->start();

	//Start thread that adds connection groups to database
	neuronNetworkDaoThread->prepareAddNeuronGroups(getID(), newNeuronGroups);
	currentNeuronTask = ADD_NEURONS_TASK;
	neuronNetworkDaoThread->start();


	//--------------------------------------------------
	//   HANDLE DELETED NEURON AND CONNECTION GROUPS
	//--------------------------------------------------
}


/*! Puts the network into prototype mode */
void Network::setPrototypeMode(bool mode){
	prototypeMode = mode;
}


/*--------------------------------------------------------- */
/*-----                  PRIVATE SLOTS                ----- */
/*--------------------------------------------------------- */

/*! Slot called when thread processing connections has finished running. */
void Network::connectionThreadFinished(){
    if(connectionNetworkDaoThread->isError()){
		setError("Connection Loading Error: '" + connectionNetworkDaoThread->getErrorMessage() + "'. ");
    }
	if(!error){
		try{
			switch(currentConnectionTask){
				case DELETE_CONNECTIONS_TASK:
					//Remove deleted connection groups from memory
					foreach(unsigned int conGrpID,  deleteConnectionGroupIDs){
						if(!connGrpMap.contains(conGrpID))
							throw SpikeStreamException("Connection group ID " + QString::number(conGrpID) + " cannot be found in network.");
						delete connGrpMap[conGrpID];
						connGrpMap.remove(conGrpID);
					}
				break;
				case ADD_CONNECTIONS_TASK:
					//Add connection groups to network
					for(QList<ConnectionGroup*>::iterator iter = newConnectionGroups.begin(); iter != newConnectionGroups.end(); ++iter){
						if(connGrpMap.contains((*iter)->getID()))
							throw SpikeStreamException("Connection group with ID " + QString::number((*iter)->getID()) + " is already present in the network.");
						connGrpMap[(*iter)->getID()] = *iter;
						newConnectionGroups.clear();
					}
				break;
				case LOAD_CONNECTIONS_TASK:
					;//Nothing to do at present
				break;
				default:
					throw SpikeStreamException("The current task ID has not been recognized.");
			}
		}
		catch(SpikeStreamException& ex){
			setError(" End connection thread error " + ex.getMessage());
		}
	}

    //Reset task
    currentConnectionTask = -1;

    if(!isBusy())
		emit taskFinished();
}


/*! Slot called when thread processing neurons has finished running. */
void Network::neuronThreadFinished(){
    //Check for errors
    if(neuronNetworkDaoThread->isError()){
		setError("Neuron Loading Error: '" + neuronNetworkDaoThread->getErrorMessage() + "'. ");
    }

    //Handle any task-specific stuff
    if(!error){
		try{
			switch(currentNeuronTask){
				case ADD_NEURONS_TASK:
					//Add neuron groups to the network now that they have the correct ID
					for(QList<NeuronGroup*>::iterator iter = newNeuronGroups.begin(); iter != newNeuronGroups.end(); ++iter){
						//Check to see if ID already exists - error in this case
						if( neurGrpMap.contains( (*iter)->getID() ) ){
							throw SpikeStreamException("Adding neurons task - trying to add a neuron group with ID " + QString::number((*iter)->getID()) + " that already exists in the network.");
						}

						//Store neuron group
						neurGrpMap[ (*iter)->getID() ] = *iter;
						newNeuronGroups.clear();
					}
				break;
				case DELETE_NEURONS_TASK:
					//Remove deleted neuron groups from memory
					foreach(unsigned int neurGrpID,  deleteNeuronGroupIDs){
						if(!neurGrpMap.contains(neurGrpID))
							throw SpikeStreamException("Neuron group ID " + QString::number(neurGrpID) + " cannot be found in network.");
						delete neurGrpMap[neurGrpID];
						neurGrpMap.remove(neurGrpID);
					}
				break;
				case LOAD_NEURONS_TASK:
					;//Nothing to do at present
				break;
				default:
					throw SpikeStreamException("The current task ID has not been recognized.");
			}
		}
		catch(SpikeStreamException& ex){
			setError("End neuron thread error: " + ex.getMessage());
		}
    }

    //Reset task
    currentNeuronTask = -1;

    //Emit signal that task has finished if no other threads are carrying out operations
    if(!isBusy())
		emit taskFinished();
}


/*! Returns the number of neurons in the network.
	Throws exception if this is called when network is not loaded. */
int Network::size(){
	if(this->isBusy())
		throw SpikeStreamException("Size of network cannot be determined while network is busy.");

	int neurCnt = 0;
	for(QHash<unsigned int, NeuronGroup*>::iterator iter = neurGrpMap.begin(); iter != neurGrpMap.end(); ++iter)
		neurCnt += iter.value()->size();
	return neurCnt;
}


/*--------------------------------------------------------- */
/*-----                PRIVATE METHODS                ----- */
/*--------------------------------------------------------- */

/*! Checks that the specified neuron group id is present in the network
    and throws an exception if not. */
void Network::checkConnectionGroupID(unsigned int id){
    if(!connGrpMap.contains(id))
		throw SpikeStreamException(QString("Connection group with id ") + QString::number(id) + " is not in network with id " + QString::number(getID()));
}


/*! Checks that the specified connection group id is present in the network
    and throws an exception if not. */
void Network::checkNeuronGroupID(unsigned int id){
    if(!neurGrpMap.contains(id))
		throw SpikeStreamException(QString("Neuron group with id ") + QString::number(id) + " is not in network with id " + QString::number(getID()));
}


/*! Applies connection mode filters to the specified connection and returns
	true if it should not be displayed. */
bool Network::filterConnection(Connection *connection, unsigned connectionMode){
	//Filter by weight
	if(connectionMode & SHOW_POSITIVE_CONNECTIONS){
		if(connection->weight < 0 || connection->tempWeight < 0)
			return true;
	}
	else if(connectionMode & SHOW_NEGATIVE_CONNECTIONS){
		if(connection->weight >= 0 || connection->tempWeight >= 0)
			return true;
	}
	return false;
}


/*! Returns an ID that is highly unlikey to conflict with database IDs
	for use as a temporary connection group ID. */
unsigned Network::getTemporaryConGrpID(){
	unsigned tmpID = 0xffffffff;
	while (connGrpMap.contains(tmpID)){
		--tmpID;
		if(tmpID == 0)
			throw SpikeStreamException("Cannot find a temporary connection ID");
	}
	return tmpID;
}


/*! Returns an ID that is highly unlikey to conflict with database IDs
	for use as a temporary neuron group ID. */
unsigned Network::getTemporaryNeurGrpID(){
	unsigned tmpID = 0xffffffff;
	while (neurGrpMap.contains(tmpID)){
		--tmpID;
		if(tmpID == 0)
			throw SpikeStreamException("Cannot find a temporary neuron group ID");
	}
	return tmpID;
}


/*! Uses network dao to obtain list of connection groups and load them into hash map. */
void Network::loadConnectionGroupsInfo(){
    //Clear hash map
    deleteConnectionGroups();

    //Get list of neuron groups from database
	NetworkDao networkDao(networkDBInfo);
	QList<ConnectionGroupInfo> connGrpInfoList = networkDao.getConnectionGroupsInfo(getID());

    //Copy list into hash map for faster access
    for(int i=0; i<connGrpInfoList.size(); ++i)
		connGrpMap[ connGrpInfoList[i].getID() ] = new ConnectionGroup(connGrpInfoList[i]);
}


/*! Uses network dao to obtain list of neuron groups and load them into hash map.
    Individual neurons are loaded separately to enable lazy loading if required.  */
void Network::loadNeuronGroupsInfo(){
    //Clear hash map
    deleteNeuronGroups();

    //Get list of neuron groups from database
	NetworkDao networkDao(networkDBInfo);
	QList<NeuronGroupInfo> neurGrpInfoList = networkDao.getNeuronGroupsInfo(getID());

    //Copy list into hash map for faster access
    for(int i=0; i<neurGrpInfoList.size(); ++i)
		neurGrpMap[ neurGrpInfoList[i].getID() ] = new NeuronGroup(neurGrpInfoList[i]);
}


/*! Deletes all connection groups */
void Network::deleteConnectionGroups(){
    for(QHash<unsigned int, ConnectionGroup*>::iterator iter = connGrpMap.begin(); iter != connGrpMap.end(); ++iter)
		delete iter.value();
    connGrpMap.clear();
}


/*! Deletes all neuron groups */
void Network::deleteNeuronGroups(){
    //Delete all neuron groups
    for(QHash<unsigned int, NeuronGroup*>::iterator iter = neurGrpMap.begin(); iter != neurGrpMap.end(); ++iter)
		delete iter.value();
    neurGrpMap.clear();
}


/*! Called when the network is changed in memory when editing in prototype mode. */
void Network::savedStateChanged(bool newSavedState){
	if(!prototypeMode)
		throw SpikeStreamException("Saved state should only change in prototype mode.");
	saved = newSavedState;
}


/*! Sets the class into error state and adds error message */
void Network::setError(const QString& errorMsg){
	this->errorMessage += " " + errorMsg;
	error = true;
}
