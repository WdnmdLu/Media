#include "widget.h"
#include "ui_widget.h"
#include <QFileDialog>
#include <QMessageBox>
static int flag1 = 1;
static int flag2 = 1;
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    printf("InitSuccress\n");
    player = new Player();
    fflush(NULL);
}

Widget::~Widget()
{
    delete(player);
    printf("~Widget\n");
    fflush(NULL);
    delete ui;
}


void Widget::on_Chose_clicked()
{
    player->SetFilePath(QFileDialog::getOpenFileName(this,"请选择文件","D:/","(*.mp4 *.flv);;").toStdString().c_str());
    printf("InitFFmpegok\n");
    fflush(NULL);
    ui->horizontalSlider->setRange(0,player->TotalTime);
}

void Widget::on_Play_clicked()
{
    static int flag = 1;
    if(flag == 1){
        this->player->setUpdateCallback(std::bind(&Widget::UpdateSlider,this));
        player->setRenderCallback(std::bind(&CCOpenGLWidget::RendVideo, ui->Show, std::placeholders::_1));
        printf("SetCallBackOK\n");
        fflush(NULL);
        //这个是切换新的视频进行播放
        if(!player->Begin()){
            QMessageBox::information(this,"Error","未选择视频，无法播放");
            flag = 1;
            return;
        }
        int Seconds = player->TotalTime%60;
        int Minutes = player->TotalTime/60;
        char buffer[64] = {0};
        sprintf(buffer, "%02d:%02d", Minutes, Seconds);
        ui->Total->setText(buffer);
        flag = 2;
        this->ui->Play->setText("暂停");
    }//执行暂停操作
    else if(flag == 2){
        flag = 3;
        this->ui->Play->setText("播放");
        //调用暂停函数
        player->Stop();
    }
    else{
        flag = 2;
        this->ui->Play->setText("暂停");
        //这个是让正在播放的视频从暂停状态切换到播放状态接着播放
        this->player->Play();
    }
}

void Widget::UpdateSlider(){
    static int Time = 0;
    static int Seconds = 0;// 分
    static int Minutes = 0;// 秒
    if(flag1 == 2 || flag2 == 2)
    {
        return;
    }
    Time = ui->horizontalSlider->value();
    Time++;
    Seconds = Time%60;
    Minutes = Time/60;
    char buffer[64] = {0};
    sprintf(buffer, "%02d:%02d", Minutes, Seconds);

    //转化完成后就进行显示
    ui->Time->setText(buffer);
    ui->horizontalSlider->setValue(Time);
}

void Widget::on_horizontalSlider_valueChanged(int value)
{
    if(flag2 == 2 || flag1 == 1){
        return;
    }
    printf("Seek\n");
    fflush(NULL);
    flag1 = 2;
    //进行seek操作
    player->Seek(ui->horizontalSlider->value());
    flag1 = 1;
}

void Widget::on_horizontalSlider_actionTriggered(int action)
{
    if(flag2 == 2){
        return;
    }
    flag1 = 2;
}

void Widget::on_horizontalSlider_sliderPressed()
{
    flag2 = 2;
}

void Widget::on_horizontalSlider_sliderReleased()
{
    player->Seek(this->ui->horizontalSlider->value());
    flag2 = 1;
    return;
}
