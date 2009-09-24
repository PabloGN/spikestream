#include "ArchiveWidget_V2.h"
using namespace spikestream;


/*! Constructor */
ArchiveWidget_V2::ArchiveWidget_V2(QWidget* parent) : QWidget(parent){
    QVBoxLayout* verticalBox = new QVBoxLayout(this, 2, 2);

    //Add controls to play the loaded archive
    buildTransportControls();
    verticalBox->addWidget(transportControlWidget);
    verticalBox->addSpacing(10);

    gridLayout = new QGridLayout();
    gridLayout->setMargin(10);
    gridLayout->setColumnMinimumWidth(idCol, 50);//Archive ID
    gridLayout->setColumnMinimumWidth(netIDCol, 50);//NetworkID
    gridLayout->setColumnMinimumWidth(dateCol, 100);//Date and time
    gridLayout->setColumnMinimumWidth(descCol, 250);//Description
    gridLayout->setColumnMinimumWidth(loadButCol, 100);//Load button
    gridLayout->setColumnMinimumWidth(delButCol, 100);//Delete button

    QHBoxLayout* gridLayoutHolder = new QHBoxLayout();
    gridLayoutHolder->addLayout(gridLayout);
    gridLayoutHolder->addStretch(5);
    verticalBox->addLayout(gridLayoutHolder);

    //Load the current set of archives, if they exist, into the grid layout
    loadArchiveList();

    //Listen for changes in the network
    connect(Globals::getEventRouter(), SIGNAL(networkChanged()), this, SLOT(loadArchiveList()));

    verticalBox->addStretch(10);
}


/*! Destructor */
ArchiveWidget_V2::~ArchiveWidget_V2(){
}


/*! Deletes an archive */
void ArchiveWidget_V2::deleteArchive(){
    //Get the ID of the network to be deleted
    unsigned int networkID = Util::getUInt(sender()->objectName());
    if(!networkInfoMap.contains(networkID)){
	qCritical()<<"Archive with ID "<<networkID<<" cannot be found.";
	return;
    }

    //Confirm that user wants to take this action.
    QMessageBox msgBox;
    msgBox.setText("Deleting Archive");
    msgBox.setInformativeText("Are you sure that you want to delete archive with ID " + QString::number(networkID) + "? This step cannot be undone.");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();
    if(ret != QMessageBox::Ok)
	return;

    //Delete the archive from the database
    try{
	Globals::getArchiveDao()->deleteArchive(archiveID);
    }
    catch(SpikeStreamException& ex){
	qCritical()<<"Exception thrown when deleting archive.";
    }

    /* If we have deleted the current archive, use event router to inform other classes that the network has changed.
       This will automatically reload the archive list. */
    if(Globals::archiveLoaded() && Globals::getArchive()->getID() == archiveID){
	Globals::seArchive(NULL);
	emit archiveChanged();
    }

    //Otherwise, just reload the network list
    else{
	loadArchiveList();
    }
}


void ArchiveWidget_V2::loadArchiveList(){
    //Reset widget
    reset();

    //If no network is loaded, show no network loaded message and return
    if(!Globals::networkLoaded()){
	gridLayout->addWidget(new QLabel("No network loaded."), 0, 0);
	return;
    }

    //Get a list of the networks in the database
    ArchiveDao* archiveDao = Globals::getArchiveDao();
    QList<ArchivekInfo> archiveInfoList;
    try{
	archiveInfoList = archiveDao->getArchivesInfo(Globals::getNetwork()->getID());
    }
    catch(SpikeStreamException& ex){
	qCritical()<<ex.getMessage();
	return;
    }

    //Show "no archive" message if list is empty
    if(archiveInfoList.size() == 0){
	gridLayout->addWidget(new QLabel("No archives in database"), 0, 0);
	Globals::setArchive(0);
	emit archiveChanged();
	return;
    }

    //Copy network infos into map
    for(int i=0; i<archiveInfoList.size(); ++i){
	archiveInfoMap[archiveInfoList[i].getID()] = archiveInfoList[i];
    }

    /* If the current network is in the network list, then set this as the one loaded
       Otherwise currentNetworkID is set to zero and the user has to choose the loaded network */
    unsigned int currentArchiveID = 0;
    if(Globals::archiveLoaded() && archiveInfoMap.contains(Globals::getArchive()->getID())){
	currentArchiveID = Globals::getArchive()->getID();
    }

    //Display the list in the widget
    for(int i=0; i<archiveInfoList.size(); ++i){
	//Create labels
	QLabel* idLabel = new QLabel(QString::number(archiveInfoList[i].getID()));
	QLabel* networkIDLabel = new QLabel(archiveInfoList[i].getNetworkID());
	QLabel* dateLabel = new QLabel(archiveInfoList[i].getDate());
	QLabel* descriptionLabel = new QLabel(archiveInfoList[i].getDescription());

	//Create the load button and name it with the object id so we can tell which button was invoked
	QPushButton* loadButton = new QPushButton("Load");
	loadButton->setObjectName(QString::number(archiveInfoList[i].getID()));
	connect ( loadButton, SIGNAL(clicked()), this, SLOT( loadArchive() ) );

	//Create the delete button and name it with the object id so we can tell which button was invoked
	QPushButton* deleteButton = new QPushButton("Delete");
	deleteButton->setObjectName(QString::number(archiveInfoList[i].getID()));
	connect ( deleteButton, SIGNAL(clicked()), this, SLOT( deleteNetwork() ) );

	//Set labels and buttons depending on whether it is the current network
	if(currentArchiveID == archiveInfoList[i].getID()){//The curently loaded archive
	    idLabel->setStyleSheet( "QLabel { color: #008000; font-weight: bold; }");
	    networkIDLabel->setStyleSheet( "QLabel { color: #008000; font-weight: bold; }");
	    dateLabel->setStyleSheet( "QLabel { color: #008000; font-weight: bold; }");
	    descriptionLabel->setStyleSheet( "QLabel { color: #008000; font-weight: bold; }");
	    loadButton->setEnabled(false);
	}
	else{//An archive that is not loaded
	    idLabel->setStyleSheet( "QLabel { color: #777777; }");
	    networkIDLabel->setStyleSheet( "QLabel { color: #777777; }");
	    dateLabel->setStyleSheet( "QLabel { color: #777777; }");
	    descriptionLabel->setStyleSheet( "QLabel { color: #777777; }");
	}

	//Add the widgets to the layout
	gridLayout->addWidget(idLabel, i, idCol);
	gridLayout->addWidget(networkIDLabel, i, netIDCol);
	gridLayout->addWidget(dateLabel, i, dateCol);
	gridLayout->addWidget(descriptionLabel, i, descCol);
	gridLayout->addWidget(loadButton, i, loadButCol);
	gridLayout->addWidget(deleteButton, i, delButCol);
    }
}


/*----------------------------------------------------------*/
/*-----                 PRIVATE SLOTS                  -----*/
/*----------------------------------------------------------*/
void ArchiveWidget_V2::rewindButtonPressed(){
}
void ArchiveWidget_V2::playButtonToggled(bool on){
}
void ArchiveWidget_V2::stepButtonPressed(){
}
void ArchiveWidget_V2::fastForwardButtonToggled(bool on){
}
void ArchiveWidget_V2::stopButtonPressed(){
}
void ArchiveWidget_V2::frameRateComboChanged(int){
    }



/*----------------------------------------------------------*/
/*-----                PRIVATE METHODS                 -----*/
/*----------------------------------------------------------*/

/*! Adds the transport controls to the supplied layout */
void ArchiveWidget_V2::buildTransportControls(){
    //Set up pixmaps to control play and stop
    QPixmap playPixmap(SpikeStreamMainWindow::workingDirectory + "/images/play.xpm");
    QPixmap stepPixmap(SpikeStreamMainWindow::workingDirectory + "/images/step.xpm");
    QPixmap stopPixmap(SpikeStreamMainWindow::workingDirectory + "/images/stop.xpm");
    QPixmap rewindPixmap(SpikeStreamMainWindow::workingDirectory + "/images/rewind.xpm");
    QPixmap fastForwardPixmap(SpikeStreamMainWindow::workingDirectory + "/images/fast_forward.xpm");

    transportControlWidget = new QWidget(this);
    QHBoxLayout *archTransportBox = new QHBoxLayout(transportControlWidget);
    QPushButton* rewindButton = new QPushButton(QIcon(rewindPixmap));
    rewindButton->setBaseSize(30, 30);
    rewindButton->setMaximumSize(30, 30);
    rewindButton->setMinimumSize(30, 30);
    connect (rewindButton, SIGNAL(clicked()), this, SLOT(rewindButtonPressed()));
    archTransportBox->addSpacing(10);
    archTransportBox->addWidget(rewindButton);

    QPushButton* playButton = new QPushButton(QIcon(playPixmap));
    playButton->setToggleButton(true);
    playButton->setBaseSize(100, 30);
    playButton->setMaximumSize(100, 30);
    playButton->setMinimumSize(100, 30);
    connect (playButton, SIGNAL(toggled(bool)), this, SLOT(playButtonToggled(bool)));
    archTransportBox->addWidget(playButton);

    QPushButton* stepButton = new QPushButton(QIcon(stepPixmap));
    stepButton->setBaseSize(50, 30);
    stepButton->setMaximumSize(50, 30);
    stepButton->setMinimumSize(50, 30);
    connect (stepButton, SIGNAL(clicked()), this, SLOT(stepButtonPressed()));
    archTransportBox->addWidget(stepButton);

    QPushButton* fastForwardButton = new QPushButton(QIcon(fastForwardPixmap));
    fastForwardButton->setToggleButton(true);
    fastForwardButton->setBaseSize(30, 30);
    fastForwardButton->setMaximumSize(30, 30);
    fastForwardButton->setMinimumSize(30, 30);
    connect (fastForwardButton, SIGNAL(toggled(bool)), this, SLOT(fastForwardButtonToggled(bool)));
    archTransportBox->addWidget(fastForwardButton);

    QPushButton* topButton = new QPushButton(QIcon(stopPixmap));
    stopButton->setBaseSize(50, 30);
    stopButton->setMaximumSize(50, 30);
    stopButton->setMinimumSize(50, 30);
    connect (stopButton, SIGNAL(clicked()), this, SLOT(stopButtonPressed()));
    archTransportBox->addWidget(stopButton);
    archTransportBox->addSpacing(10);

    archFrameRateBox->addWidget( new QLabel("Frames per second") );
    QComboBox* frameRateCombo = new QComboBox();
    frameRateCombo->insertItem("1");
    frameRateCombo->insertItem("5");
    frameRateCombo->insertItem("10");
    frameRateCombo->insertItem("15");
    frameRateCombo->insertItem("20");
    frameRateCombo->insertItem("25");
    connect(frameRateCombo, SIGNAL(activated(int)), this, SLOT(frameRateComboChanged(int)));
    archTransportBox->addWidget(frameRateCombo);
    archTransportBox->addStretch(5);

    //Disable widget and all children - will be re-enabled when an archive is loaded
    transportControlWidget->setEnabled(false);
}


/*! Loads a particular network into memory */
void ArchiveWidget_V2::loadArchive(){
    unsigned int archiveID = sender()->objectName().toUInt();
    if(!archiveInfoMap.contains(archiveID)){
	qCritical()<<"Archive with ID "<<archiveID<<" cannot be found.";
	return;
    }

    //load the network
    loadArchive(archiveInfoMap[archiveID]);
}


void ArchiveWidget_V2::loadArchive(ArchiveInfo& archiveInfo){
    if(!archiveInfoMap.contains(archiveInfo.getID())){
	qCritical()<<"Archive with ID "<<archiveInfo.getID()<<" cannot be found.";
	return;
    }

    // Create new archive
    Archive* newArchive = new Archive(archiveInfo);
    Globals::setArchive(newArchive);
}


/*! Resets the state of the widget.
    Deleting the widget automatically removes it from the layout. */
void ArchiveWidget_V2::reset(){
    //Remove no networks label if it exists
    if(archiveInfoMap.size() == 0){
    	QLayoutItem* item = gridLayout->itemAtPosition(0, 0);
	if(item != 0){
	    delete item->widget();
	}
	return;
    }

    //Remove list of archives
    for(int i=0; i<archiveInfoMap.size(); ++i){
	QLayoutItem* item = gridLayout->itemAtPosition(i, idCol);
	if(item != 0){
	    item->widget()->deleteLater();
	}
	item = gridLayout->itemAtPosition(i, netIDCol);
	if(item != 0){
	    item->widget()->deleteLater();
	}
	item = gridLayout->itemAtPosition(i, dateCol);
	if(item != 0){
	    item->widget()->deleteLater();
	}
	item = gridLayout->itemAtPosition(i, descCol);
	if(item != 0){
	    item->widget()->deleteLater();
	}
	item = gridLayout->itemAtPosition(i, loadButCol);
	if(item != 0){
	    item->widget()->deleteLater();
	}
	item = gridLayout->itemAtPosition(i, delButCol);
	if(item != 0){
	    item->widget()->deleteLater();
	}
    }
    archiveInfoMap.clear();
}

