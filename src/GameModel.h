/*
 * GameModel.h
 *
 *  Created on: 9 Jan 2011
 *      Author: thomas
 *
 */

#ifndef GAMEMODEL_H_
#define GAMEMODEL_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <iomanip>
#include <vector>

#include "GameComponents.h"
#include "BoardModel.h"

using namespace std;

#define PRINT_DEBUG 0

class GameModel
{
    public:
        GameModel(BoardModel * inBoard, const unsigned int inNumPlayers, const vector<unsigned int> inNumCardsPerPlayer, const vector<unsigned int> & inGlobalCards);
        virtual ~GameModel();

        unsigned int * getCardsDiscovered();
        unsigned int ** getMatrix();
        vector<vector<unsigned int> > * getTheories();
        void getBestGuess(unsigned int & suspectGuest, unsigned int & suspectWeapon, unsigned int & suspectRoom);

        void printMatrix();

        void stepBack();

        bool addPossession(unsigned int player, unsigned int card, bool calledOutside = false);
        bool removePossession(unsigned int player, unsigned int card);
        int processSuspicion(unsigned int startPlayer, unsigned int endPlayer, unsigned int suspectGuest, unsigned int suspectWeapon, unsigned int suspectRoom);

        bool isGuest(unsigned int inComp)
        {
            if(inComp >= myBoardModel->getFirstGuest() && inComp <= myBoardModel->getLastGuest())
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        bool isWeapon(unsigned int inComp)
        {
            if(inComp >= myBoardModel->getFirstWeapon() && inComp <= myBoardModel->getLastWeapon())
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        bool isRoom(unsigned int inComp)
        {
            if(inComp >= myBoardModel->getFirstRoom() && inComp <= myBoardModel->getLastRoom())
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        unsigned int getUndoHistorySize()
        {
            return undoHistorySize;
        }

    private:
        BoardModel * myBoardModel;
        const unsigned int numPlayers;
        const vector<unsigned int> numCardsPerPlayer;
        int numGlobalCards;

        vector<vector<unsigned int> > * accusations;
        unsigned int * numCardsDiscovered;
        bool * cardPossessionFound;
        unsigned int ** possessionMatrix;
        int guest, weapon, room;

        struct state
        {
            vector<vector<unsigned int> > * accusations;
            unsigned int * numCardsDiscovered;
            bool * cardAccountedFor;
            unsigned int ** possessionMatrix;
            int guest, weapon, room;
            unsigned int size;
        };

        unsigned int undoHistorySize;

        vector<state> undoHistory;

        void saveState();
        void deleteState(const unsigned int state);

        void printLine();
        void processAccusations();

        void evaluateGuesses();
        void evaluateLastOneLeft();
        void evaluateWholeRow();
        void checkOneLeftOut();
        void checkEqualRemainingCards();

        bool checkConstraints();
        bool checkRemainingCards();
        bool checkNumberNotPossessed();

        void removeFromAccusations(unsigned int card, int whichPlayer = -1, int triggeringPlayer = -1);
        void removeFromAllPossession(unsigned int card);
};

#endif /* GAMEMODEL_H_ */
