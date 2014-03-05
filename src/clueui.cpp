#include "clueui.h"

ClueUI::ClueUI(QWidget * parent) : QMainWindow(parent)
{
	ui.setupUi(this);
	myGameModel = 0;
	currentlyInGame = false;
	currentNumPlayers = 0;
	playerNames = 0;

	const unsigned int numComponents = 24;
	unsigned int guestEnd = 5;
	unsigned int weaponEnd = 14;

	string componentNames[numComponents] =
	{
	    "Mustard", "Plum", "Green", "Peacock",
	    "Scarlet", "White", "Knife", "Candlestick",
	    "Pistol", "Poison", "Trophy", "Rope",
	    "Bat", "Axe", "Dumbbell", "Hall",
	    "Dining Room", "Kitchen", "Patio", "Observatory",
	    "Theater", "Living Room", "Spa", "Guest House"
	};

	myBoardModel = new BoardModel(&componentNames[0], guestEnd, weaponEnd, numComponents);

	numRows = myBoardModel->getNumComponents();

	setupGui();
}

ClueUI::~ClueUI()
{
    deconstruct();
}

void ClueUI::deconstruct()
{
    delete [] componentTableItems;

    if(possessionMatrixItems[0] != 0)
    {
        for(unsigned int i = 0; i < numRows; i++)
        {
            delete [] possessionMatrixItems[i];
        }
    }

    delete [] possessionMatrixItems;

    if(myGameModel != 0)
    {
        delete myGameModel;
    }

    if(playerNames != 0)
    {
        delete [] playerNames;
    }

    for(unsigned int i = 0; i < theoryItems.size(); i++)
    {
        delete theoryItems[i];
    }

    theoryItems.clear();

    delete myBoardModel;
}

void ClueUI::setupGui()
{
    connect(ui.actionQuit, SIGNAL(triggered()), this, SLOT(quit()));
    connect(ui.actionNew_Game, SIGNAL(triggered()), this, SLOT(newGame()));
    connect(ui.possessionTable->horizontalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(enterName(int)));
    connect(ui.possessionTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(addKnownCard(int, int)));
    connect(ui.addTheoryButton, SIGNAL(clicked()), this, SLOT(addTheory()));
    connect(ui.undoButton, SIGNAL(clicked()), this, SLOT(undoLast()));

    ui.possessionTable->setSortingEnabled(false);
    ui.possessionTable->setRowCount(numRows);
    ui.possessionTable->setColumnCount(1);
    ui.possessionTable->setColumnWidth(0, 80);
    ui.possessionTable->horizontalHeader()->setClickable(true);
    ui.possessionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui.possessionTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.possessionTable, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(crossOffCard(const QPoint &)));

    ui.theoryTable->setSortingEnabled(false);
    ui.theoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QStringList rowNumbers;
    for(unsigned int i = 1; i < numRows + 1; i++)
    {
        rowNumbers << QString::number(i);
    }

    ui.possessionTable->setVerticalHeaderLabels(rowNumbers);

    QStringList column;
    column << "Card";
    ui.possessionTable->setHorizontalHeaderLabels(column);

    componentTableItems = new QTableWidgetItem[numRows];

    for(unsigned int i = 0; i < numRows; i++)
    {
        componentTableItems[i].setText(QString::fromStdString(myBoardModel->getComponentNames()[i]));
        ui.possessionTable->setItem(i, 0, &componentTableItems[i]);
    }

    possessionMatrixItems = new QTableWidgetItem * [numRows];
    possessionMatrixItems[0] = 0;
}

void ClueUI::closeEvent(QCloseEvent *)
{
    quit();
}

void ClueUI::resizeEvent(QResizeEvent *)
{
    optimiseTableSizes();
}

void ClueUI::optimiseTableSizes()
{
    if((ui.possessionTable->height() - 23) / ui.possessionTable->rowCount() >= 18)
    {
        for(int i = 0; i < ui.possessionTable->rowCount(); i++)
        {
            ui.possessionTable->setRowHeight(i, (ui.possessionTable->height() - 23) / ui.possessionTable->rowCount());
        }
    }

    if(ui.possessionTable->width() - 80 / ui.possessionTable->columnCount() >= 22)
    {
        for(int i = 1; i < ui.possessionTable->columnCount(); i++)
        {
            ui.possessionTable->setColumnWidth(i, (ui.possessionTable->width() - 80) / ui.possessionTable->columnCount());
        }
    }

    if(currentlyInGame && ui.theoryTable->width() / ui.theoryTable->columnCount() >= 22)
    {
        for(int i = 0; i < ui.theoryTable->columnCount(); i++)
        {
            ui.theoryTable->setColumnWidth(i, (ui.theoryTable->width() / ui.theoryTable->columnCount()) - 3);
        }
    }
}

void ClueUI::newGame()
{
    if(!currentlyInGame)
    {
        bool ok = true;

        currentNumPlayers = QInputDialog::getInt(this, "How many players?", "Number of players:", 2, 2, 6, 1, &ok);

        if(ok)
        {
            QString cardsPerPlayer, globalCardsString;

            cardsPerPlayer = QInputDialog::getText(this, "Number of cards per player", "Cards per player (Separated by a space)", QLineEdit::Normal, "", &ok);

            if(ok)
            {
                unsigned int numCardTokens = 0;
                numCardsPerPlayer = parseNumericInput(cardsPerPlayer, &numCardTokens);

                if(numCardTokens != currentNumPlayers)
                {
                    QMessageBox::warning(this, "Notice", "You didn't specify enough cards per player...");
                    newGame();
                }
                else
                {
                    unsigned int totalPlayerCards = 0;
                    for(unsigned int i = 0; i < numCardsPerPlayer.size(); i++)
                    {
                        totalPlayerCards += numCardsPerPlayer[i];
                    }

                    if(totalPlayerCards < myBoardModel->getNumComponents() - 3)
                    {
                        stringstream str;
                        str << "ID numbers for global cards (Separated by a space) [" << myBoardModel->getNumComponents() - 3 - totalPlayerCards << "]:";
                        globalCardsString = QInputDialog::getText(this, "Global cards", str.str().c_str(), QLineEdit::Normal, "", &ok);
                    }

                    if(ok && totalPlayerCards <= myBoardModel->getNumComponents() - 3)
                    {
                        unsigned int numGlobalCards = 0;
                        vector<unsigned int> myGlobalCards;
                        bool keepAsking = true;
                        bool inputOk = true;

                        if(totalPlayerCards < myBoardModel->getNumComponents() - 3)
                        {
                            myGlobalCards = parseNumericInput(globalCardsString, &numGlobalCards);

                            for(unsigned int i = 0; i < myGlobalCards.size(); i++)
                            {
                                if(myGlobalCards[i] > myBoardModel->getNumComponents() || myGlobalCards[i] < 1)
                                {
                                    inputOk = false;
                                    break;
                                }
                            }

                            while((myGlobalCards.size() != myBoardModel->getNumComponents() - 3 - totalPlayerCards && keepAsking) || !inputOk)
                            {
                                QMessageBox::warning(this, "Notice", "Wrong number(s) of global cards...");
                                stringstream str;
                                str << "ID numbers for global cards (Separated by a space) [" << myBoardModel->getNumComponents() - 3 - totalPlayerCards << "]:";
                                globalCardsString = QInputDialog::getText(this, "Global cards", str.str().c_str(), QLineEdit::Normal, "", &keepAsking);
                                myGlobalCards.clear();
                                myGlobalCards = parseNumericInput(globalCardsString, &numGlobalCards);
                                inputOk = true;

                                for(unsigned int i = 0; i < myGlobalCards.size(); i++)
                                {
                                    if(myGlobalCards[i] > myBoardModel->getNumComponents() || myGlobalCards[i] < 1)
                                    {
                                        inputOk = false;
                                        break;
                                    }
                                }
                            }
                        }

                        if(keepAsking)
                        {
                            ui.possessionTable->setColumnCount(currentNumPlayers + 2);
                            ui.theoryTable->setColumnCount(currentNumPlayers);

                            QStringList theoryTitles;
                            QStringList columnTitles;
                            columnTitles << "Card";
                            QString fraction = " [0/";

                            playerNames = new QString[currentNumPlayers];

                            for(unsigned int i = 1; i < currentNumPlayers + 1; i++)
                            {
                                playerNames[i - 1] = QString::number(i);
                                theoryTitles << playerNames[i - 1];
                                QString nextTitle = playerNames[i - 1];
                                nextTitle.append(fraction).append(QString::number(numCardsPerPlayer[i - 1])).append(']');
                                columnTitles << nextTitle;
                            }
                            columnTitles << "G";
                            ui.possessionTable->setHorizontalHeaderLabels(columnTitles);
                            ui.theoryTable->setHorizontalHeaderLabels(theoryTitles);

                            for(unsigned int i = 0; i < numRows; i++)
                            {
                                possessionMatrixItems[i] = new QTableWidgetItem[currentNumPlayers + 1];
                            }

                            vector<unsigned int> globalCards;

                            for(unsigned int i = 0; i < numGlobalCards; i++)
                            {
                                globalCards.push_back(myGlobalCards[i] - 1);
                            }

                            myGameModel = new GameModel(myBoardModel, currentNumPlayers, numCardsPerPlayer, globalCards);

                            currentlyInGame = true;

                            optimiseTableSizes();

                            displayMatrix(true);

                            ui.addTheoryButton->setEnabled(true);
                            ui.undoButton->setEnabled(true);
                        }
                    }
                    if(totalPlayerCards > myBoardModel->getNumComponents() - 3)
                    {
                        QMessageBox::warning(this, "Notice", "Too many cards per player...");
                        newGame();
                    }
                }
            }
        }
    }
    else
    {
        if(!QMessageBox::question(this, "Continue?", "You are currently in a game, end it?", "&Yes", "&No", QString::null, 0, 1 ))
        {
            currentlyInGame = false;
            delete myGameModel;
            myGameModel = 0;
            ui.addTheoryButton->setEnabled(false);
            ui.undoButton->setEnabled(false);

            for(unsigned int i = 0; i < numRows; i++)
            {
                delete [] possessionMatrixItems[i];
            }

            delete [] playerNames;
            playerNames = 0;

            possessionMatrixItems[0] = 0;

            ui.possessionTable->setColumnCount(1);

            for(unsigned int i = 0; i < theoryItems.size(); i++)
            {
                delete theoryItems[i];
            }

            theoryItems.clear();

            ui.theoryTable->setColumnCount(0);
            ui.theoryTable->setRowCount(0);

            newGame();
        }
    }
}

void ClueUI::enterName(int section)
{
    if(currentlyInGame && section > 0 && section < (int)currentNumPlayers + 1)
    {
        bool ok = true;

        QString newPlayerName = QInputDialog::getText(this, "Rename player", "Players name:", QLineEdit::Normal, "", &ok);

        playerNames[section - 1] = newPlayerName;

        if(ok)
        {
            ui.theoryTable->horizontalHeaderItem(section - 1)->setText(newPlayerName);

            QString fraction = " [";
            fraction.append(QString::number(myGameModel->getCardsDiscovered()[section - 1]));
            fraction.append('/');
            newPlayerName.append(fraction).append(QString::number(numCardsPerPlayer[section - 1])).append(']');
            ui.possessionTable->horizontalHeaderItem(section)->setText(newPlayerName);
        }
    }
}

void ClueUI::addKnownCard(int row, int column)
{
    if(currentlyInGame && column > 0 && column <= (int)currentNumPlayers)
    {
        myGameModel->addPossession(column, row, true);
        displayMatrix();
    }
}

void ClueUI::crossOffCard(const QPoint & point)
{
    if(currentlyInGame)
    {
        unsigned int column = ui.possessionTable->columnAt(point.x());
        unsigned int row = ui.possessionTable->rowAt(point.y());
        myGameModel->removePossession(column, row);
        displayMatrix();
    }
}

void ClueUI::addTheory()
{
    if(currentlyInGame)
    {
        QList<QTableWidgetItem *> selectedItems = ui.possessionTable->selectedItems();

        if(selectedItems.size() != 3)
        {
            QMessageBox::warning(this, "Notice", "You didn't select 3 cards");
        }
        else
        {
            if(selectedItems.at(0)->column() == selectedItems.at(1)->column() &&
               selectedItems.at(0)->column() == selectedItems.at(2)->column() &&
               selectedItems.at(0)->column() > 0 && selectedItems.at(0)->column() < (int)currentNumPlayers + 1)
            {
                unsigned int cardIndices[3] = {0, 0, 0};
                unsigned int checkAmount = 0;

                //Guest check
                for(unsigned int i = 0; i < 3; i++)
                {
                    if(myGameModel->isGuest(selectedItems.at(i)->row()))
                    {
                        checkAmount++;
                        cardIndices[0] = i;
                        break;
                    }
                }

                //Weapon check
                for(unsigned int i = 0; i < 3; i++)
                {
                    if(myGameModel->isWeapon(selectedItems.at(i)->row()))
                    {
                        checkAmount++;
                        cardIndices[1] = i;
                        break;
                    }
                }

                //Room check
                for(unsigned int i = 0; i < 3; i++)
                {
                    if(myGameModel->isRoom(selectedItems.at(i)->row()))
                    {
                        checkAmount++;
                        cardIndices[2] = i;
                        break;
                    }
                }

                if(checkAmount == 3)
                {
                    bool ok = false;
                    unsigned int endPlayer = QInputDialog::getInt(this, "End player", "End player's number:", 1, 1, currentNumPlayers, 1, &ok);

                    if(ok)
                    {
                        myGameModel->processSuspicion(selectedItems.at(0)->column(), endPlayer, selectedItems.at(cardIndices[0])->row(), selectedItems.at(cardIndices[1])->row(), selectedItems.at(cardIndices[2])->row());
                        displayMatrix();
                    }
                }
                else
                {
                    QMessageBox::warning(this, "Notice", "You didn't select a guest, weapon and room");
                }
            }
            else
            {
                QMessageBox::warning(this, "Notice", "You have selected an invalid column / cards from more than one player");
            }
        }
    }
}

void ClueUI::displayMatrix(bool installItems)
{
    if(currentlyInGame)
    {
        unsigned int ** possessionMatrix = myGameModel->getMatrix();

        for(unsigned int i = 0; i < numRows; i++)
        {
            for(unsigned int j = 0; j < currentNumPlayers + 1; j++)
            {
                possessionMatrixItems[i][j].setText(QString::fromStdString(statusSymbols[possessionMatrix[j][i]]));
                possessionMatrixItems[i][j].setTextAlignment(Qt::AlignCenter);

                if(installItems)
                {
                    ui.possessionTable->setItem(i, j + 1, &possessionMatrixItems[i][j]);
                }
            }
        }

        QStringList columnTitles;
        QString fraction = " [";
        columnTitles << "Card";
        for(unsigned int i = 1; i < currentNumPlayers + 1; i++)
        {
            QString nextTitle = playerNames[i - 1];
            nextTitle.append(fraction).append(QString::number(myGameModel->getCardsDiscovered()[i - 1])).append('/').append(QString::number(numCardsPerPlayer[i - 1])).append(']');
            columnTitles << nextTitle;
        }
        columnTitles << "G";
        ui.possessionTable->setHorizontalHeaderLabels(columnTitles);

        unsigned int bestGuest, bestWeapon, bestRoom;
        myGameModel->getBestGuess(bestGuest, bestWeapon, bestRoom);
        QString accuse = "Accusation: ";

        if(bestGuest >= myBoardModel->getFirstGuest() && bestGuest <= myBoardModel->getLastGuest())
        {
            accuse.append(QString::fromStdString(myBoardModel->getComponentNames()[bestGuest]));
            accuse.append(", ");
        }

        if(bestWeapon >= myBoardModel->getFirstWeapon() && bestWeapon <= myBoardModel->getLastWeapon())
        {
            accuse.append(QString::fromStdString(myBoardModel->getComponentNames()[bestWeapon]));
            accuse.append(", ");
        }

        if(bestRoom >= myBoardModel->getFirstRoom() && bestRoom <= myBoardModel->getLastRoom())
        {
            accuse.append(QString::fromStdString(myBoardModel->getComponentNames()[bestRoom]));
            accuse.append(".");
        }

        ui.accusationLabel->setText(accuse);

        QString undoSize = "Undo history size: ";
        float undoHistorySize = ((float)myGameModel->getUndoHistorySize()) / 1000.0f;
        undoSize.append(QString::number(undoHistorySize, 'f', 2));
        undoSize.append("kB");

        ui.undoHistoryLabel->setText(undoSize);

        displayTheories();
    }
}

void ClueUI::displayTheories()
{
    if(currentlyInGame)
    {
        vector<vector<unsigned int> > * theories = myGameModel->getTheories();

        unsigned int numRows = 0;

        for(unsigned int i = 0; i < currentNumPlayers; i++)
        {
            if(theories[i].size() > numRows)
            {
                numRows = theories[i].size();
            }
        }

        if(numRows > 0)
        {
            for(unsigned int i = 0; i < theoryItems.size(); i++)
            {
                delete theoryItems[i];
            }

            theoryItems.clear();

            ui.theoryTable->setRowCount(numRows);

            for(unsigned int i = 0; i < currentNumPlayers; i++)
            {
                for(unsigned int j = 0; j < theories[i].size(); j++)
                {
                    QString theoryString;

                    for(unsigned int k = 0; k < theories[i].at(j).size(); k++)
                    {
                        theoryString.append(QString::fromStdString(myBoardModel->getComponentNames()[theories[i].at(j).at(k)]));
                        if(k < theories[i].at(j).size() - 1)
                        {
                            theoryString.append(", ");
                        }
                    }

                    QTableWidgetItem * newTheoryItem = new QTableWidgetItem;
                    newTheoryItem->setText(theoryString);
                    theoryItems.push_back(newTheoryItem);

                    ui.theoryTable->setItem(j, i, newTheoryItem);
                }
            }
        }
        else
        {
            for(unsigned int i = 0; i < theoryItems.size(); i++)
            {
                delete theoryItems[i];
            }

            theoryItems.clear();

            ui.theoryTable->setRowCount(0);
        }
    }
}

void ClueUI::undoLast()
{
    if(currentlyInGame)
    {
        myGameModel->stepBack();
        displayMatrix();
    }
}

vector<unsigned int> ClueUI::parseNumericInput(QString & inCards, unsigned int * numTokens)
{
    vector<string> tokens;

    tokenize(inCards.toStdString(), tokens);

    vector<unsigned int> numCards;

    for(unsigned int i = 0; i < tokens.size(); i++)
    {
        numCards.push_back(atoi(tokens.at(i).c_str()));
    }

    if(numTokens != 0)
    {
        *numTokens = tokens.size();
    }

    return numCards;
}

void ClueUI::tokenize(const string & inString, vector<string> & inTokens)
{
    const string & delimiters = " ";
    string::size_type lastPos = inString.find_first_not_of(delimiters, 0);
    string::size_type pos = inString.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        inTokens.push_back(inString.substr(lastPos, pos - lastPos));
        lastPos = inString.find_first_not_of(delimiters, pos);
        pos = inString.find_first_of(delimiters, lastPos);
    }
}

void ClueUI::quit()
{
    deconstruct();
    exit(0);
}
