/*
 * BoardModel.cpp
 *
 *  Created on: 25 Jan 2011
 *      Author: thomas
 */

#include "BoardModel.h"

BoardModel::BoardModel(std::string * inComponents, unsigned int guestEnd, unsigned int weaponEnd, unsigned int inNumComponents)
{
	numComponents = inNumComponents;

	firstGuest = 0;
	firstWeapon = guestEnd + 1;
	firstRoom = weaponEnd + 1;
	lastGuest = guestEnd;
	lastWeapon = weaponEnd;
	lastRoom = numComponents - 1;
	numGuests = lastGuest + 1;
	numWeapons = lastWeapon + 1 - numGuests;
	numRooms = lastRoom + 1 - numWeapons - numGuests;

	componentNames = new std::string[numComponents];

	for(unsigned int i = 0; i < numComponents; i++)
	{
		componentNames[i] = inComponents[i];
	}
}

BoardModel::BoardModel(const unsigned char * serialised)
{
	unsigned int * integerRepresentation = (unsigned int *)serialised;

	firstGuest = integerRepresentation[0];
	firstWeapon = integerRepresentation[1];
	firstRoom = integerRepresentation[2];
	lastGuest = integerRepresentation[3];
	lastWeapon = integerRepresentation[4];
	lastRoom = integerRepresentation[5];
	numGuests = integerRepresentation[6];
	numWeapons = integerRepresentation[7];
	numRooms = integerRepresentation[8];
	numComponents = integerRepresentation[9];

	componentNames = new std::string[numComponents];

	char * stringOffset = (char *)&integerRepresentation[10];

	for(unsigned int i = 0; i < numComponents; i++)
	{
		componentNames[i].assign(stringOffset);

		while(*stringOffset != '\0')
		{
			stringOffset++;
		}

		stringOffset++;
	}
}

BoardModel::~BoardModel()
{
	delete [] componentNames;
}

unsigned char * BoardModel::serialise(unsigned int & size)
{
	unsigned int stringLengths = 0;

	for(unsigned int i = 0; i < numComponents; i++)
	{
		stringLengths += componentNames[i].length();
	}

	stringLengths += numComponents;

	size = sizeof(unsigned int) * numAttributes + sizeof(char) * stringLengths;

	unsigned char * serialisation = (unsigned char *)malloc(size);

	unsigned int * integerRepresentation = (unsigned int *)serialisation;

	integerRepresentation[0] = firstGuest;
	integerRepresentation[1] = firstWeapon;
	integerRepresentation[2] = firstRoom;
	integerRepresentation[3] = lastGuest;
	integerRepresentation[4] = lastWeapon;
	integerRepresentation[5] = lastRoom;
	integerRepresentation[6] = numGuests;
	integerRepresentation[7] = numWeapons;
	integerRepresentation[8] = numRooms;
	integerRepresentation[9] = numComponents;

	char * stringOffset = (char *)&integerRepresentation[10];

	for(unsigned int i = 0; i < numComponents; i++)
	{
		strcpy(stringOffset, componentNames[i].c_str());
		stringOffset += componentNames[i].length() + 1;
	}

	return serialisation;
}

unsigned int BoardModel::getNumComponents()
{
	return numComponents;
}

unsigned int BoardModel::getFirstGuest()
{
	return firstGuest;
}

unsigned int BoardModel::getFirstWeapon()
{
	return firstWeapon;
}

unsigned int BoardModel::getFirstRoom()
{
	return firstRoom;
}

unsigned int BoardModel::getLastGuest()
{
	return lastGuest;
}

unsigned int BoardModel::getLastWeapon()
{
	return lastWeapon;
}

unsigned int BoardModel::getLastRoom()
{
	return lastRoom;
}

std::string * BoardModel::getComponentNames()
{
	return componentNames;
}

