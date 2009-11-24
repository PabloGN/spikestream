#include "Connection.h"
using namespace spikestream;


Connection::Connection (unsigned int fromNeuronID, unsigned int toNeuronID, float delay, float weight, float tempWeight){
    this->fromNeuronID = fromNeuronID;
    this->toNeuronID = toNeuronID;
    this->delay = delay;
    this->weight = weight;
    this->tempWeight = tempWeight;
}


Connection::Connection (unsigned int id, unsigned int fromNeuronID, unsigned int toNeuronID, float delay, float weight, float tempWeight){
    this->id = id;
    this->fromNeuronID = fromNeuronID;
    this->toNeuronID = toNeuronID;
    this->delay = delay;
    this->weight = weight;
    this->tempWeight = tempWeight;
}


Connection::Connection(const Connection& conn){
    this->id = conn.id;
    this->fromNeuronID = conn.fromNeuronID;
    this->toNeuronID = conn.toNeuronID;
    this->delay = conn.delay;
    this->weight = conn.weight;
    this->tempWeight = conn.tempWeight;
}


/*! Assignment operator */
Connection& Connection::operator=(const Connection& rhs){
    //Check for self assignment
    if(this == &rhs)
	return *this;

    this->id = rhs.id;
    this->fromNeuronID = rhs.fromNeuronID;
    this->toNeuronID = rhs.toNeuronID;
    this->delay = rhs.delay;
    this->weight = rhs.weight;
    this->tempWeight = rhs.tempWeight;

    return *this;
}