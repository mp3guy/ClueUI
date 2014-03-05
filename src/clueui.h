#ifndef CLUEUI_H
#define CLUEUI_H

#include "math.h"
#include <vector>
#include <string>
#include <sstream>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include "QtGui/QInputDialog"
#include "../build/ui_clueui.h"

#include "GameModel.h"

using namespace std;

class ClueUI : public QMainWindow
{
    Q_OBJECT

    public:
        ClueUI(QWidget * parent = 0);
        ~ClueUI();

    private:
        BoardModel * myBoardModel;

        Ui::ClueUIClass ui;

        QTableWidgetItem * componentTableItems;
        unsigned int numRows;

        QString * playerNames;
        bool currentlyInGame;
        vector<unsigned int> numCardsPerPlayer;
        unsigned int currentNumPlayers;
        GameModel * myGameModel;
        QTableWidgetItem ** possessionMatrixItems;
        vector<QTableWidgetItem *> theoryItems;

        void displayMatrix(bool installItems = false);
        void displayTheories();

        void setupGui();
        void deconstruct();
        void closeEvent(QCloseEvent * close);
        void resizeEvent(QResizeEvent * resize);

        void optimiseTableSizes();

        vector<unsigned int>  parseNumericInput(QString & inCards, unsigned int * numTokens = 0);
        void tokenize(const string & str, vector<string> & tokens);

    private slots:
        void undoLast();
        void quit();
        void newGame();
        void enterName(int section);
        void addKnownCard(int row, int column);
        void crossOffCard(const QPoint & point);
        void addTheory();
};

#endif // CLUEUI_H
