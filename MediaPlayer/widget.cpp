#include "widget.h"
#include "ui_widget.h"
#include <QFileDialog>
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    Media = new MediaCore(ui->Show->width(),ui->Show->height(),(void*)ui->Show->winId());
    this->setFixedSize(this->width(),this->height());
    Media->setUpdateCallback([this](){ this->UpdateSts(); });
}

Widget::~Widget()
{
    Media->unInit();
    delete Media;
    delete ui;
}

void Widget::on_mediaChose_clicked()
{
    Media->SetPath(QFileDialog::getOpenFileName(this,"请选择媒体文件","D:/","(*.mp4 *.flv);;").toStdString().c_str());
    Media->Init();
    this->ui->Duration->setText(Media->durationStr.c_str());

    this->ui->horizontalSlider->setRange(0,Media->total_seconds);
}

void Widget::on_pushButton_clicked()
{
    static int flag = 1;
    if(flag == 1){
        ui->pushButton->setText("暂停");
        flag = 2;
        Media->StartPlayer();
    }
    else{
        ui->pushButton->setText("开始");
        flag = 1;
    }
}

void Widget::UpdateSts(){
    static int Minutes = 0;
    static int second = 0;
    static int Total = 0;
    Total++;
    second++;
    if(second == 60){
        Minutes++;
        second = 0;
    }
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", (int)Minutes, (int)second);
    ui->Time->setText(buffer);
    ui->horizontalSlider->setValue(Total);
}

void Widget::on_horizontalSlider_actionTriggered(int action)
{
    //执行seek操作
    Media->Seek(ui->horizontalSlider->value());
}
