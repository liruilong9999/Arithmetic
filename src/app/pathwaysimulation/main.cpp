#include <QApplication>
#include <QMenuBar>
#include <QTime>
#include "simulation.h"

int main(int argc, char * argv[])
{
    QApplication a(argc, argv);

    qsrand(QTime(0, 0, 0).msec());
    Simulation s;
    for (int i = 1001; i < 1011; i++)
    {
        s.startSimulation(i, 0);
        break;
    }

    s.save();
    int ret = a.exec();
    return ret;
}