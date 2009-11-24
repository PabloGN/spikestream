/***************************************************************************
 *   SpikeStream Application                                               *
 *   Copyright (C) 2007 by David Gamez                                     *
 *   david@davidgamez.eu                                                   *
 *   Version 0.1                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

//SpikeStream includes
#include "NeuronGroupHolder.h"
#include "Debug.h"

//Other includes
#include <iostream>
using namespace std;


/*! Constructor. */
NeuronGroupHolder::NeuronGroupHolder(){
	//Set pointers to null so that their deletion does not create problems if they are not used.
	neuronIDArray = NULL;
	xPosArray = NULL;
	yPosArray = NULL;
	zPosArray = NULL;
}


/*! Removes data structures associated with this class. */
NeuronGroupHolder::~NeuronGroupHolder(){
	#ifdef MEMORY_DEBUG
		cout<<"DELETING NEURON GROUP HOLDER"<<endl;
	#endif//MEMORY_DEBUG
	
	delete [] neuronIDArray;
	delete [] xPosArray;
	delete [] yPosArray;
	delete [] zPosArray;
}
