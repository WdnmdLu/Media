#include "widget.h"
#include "ui_widget.h"
#include <libavdevice/avdevice.h>
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    connect(this, &Widget::customSignal, this, &Widget::handleCustomSignal);
    worker = new WorkerThread(this);
}

void Widget::handleCustomSignal(int value){
    if(value == 1){
        // 开始录制
        worker->start();
    }// 结束录制
    else if(value == 2){
        worker->count = 1;
        worker->exit();
    }
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_record_clicked()
{
    static int flag = 0;
    if(flag == 0){
        flag = 1;
        this->ui->record->setText("退出录制");
        emit customSignal(1);
    }
    else if(flag == 1){
        //退出PCM的录制
        this->ui->record->setText("录制结束");
        emit customSignal(2);
        flag = 2;
    }
}

// 打开音频设备
void WorkerThread::OpenDevice(const char *devicename,AVFormatContext *fmt_ctx,AVDictionary *option){
    // 初始化网络库，对于dshow来说这一步是必要的
    avformat_network_init();
    // 注册所有FFmpeg设备
    avdevice_register_all();
    // 查找dshow输入格式，dshow是Windows下视频音频捕获的输入格式
    const AVInputFormat* iformat = av_find_input_format("dshow");
    av_dict_set(&option, "sample_rate", "44100", 0);
    av_dict_set(&option, "sample_fmt", "s16", 0);
    av_dict_set(&option, "channels", "2", 0);

    cout << "InputFormat: " << iformat->name<<endl;


    cout<<"Begin to open device"<<endl;
    // int avformat_open_input(AVFormatContext **ps, const char *url, const AVInputFormat *fmt, AVDictionary **options);
    int ret = avformat_open_input(&fmt_ctx, devicename, iformat, &option);
    // 如果打开设备失败，输出错误信息并返回
    if(ret < 0){
        char errors[1024];
        av_strerror(ret, errors, 1024);
        printf("failed to open device,[%d] %s\n", ret, errors);
        return;
    }
    cout << "Open device success " << endl;
    return;
}

void WorkerThread::run(){

    AVFormatContext *fmt_ctx = nullptr;
    fmt_ctx = avformat_alloc_context();
    AVPacket pkt;

    // 创建AVDictionary用于设置选项，初始为NULL
    AVDictionary *option = NULL;

    // 设定设备名称，这里使用的是之前获取的音频设备名称
    const char *devicename = "audio=麦克风阵列 (适用于数字麦克风的英特尔® 智音技术)";

    // 输出到控制台，表示线程开始工作
    cout << "Thread Working" << endl;


    // 打开文件out.pcm用于写入捕获的音频数据
    std::fstream file("out.pcm", std::ios::out | std::ios::binary);
    if(!file.is_open()){
        cout << "Open file error"<<endl;
        return;
    }

    OpenDevice(devicename,fmt_ctx,option);

    // 初始化pkt，准备读取数据
    av_init_packet(&pkt);
    // 循环读取音频帧
    cout << "Begin to capture Audio" <<endl;
    int ret = 0;
    while((ret = av_read_frame(fmt_ctx, &pkt)) == 0){

        // 将pkt中的数据写入到文件中
        file.write(reinterpret_cast<const char *>(pkt.data), pkt.size);
        cout<<"pkt size: "<< pkt.size<<"pkt data"<<pkt.data[0]<<endl;
        fflush(NULL);
        // 释放pkt占用的资源
        av_packet_unref(&pkt);
        // 如果count变量为1，则退出循环
        if(count == 1){
            break;
        }
    }
    // 关闭文件
    file.close();
    // 关闭设备上下文
    avformat_close_input(&fmt_ctx);
    // 反初始化网络库
    avformat_network_deinit();
}

