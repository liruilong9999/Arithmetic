#include <QApplication>
#include <QMenuBar>

#include "gdalparser.h"
#include "iso8211parser.h"
#include "myparser.h"

int main(int argc, char * argv[])
{
    QApplication a(argc, argv);

	run();

    int ret = a.exec();
    return ret;
}