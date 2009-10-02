#include "ArchiveDao.h"
#include "GlobalVariables.h"
#include "SpikeStreamDBException.h"
#include "Util.h"
using namespace spikestream;

/*! Constructor */
ArchiveDao::ArchiveDao(const DBInfo& dbInfo) : AbstractDao(dbInfo){
}


/*! Destructor */
ArchiveDao::~ArchiveDao(){
}


/*----------------------------------------------------------*/
/*-----                 PUBLIC METHODS                 -----*/
/*----------------------------------------------------------*/

/*! Adds the archive with the specified ID */
void ArchiveDao::addArchive(ArchiveInfo& archInfo){
    QSqlQuery query = getQuery("INSERT INTO Archives (StartTime, NetworkID, Description) VALUES (" + QString::number(archInfo.getDateTime().toTime_t()) + ", " + QString::number(archInfo.getNetworkID()) + ", '" + archInfo.getDescription() + "')");
    executeQuery(query);

    //Check id is correct and add to network info if it is
    int lastInsertID = query.lastInsertId().toInt();
    if(lastInsertID >= START_ARCHIVE_ID)
	archInfo.setID(lastInsertID);
    else
	throw SpikeStreamDBException("Insert ID for Archives is invalid: " + QString::number(lastInsertID));
}


/*! Adds data to the archive with the specified ID */
void ArchiveDao::addArchiveData(unsigned int archiveID, unsigned int timeStep, const QString& firingNeuronString){
    executeQuery("INSERT INTO ArchiveData(ArchiveID, TimeStep, FiringNeurons) VALUES (" + QString::number(archiveID) + ", " + QString::number(timeStep) + ", '"  + firingNeuronString + "')");
}


/*! Deletes the archive with the specified ID */
void ArchiveDao::deleteArchive(unsigned int archiveID){
    executeQuery("DELETE FROM Archives WHERE ArchiveID = " + QString::number(archiveID));
}


/*! Returns a list of the archives in the database that are associated with the specified network. */
QList<ArchiveInfo> ArchiveDao::getArchivesInfo(unsigned int networkID){
    QSqlQuery query = getQuery("SELECT ArchiveID, StartTime, Description FROM Archives WHERE NetworkID=" + QString::number(networkID) + " ORDER BY StartTime");
    executeQuery(query);
    QList<ArchiveInfo> tmpList;
    for(int i=0; i<query.size(); ++i){
	query.next();
	unsigned int archiveID = Util::getUInt(query.value(0).toString());
	tmpList.append(
		ArchiveInfo(
			archiveID,
			networkID,
			Util::getUInt(query.value(1).toString()),//Start time as a unix timestamp
			query.value(2).toString()//Description
		)
	);
    }
    return tmpList;
}


/*! Returns the number of data rows in the specified archive */
int ArchiveDao::getArchiveSize(unsigned int archiveID){
    QSqlQuery query = getQuery("SELECT COUNT(*) FROM ArchiveData WHERE ArchiveID=" + QString::number(archiveID));
    executeQuery(query);
    query.next();
    unsigned int archiveSize = Util::getInt(query.value(0).toString());
    return archiveSize;
}


/*! Returns the maximum time step in the archive */
unsigned int ArchiveDao::getMaxTimeStep(unsigned int archiveID){
    QSqlQuery query = getQuery("SELECT MAX(TimeStep) FROM ArchiveData WHERE ArchiveID=" + QString::number(archiveID));
    executeQuery(query);
    query.next();
    return Util::getUInt(query.value(0).toString());
}


/*! Returns a string containing the comma separated list of neuron IDs */
QStringList ArchiveDao::getFiringNeuronIDs(unsigned int archiveID, unsigned int timeStep){
    QSqlQuery query = getQuery("SELECT FiringNeurons FROM ArchiveData WHERE TimeStep=" + QString::number(timeStep) + " AND ArchiveID=" + QString::number(archiveID));
    executeQuery(query);
    //Return empty string if no entries for this time step
    if(query.size() == 0)
	return QStringList();

    //Return the list of firing neuron ids
    query.next();
    return query.value(0).toString().split(",");
}






