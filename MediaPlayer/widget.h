#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <mediacore.h>
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT
public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void UpdateSts();
private slots:
    void on_mediaChose_clicked();

    void on_pushButton_clicked();


    void on_horizontalSlider_actionTriggered(int action);

private:
    Ui::Widget *ui;

    MediaCore *Media;

};
#endif // WIDGET_H
