/*
 * BoardModel.h
 *
 *  Created on: 25 Jan 2011
 *      Author: thomas
 */

#ifndef BOARDMODEL_H_
#define BOARDMODEL_H_

#include <string>
#include <string.h>
#include <stdlib.h>

class BoardModel
{
	public:
		BoardModel(std::string * inComponents, unsigned int guestEnd, unsigned int weaponEnd, unsigned int inNumComponents);
		BoardModel(const unsigned char * serialised);
		virtual ~BoardModel();

		unsigned int getNumComponents();
		unsigned int getFirstGuest();
		unsigned int getFirstWeapon();
		unsigned int getFirstRoom();
		unsigned int getLastGuest();
		unsigned int getLastWeapon();
		unsigned int getLastRoom();
		std::string * getComponentNames();

		unsigned char * serialise(unsigned int & size);

	private:
		unsigned int firstGuest;
		unsigned int firstWeapon;
		unsigned int firstRoom;
		unsigned int lastGuest;
		unsigned int lastWeapon;
		unsigned int lastRoom;
		unsigned int numGuests;
		unsigned int numWeapons;
		unsigned int numRooms;
		unsigned int numComponents;

		static const unsigned int numAttributes = 10;

		std::string * componentNames;
};

#endif /* BOARDMODEL_H_ */
