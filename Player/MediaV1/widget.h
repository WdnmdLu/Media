#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <player.h>
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void UpdateSlider();

private slots:
    void on_Chose_clicked();

    void on_Play_clicked();

private:
    Ui::Widget *ui;

    Player *player;
};
#endif // WIDGET_H
