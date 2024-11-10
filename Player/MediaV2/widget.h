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

    void on_horizontalSlider_valueChanged(int value);

    void on_horizontalSlider_actionTriggered(int action);

    void on_horizontalSlider_sliderPressed();

    void on_horizontalSlider_sliderReleased();

private:
    Ui::Widget *ui;

    Player *player;
};
#endif // WIDGET_H
