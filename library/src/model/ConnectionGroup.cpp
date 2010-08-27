//SpikeStream includes
#include "ConnectionGroup.h"
#include "SpikeStreamException.h"
using namespace spikestream;

//Other includes
#include <iostream>
using namespace std;


/*! Constructor */
ConnectionGroup::ConnectionGroup(const ConnectionGroupInfo& connGrpInfo){
    this->info = connGrpInfo;
    loaded = false;
	connectionMap = new QHash<unsigned, Connection*>();
}


/*! Destructor */
ConnectionGroup::~ConnectionGroup(){
	//Deletes connection map and all its dynamically allocated objects
	if(connectionMap != NULL){
		clearConnections();
		delete connectionMap;
	}
}


/*--------------------------------------------------------*/
/*-------             PUBLIC METHODS               -------*/
/*--------------------------------------------------------*/

/*! Adds a connection to the group */
Connection* ConnectionGroup::addConnection(Connection* newConn){
	//Check that we do not already have this connection
	if(connectionMap->contains(newConn->getID()))
		throw SpikeStreamException("Connection with ID " + QString::number(newConn->getID()) + " already exists in this group.");

	//Store connection
	(*connectionMap)[newConn->getID()] = newConn;

    //Store connection data in easy to access format
    fromConnectionMap[newConn->fromNeuronID].append(newConn);
    toConnectionMap[newConn->toNeuronID].append(newConn);

    //Return pointer to connection
    return newConn;
}


/*! Returns iterator pointing to beginning of connection group */
QList<Connection*>::const_iterator ConnectionGroup::begin(){
	return connectionMap->values().begin();
}


/*! Returns true if the connection group contains the specified neuron ID */
bool ConnectionGroup::contains(unsigned neuronID){
	if(fromConnectionMap.contains(neuronID) || toConnectionMap.contains(neuronID))
		return true;
	return false;
}


/*! Returns iterator pointing to end of connection group */
QList<Connection*>::const_iterator ConnectionGroup::end(){
	return connectionMap->values().end();
}


/*! Returns list of connections.
	Not an efficient way to access connections - use iterators instead.*/
QList<Connection*> ConnectionGroup::getConnections(){
	return connectionMap->values();
}


/*! Removes all connections from this group */
void ConnectionGroup::clearConnections(){
	QList<Connection*>::const_iterator endConnList = this->end();
	for(QList<Connection*>::const_iterator iter = this->begin(); iter != endConnList; ++iter){
		delete *iter;
    }
	connectionMap->clear();
    fromConnectionMap.clear();
    toConnectionMap.clear();
    loaded = false;
}


/*! Returns a list of connections from the neuron with this ID.
    Empty list is returned if neuron id cannot be found. */
QList<Connection*> ConnectionGroup::getFromConnections(unsigned int neurID){
    if(!fromConnectionMap.contains(neurID))
		return QList<Connection*>();//Returns an empty connection list without filling map with invalid from and to neurons
    return fromConnectionMap[neurID];
}


/*! Returns the value of a named parameter.
	Throws an exception if the parameter cannot be found. */
double ConnectionGroup::getParameter(const QString& paramName){
	if(!parameterMap.contains(paramName))
		throw SpikeStreamException("Cannot find parameter with key: " + paramName + " in connection group with ID " + QString::number(info.getID()));
	return  parameterMap[paramName];
}


/*! Returns a list of connections to the neuron with the specified id.
    Exception is thrown if the neuron cannot be found. */
QList<Connection*> ConnectionGroup::getToConnections(unsigned int neurID){
	if(!toConnectionMap.contains(neurID))
		return QList<Connection*>();//Returns an empty connection list without filling map with invalid from and to neurons
	return toConnectionMap[neurID];
}


/*! Replaces the connection map with a new neuron map, most likely to fix connection IDs.
	Connections are not cleaned up because they might be included in the new map. */
void ConnectionGroup::setConnectionMap(QHash<unsigned, Connection *> *newConnectionMap){
	delete connectionMap;
	this->connectionMap = newConnectionMap;
}


/*! Sets the weight of a specific connection */
void ConnectionGroup::setTempWeight(unsigned connectionID, float tempWeight){
	if(!connectionMap->contains(connectionID))
		throw SpikeStreamException("Failed to set temp weight. Connection with ID " + QString::number(connectionID) + " does not exist in this connection group.");
	(*connectionMap)[connectionID]->setTempWeight(tempWeight);
}


/*! Sets the weight of a specific connection */
void ConnectionGroup::setWeight(unsigned connectionID, float weight){
	if(!connectionMap->contains(connectionID))
		throw SpikeStreamException("Failed to set weight. Connection with ID " + QString::number(connectionID) + " does not exist in this connection group.");
	(*connectionMap)[connectionID]->setWeight(weight);
}

