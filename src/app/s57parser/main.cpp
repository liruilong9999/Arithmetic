#include <QApplication>
#include <QMenuBar>

#include "gdalparser.h"
#include "iso8211parser.h"

int main(int argc, char * argv[])
{
    QApplication a(argc, argv);



    int ret = a.exec();
    return ret;
}