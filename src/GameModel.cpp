/*
 * GameModel.cpp
 *
 *  Created on: 9 Jan 2011
 *      Author: thomas
 */

#include "GameModel.h"

GameModel::GameModel(BoardModel * inBoard, const unsigned int inNumPlayers, const vector<unsigned int> inNumCardsPerPlayer, const vector<unsigned int> & inGlobalCards) : myBoardModel(inBoard), numPlayers(inNumPlayers), numCardsPerPlayer(inNumCardsPerPlayer)
{
    if(numPlayers > 1)
    {
        unsigned int possessedSum = 0;

        for(unsigned int i = 0; i < numPlayers; i++)
        {
            possessedSum += numCardsPerPlayer[i];
        }

        numGlobalCards = myBoardModel->getNumComponents() - possessedSum - 3;

        if(numGlobalCards < 0 || numGlobalCards != (int)inGlobalCards.size())
        {
#if PRINT_DEBUG
            cout << "Cards per player / global cards miscount!" << endl;
#endif
        }
        else
        {
            undoHistorySize = 0;

            guest = weapon = room = -1;

            possessionMatrix = new unsigned int * [numPlayers + 1];

            for(unsigned int i = 0; i < numPlayers + 1; i++)
            {
            	possessionMatrix[i] = new unsigned int[myBoardModel->getNumComponents()];
            }

            numCardsDiscovered = (unsigned int *)malloc(sizeof(unsigned int) * numPlayers);

            cardPossessionFound = (bool *)malloc(sizeof(bool) * myBoardModel->getNumComponents());

            accusations = new vector<vector<unsigned int> >[numPlayers];

            for(unsigned int i = 0; i < numPlayers + 1; i++)
            {
            	for(unsigned int j = 0; j < myBoardModel->getNumComponents(); j++)
            	{
            		possessionMatrix[i][j] = UNKNOWN;
            	}
            }

            memset(numCardsDiscovered, 0, sizeof(unsigned int) * numPlayers);

            memset(cardPossessionFound, 0, sizeof(bool) * myBoardModel->getNumComponents());

            for(unsigned int i = 0; i < myBoardModel->getNumComponents(); i++)
            {
                possessionMatrix[numPlayers][i] = NOT_POSSESSED_BY;
            }

            for(unsigned int i = 0; i < inGlobalCards.size(); i++)
            {
                possessionMatrix[numPlayers][inGlobalCards.at(i)] = POSSESSED_BY;

                cardPossessionFound[inGlobalCards.at(i)] = true;

                for(unsigned int j = 0; j < numPlayers; j++)
                {
                    possessionMatrix[j][inGlobalCards.at(i)] = NOT_POSSESSED_BY;
                }
            }
        }
    }
    else
    {
#if PRINT_DEBUG
        cout << "Not enough players..." << endl;
#endif
    }
}

GameModel::~GameModel()
{
    delete [] accusations;
    free(cardPossessionFound);
    free(numCardsDiscovered);

    for(unsigned int i = 0; i < numPlayers + 1; i++)
    {
        delete [] possessionMatrix[i];
    }

    delete [] possessionMatrix;

    while(undoHistory.size() > 0)
    {
        deleteState(0);
    }
}

bool GameModel::addPossession(unsigned int player, unsigned int card, bool calledOutside)
{
    if(possessionMatrix[player - 1][card] != NOT_POSSESSED_BY &&
       possessionMatrix[player - 1][card] != POSSESSED_BY &&
       !cardPossessionFound[card])
    {
        if(calledOutside)
        {
            saveState();
        }

        removeFromAllPossession(card);

        possessionMatrix[player - 1][card] = POSSESSED_BY;
        cardPossessionFound[card] = true;
        numCardsDiscovered[player - 1]++;

        if(numCardsDiscovered[player - 1] == numCardsPerPlayer[player - 1])
        {
            for(unsigned int i = 0; i < myBoardModel->getNumComponents(); i++)
            {
                if(possessionMatrix[player - 1][i] != POSSESSED_BY)
                {
                    possessionMatrix[player - 1][i] = NOT_POSSESSED_BY;
                }
            }
        }

        removeFromAccusations(card, -1, player - 1);

        processAccusations();

        if(calledOutside && !checkConstraints())
        {
        	stepBack();
        	return false;
        }

        return true;
    }

    return false;
}

bool GameModel::removePossession(unsigned int player, unsigned int card)
{
    if(possessionMatrix[player - 1][card] != NOT_POSSESSED_BY &&
       possessionMatrix[player - 1][card] != POSSESSED_BY)
    {
        saveState();
        possessionMatrix[player - 1][card] = NOT_POSSESSED_BY;
        removeFromAccusations(card, player - 1);
        processAccusations();

        if(!checkConstraints())
        {
        	stepBack();
        	return false;
        }

        return true;
    }

    return false;
}

void GameModel::removeFromAllPossession(unsigned int card)
{
    for(unsigned int i = 0; i < numPlayers; i++)
    {
        possessionMatrix[i][card] = NOT_POSSESSED_BY;
    }
}

void GameModel::removeFromAccusations(unsigned int card, int whichPlayer, int triggeringPlayer)
{
    unsigned int end = whichPlayer == -1 ? numPlayers : whichPlayer + 1;

    for(unsigned int i = whichPlayer == -1 ? 0 : whichPlayer; i < end; i++)
    {
        for(unsigned int j = 0; j < accusations[i].size(); j++)
        {
            for(unsigned int k = 0; k < accusations[i].at(j).size(); k++)
            {
                if(accusations[i].at(j).at(k) == card)
                {
                    if(triggeringPlayer != -1 && triggeringPlayer == (int)i)
                    {
                        accusations[i].erase(accusations[i].begin() + j, accusations[i].begin() + 1 + j);
                        j--;
                        break;
                    }
                    else
                    {
                        accusations[i].at(j).erase(accusations[i].at(j).begin() + k, accusations[i].at(j).begin() + 1 + k);
                    }
                }
            }
        }
    }
}

void GameModel::processAccusations()
{
    for(unsigned int i = 0; i < numPlayers; i++)
    {
        for(unsigned int j = 0; j < accusations[i].size(); j++)
        {
            if(accusations[i].at(j).size() == 1)
            {
                unsigned int card = accusations[i].at(j).at(0);
                accusations[i].erase(accusations[i].begin() + j, accusations[i].begin() + 1 + j);
                addPossession(i + 1, card);
            }
        }
    }
    evaluateGuesses();
}

void GameModel::getBestGuess(unsigned int & suspectGuest, unsigned int & suspectWeapon, unsigned int & suspectRoom)
{
    suspectGuest = guest;
    suspectRoom = room;
    suspectWeapon = weapon;
}

void GameModel::evaluateGuesses()
{
    evaluateLastOneLeft();
    evaluateWholeRow();
    checkOneLeftOut();
    checkEqualRemainingCards();
}

void GameModel::evaluateWholeRow()
{
    if(guest == -1)
    {
        unsigned int sum;

        for(unsigned int i = myBoardModel->getFirstGuest(); i <= myBoardModel->getLastGuest(); i++)
        {
            sum = 0;

            for(unsigned int j = 0; j < numPlayers + 1; j++)
            {
                if(possessionMatrix[j][i] == NOT_POSSESSED_BY)
                {
                    sum++;
                }
            }

            if(sum == numPlayers + 1)
            {
                guest = i;
                break;
            }
        }
    }

    if(weapon == -1)
    {
        unsigned int sum;

        for(unsigned int i = myBoardModel->getFirstWeapon(); i <= myBoardModel->getLastWeapon(); i++)
        {
            sum = 0;

            for(unsigned int j = 0; j < numPlayers + 1; j++)
            {
                if(possessionMatrix[j][i] == NOT_POSSESSED_BY)
                {
                    sum++;
                }
            }

            if(sum == numPlayers + 1)
            {
                weapon = i;
                break;
            }
        }
    }

    if(room == -1)
    {
        unsigned int sum;

        for(unsigned int i = myBoardModel->getFirstRoom(); i <= myBoardModel->getLastRoom(); i++)
        {
            sum = 0;

            for(unsigned int j = 0; j < numPlayers + 1; j++)
            {
                if(possessionMatrix[j][i] == NOT_POSSESSED_BY)
                {
                    sum++;
                }
            }

            if(sum == numPlayers + 1)
            {
                room = i;
                break;
            }
        }
    }
}

void GameModel::evaluateLastOneLeft()
{
    unsigned int guestSum = 0, weaponSum = 0, roomSum = 0;

    int guestIndex = -1, weaponIndex = -1, roomIndex = -1;

    if(guest == -1)
    {
        for(unsigned int i = myBoardModel->getFirstGuest(); i <= myBoardModel->getLastGuest(); i++)
        {
            if(cardPossessionFound[i])
            {
                guestSum++;
            }
            else
            {
                guestIndex = i;
            }
        }
    }

    if(weapon == -1)
    {
        for(unsigned int i = myBoardModel->getFirstWeapon(); i <= myBoardModel->getLastWeapon(); i++)
        {
            if(cardPossessionFound[i])
            {
                weaponSum++;
            }
            else
            {
                weaponIndex = i;
            }
        }
    }

    if(room == -1)
    {
        for(unsigned int i = myBoardModel->getFirstRoom(); i <= myBoardModel->getLastRoom(); i++)
        {
            if(cardPossessionFound[i])
            {
                roomSum++;
            }
            else
            {
                roomIndex = i;
            }
        }
    }

    if(guestSum == 5 && guest == -1 && guestIndex != -1)
    {
        guest = guestIndex;
        removeFromAllPossession(guestIndex);
        removeFromAccusations(guestIndex);
        processAccusations();
    }

    if(weaponSum == 8 && weapon == -1 && weaponIndex != -1)
    {
        weapon = weaponIndex;
        removeFromAllPossession(weaponIndex);
        removeFromAccusations(weaponIndex);
        processAccusations();
    }

    if(roomSum == 8 && room == -1 && roomIndex != -1)
    {
        room = roomIndex;
        removeFromAllPossession(roomIndex);
        removeFromAccusations(roomIndex);
        processAccusations();
    }
}

void GameModel::checkOneLeftOut()
{
	if(guest != -1)
	{
		unsigned int sum;

		for(unsigned int i = myBoardModel->getFirstGuest(); i <= myBoardModel->getLastGuest(); i++)
		{
			int lastPlayer = -1;
			sum = 0;

			for(unsigned int j = 0; j < numPlayers; j++)
			{
				if(possessionMatrix[j][i] == NOT_POSSESSED_BY)
				{
					sum++;
				}
				else if(possessionMatrix[j][i] == UNKNOWN)
				{
					lastPlayer = (int)j;
				}
			}

			if(sum == numPlayers - 1 && lastPlayer != -1)
			{
				addPossession(lastPlayer + 1, i);
			}
		}
	}

	if(weapon != -1)
	{
		unsigned int sum;

		for(unsigned int i = myBoardModel->getFirstWeapon(); i <= myBoardModel->getLastWeapon(); i++)
		{
			int lastPlayer = -1;
			sum = 0;

			for(unsigned int j = 0; j < numPlayers; j++)
			{
				if(possessionMatrix[j][i] == NOT_POSSESSED_BY)
				{
					sum++;
				}
				else if(possessionMatrix[j][i] == UNKNOWN)
				{
					lastPlayer = (int) j;
				}
			}

			if(sum == numPlayers - 1 && lastPlayer != -1)
			{
				addPossession(lastPlayer + 1,  i);
			}
		}
	}

	if(room != -1)
	{
		unsigned int sum;

		for(unsigned int i = myBoardModel->getFirstRoom(); i <= myBoardModel->getLastRoom(); i++)
		{
			int lastPlayer = -1;
			sum = 0;

			for(unsigned int j = 0; j < numPlayers; j++)
			{
				if(possessionMatrix[j][i] == NOT_POSSESSED_BY)
				{
					sum++;
				}
				else if(possessionMatrix[j][i] == UNKNOWN)
				{
					lastPlayer = (int) j;
				}
			}

			if(sum == numPlayers - 1 && lastPlayer != -1)
			{
				addPossession(lastPlayer + 1,  i);
			}
		}
	}
}

void GameModel::checkEqualRemainingCards()
{
	for(unsigned int i = 0; i < numPlayers; i++)
	{
		vector<unsigned int> unknownCards;

		for(unsigned int j = 0; j < myBoardModel->getNumComponents(); j++)
		{
			if(possessionMatrix[i][j] == UNKNOWN)
			{
				unknownCards.push_back(j);
			}
		}

		if(unknownCards.size() == numCardsPerPlayer[i] - numCardsDiscovered[i])
		{
			for(unsigned int k = 0; k < unknownCards.size(); k++)
			{
				addPossession(i + 1, unknownCards[k]);
			}
		}
	}
}

bool GameModel::checkConstraints()
{
	return checkRemainingCards() && checkNumberNotPossessed();
}

bool GameModel::checkRemainingCards()
{
	for(unsigned int i = 0; i < numPlayers; i++)
	{
		unsigned int numUnknown = 0;

		for(unsigned int j = 0; j < myBoardModel->getNumComponents(); j++)
		{
			if(possessionMatrix[i][j] == UNKNOWN)
			{
				numUnknown++;
			}
		}

		if(numUnknown < numCardsPerPlayer[i] - numCardsDiscovered[i])
		{
			return false;
		}
	}

	return true;
}

bool GameModel::checkNumberNotPossessed()
{
	unsigned int sum;
	unsigned int count = 0;

	for(unsigned int i = myBoardModel->getFirstGuest(); i <= myBoardModel->getLastGuest(); i++)
	{
		sum = 0;

		for(unsigned int j = 0; j < numPlayers + 1; j++)
		{
			if(possessionMatrix[j][i] == NOT_POSSESSED_BY)
			{
				sum++;
			}
		}

		if(sum == numPlayers + 1)
		{
			count++;

			if(count > 1)
			{
				return false;
			}
		}
	}

	count = 0;

	for(unsigned int i = myBoardModel->getFirstWeapon(); i <= myBoardModel->getLastWeapon(); i++)
	{
		sum = 0;

		for(unsigned int j = 0; j < numPlayers + 1; j++)
		{
			if(possessionMatrix[j][i] == NOT_POSSESSED_BY)
			{
				sum++;
			}
		}

		if(sum == numPlayers + 1)
		{
			count++;

			if(count > 1)
			{
				return false;
			}
		}
	}

	count = 0;

	for(unsigned int i = myBoardModel->getFirstRoom(); i <= myBoardModel->getLastRoom(); i++)
	{
		sum = 0;

		for(unsigned int j = 0; j < numPlayers + 1; j++)
		{
			if(possessionMatrix[j][i] == NOT_POSSESSED_BY)
			{
				sum++;
			}
		}

		if(sum == numPlayers + 1)
		{
			count++;

			if(count > 1)
			{
				return false;
			}
		}
	}

	return true;
}

int GameModel::processSuspicion(unsigned int startPlayer, unsigned int endPlayer, unsigned int suspectGuest, unsigned int suspectWeapon, unsigned int suspectRoom)
{
#if PRINT_DEBUG
    cout << "processSuspicion: " << startPlayer << ", " << endPlayer << ", " << myBoardModel->getComponentNames()[suspectGuest] << ", " << myBoardModel->getComponentNames()[suspectWeapon] << ", " << myBoardModel->getComponentNames()[suspectRoom] << endl;
#endif

    saveState();

    unsigned int currentIndex = startPlayer % numPlayers;

    vector<unsigned int> accusationSet;

    if(possessionMatrix[numPlayers][suspectGuest] != POSSESSED_BY)
    {
        accusationSet.push_back(suspectGuest);
    }
    if(possessionMatrix[numPlayers][suspectWeapon] != POSSESSED_BY)
    {
        accusationSet.push_back(suspectWeapon);
    }
    if(possessionMatrix[numPlayers][suspectRoom] != POSSESSED_BY)
    {
        accusationSet.push_back(suspectRoom);
    }

    for(unsigned int i = 0; i < accusationSet.size(); i++)
    {
        if(possessionMatrix[startPlayer - 1][accusationSet[i]] == POSSESSED_BY)
        {
            accusationSet.erase(accusationSet.begin() + i, accusationSet.begin() + 1 + i);
            i--;
        }
    }

    unsigned int possibleShownNumber = accusationSet.size();

    for(unsigned int i = 0; i < accusationSet.size(); i++)
    {
        if(startPlayer != endPlayer && possessionMatrix[endPlayer - 1][accusationSet[i]] == NOT_POSSESSED_BY)
        {
            possibleShownNumber--;
        }
    }

    if(possibleShownNumber == 0)
    {
#if PRINT_DEBUG
    	cout << "Invalid end player" << endl;
#endif
    	stepBack();
    	return -1;
    }

    while(currentIndex != endPlayer - 1)
    {
        for(unsigned int i = 0; i < accusationSet.size(); i++)
        {
            if(possessionMatrix[currentIndex][accusationSet[i]] != POSSESSED_BY)
            {
                possessionMatrix[currentIndex][accusationSet[i]] = NOT_POSSESSED_BY;
            }
            else
            {
#if PRINT_DEBUG
                cout << "Possession error on suspicion: " << currentIndex + 1 << ", " << myBoardModel->getComponentNames()[accusationSet[i]] << endl;
#endif
            }
            removeFromAccusations(accusationSet[i], currentIndex);
        }

        currentIndex++;
        currentIndex %= numPlayers;
    }

    for(unsigned int i = 0; i < accusationSet.size(); i++)
    {
        if(startPlayer != endPlayer && possessionMatrix[endPlayer - 1][accusationSet[i]] == NOT_POSSESSED_BY)
        {
            accusationSet.erase(accusationSet.begin() + i, accusationSet.begin() + 1 + i);
            i--;
        }
    }

#if PRINT_DEBUG
    cout << "Accusation set size: " << accusationSet.size() << endl;
#endif

    if(accusationSet.size() == 0 && startPlayer == endPlayer)
    {
#if PRINT_DEBUG
        cout << "They suspected cards they already know..." << endl;
#endif
    }
    else if(accusationSet.size() > 0 && startPlayer == endPlayer)
    {
#if PRINT_DEBUG
        cout << "Good call" << endl;
#endif
        for(unsigned int i = 0; i < accusationSet.size(); i++)
        {
            if(numCardsDiscovered[startPlayer - 1] == numCardsPerPlayer[startPlayer - 1] ||
               possessionMatrix[startPlayer - 1][accusationSet[i]] == NOT_POSSESSED_BY)
            {
                removeFromAllPossession(accusationSet[i]);
                removeFromAccusations(accusationSet[i]);

                if(accusationSet[i] >= myBoardModel->getFirstGuest() && accusationSet[i] <= myBoardModel->getLastGuest())
                {
                    guest = accusationSet[i];
                }
                else if(accusationSet[i] >= myBoardModel->getFirstWeapon() && accusationSet[i] <= myBoardModel->getLastWeapon())
                {
                    weapon = accusationSet[i];
                }
                else if(accusationSet[i] >= myBoardModel->getFirstRoom() && accusationSet[i] <= myBoardModel->getLastRoom())
                {
                    room = accusationSet[i];
                }
            }
        }
    }
    else if(accusationSet.size() == 1 && startPlayer != endPlayer)
    {
#if PRINT_DEBUG
        cout << "Single possession" << endl;
#endif
        if(possessionMatrix[endPlayer - 1][accusationSet[0]] != POSSESSED_BY)
        {
            addPossession(endPlayer, accusationSet[0]);
        }
    }
    else if(accusationSet.size() > 1 && startPlayer != endPlayer)
    {
#if PRINT_DEBUG
        cout << "Potential new theory" << endl;
#endif
        bool createNewTheory = true;

        for(unsigned int i = 0; i < accusationSet.size(); i++)
        {
            if(possessionMatrix[endPlayer - 1][accusationSet[i]] == POSSESSED_BY)
            {
                createNewTheory = false;
                break;
            }
        }

        if(createNewTheory)
        {
#if PRINT_DEBUG
            cout << "New theory created" << endl;
#endif
            accusations[endPlayer - 1].push_back(accusationSet);
        }
    }
    else
    {
#if PRINT_DEBUG
        cout << "Weird: Zero accusation set size..." << endl;
#endif
    }
#if PRINT_DEBUG
    printMatrix();
#endif
    processAccusations();

    if(!checkConstraints())
    {
    	stepBack();
    }

    return 0;
}

unsigned int * GameModel::getCardsDiscovered()
{
    return numCardsDiscovered;
}

unsigned int ** GameModel::getMatrix()
{
    return possessionMatrix;
}

vector<vector<unsigned int> > * GameModel::getTheories()
{
    return accusations;
}

void GameModel::saveState()
{
    state newSave;

    newSave.accusations = new vector<vector<unsigned int> >[numPlayers];

    newSave.possessionMatrix = new unsigned int * [numPlayers + 1];

    for(unsigned int i = 0; i < numPlayers + 1; i++)
    {
    	newSave.possessionMatrix[i] = new unsigned int[myBoardModel->getNumComponents()];
    }

    newSave.numCardsDiscovered = (unsigned int *)malloc(sizeof(unsigned int) * numPlayers);
    newSave.cardAccountedFor = (bool *)malloc(sizeof(bool) * myBoardModel->getNumComponents());

    for(unsigned int i = 0; i < numPlayers; i++)
    {
        newSave.accusations[i] = accusations[i];
    }

    memcpy(newSave.cardAccountedFor, cardPossessionFound, sizeof(bool) * myBoardModel->getNumComponents());
    memcpy(newSave.numCardsDiscovered, numCardsDiscovered, sizeof(unsigned int) * numPlayers);

    for(unsigned int i = 0; i < numPlayers + 1; i++)
    {
    	for(unsigned int j = 0; j < myBoardModel->getNumComponents(); j++)
    	{
    		newSave.possessionMatrix[i][j] = possessionMatrix[i][j];
    	}
    }

    newSave.guest = guest;
    newSave.weapon = weapon;
    newSave.room = room;

    unsigned int vectorSize = 0;

    for(unsigned int i = 0; i < numPlayers; i++)
    {
        vectorSize += sizeof(accusations[i]);

        for(unsigned int j = 0; j < accusations[i].size(); j++)
        {
            vectorSize += sizeof(accusations[i].at(j));
            vectorSize += sizeof(unsigned int) * accusations[i].at(j).size();
        }
    }

    newSave.size = sizeof(unsigned int) * (numPlayers + 1) * myBoardModel->getNumComponents() +
                   sizeof(unsigned int) * numPlayers +
                   sizeof(bool) * myBoardModel->getNumComponents() +
                   sizeof(state) +
                   vectorSize;

    undoHistorySize += newSave.size;

    undoHistory.push_back(newSave);
}

void GameModel::stepBack()
{
    if(undoHistory.size() > 0)
    {
        for(unsigned int i = 0; i < numPlayers; i++)
        {
            accusations[i] = undoHistory.at(undoHistory.size() - 1).accusations[i];
        }

        memcpy(cardPossessionFound, undoHistory.at(undoHistory.size() - 1).cardAccountedFor, sizeof(bool) * myBoardModel->getNumComponents());
        memcpy(numCardsDiscovered, undoHistory.at(undoHistory.size() - 1).numCardsDiscovered, sizeof(unsigned int) * numPlayers);

        for(unsigned int i = 0; i < numPlayers + 1; i++)
        {
        	for(unsigned int j = 0; j < myBoardModel->getNumComponents(); j++)
        	{
        		possessionMatrix[i][j] = undoHistory.at(undoHistory.size() - 1).possessionMatrix[i][j];
        	}
        }

        guest = undoHistory.at(undoHistory.size() - 1).guest;
        weapon = undoHistory.at(undoHistory.size() - 1).weapon;
        room = undoHistory.at(undoHistory.size() - 1).room;

        deleteState(undoHistory.size() - 1);
    }
}

void GameModel::deleteState(const unsigned int state)
{
    if(state < undoHistory.size())
    {
        delete [] undoHistory.at(state).accusations;
        free(undoHistory.at(state).cardAccountedFor);
        free(undoHistory.at(state).numCardsDiscovered);

        for(unsigned int i = 0; i < numPlayers + 1; i++)
        {
            delete [] undoHistory.at(state).possessionMatrix[i];
        }

        delete [] undoHistory.at(state).possessionMatrix;

        undoHistorySize -= undoHistory.at(state).size;

        undoHistory.erase(undoHistory.begin() + state, undoHistory.begin() + 1 + state);
    }
}

void GameModel::printMatrix()
{

    cout << setw(11) << " " << " | ";

    for(unsigned int i = 0; i < numPlayers; i++)
    {
        cout << (i + 1) << " | ";
    }

    cout << "G | " << endl;

    printLine();

    for(unsigned int i = 0; i < myBoardModel->getNumComponents(); i++)
    {
        if(i == myBoardModel->getFirstWeapon() || i == myBoardModel->getFirstRoom())
        {
            printLine();
        }

        cout << setw(11) << myBoardModel->getComponentNames()[i] << " | ";

        for(unsigned int j = 0; j < numPlayers + 1; j++)
        {
            cout << statusSymbols[possessionMatrix[j][i]] << " | ";
        }

        cout << endl;
    }

    printLine();

    cout << "Discovered: " << (guest != -1 ? myBoardModel->getComponentNames()[guest] : "") << " "
                           << (weapon != -1 ? myBoardModel->getComponentNames()[weapon] : "") << " "
                           << (room != -1 ? myBoardModel->getComponentNames()[room] : "") << endl;
}

void GameModel::printLine()
{
    for(unsigned int i = 0; i < 13 + (numPlayers + 1) * 4; i++)
    {
        cout << "-";
    }

    cout << endl;
}
