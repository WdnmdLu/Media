#include "widget.h"
#include <QApplication>
#undef main
int main(int argc, char *argv[])
{
    cout<<"Hello World"<<endl;
    QApplication a(argc, argv);

    Widget w;
    w.show();
    return a.exec();
}
