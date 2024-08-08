#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QThread>
#include <iostream>
#include <fstream>
extern "C"
{
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
}


using namespace std;
QT_BEGIN_NAMESPACE

// 定义WorkerThread类，继承自QThread，使得它可以作为一个线程运行
class WorkerThread : public QThread {
    Q_OBJECT

public:
    // 构造函数，接受一个QObject指针作为父对象，如果没有父对象则为nullptr
    WorkerThread(QObject *parent = nullptr) : QThread(parent) {}

    void OpenDevice(const char *devicename,AVFormatContext *fmt_ctx,AVDictionary *option);

    // 重写QThread的run方法，这是线程执行的入口点
    void run() override;

    // count变量，可能用于控制循环或线程的退出
    int count = 0;
};

namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

signals:
    void customSignal(int value);
private slots:
    void on_record_clicked();
    void handleCustomSignal(int value);
private:
    Ui::Widget *ui;
    WorkerThread *worker;
};
#endif // WIDGET_H
