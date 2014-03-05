#include "clueui.h"

#include <QtGui>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ClueUI w;
    w.show();
    return a.exec();
}
