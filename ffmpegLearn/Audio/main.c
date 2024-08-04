#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int run_capture = 0; // 用于控制捕获的全局变量

// 线程函数，用于监控终端输入
void* monitor_input(void* arg) {
    char input;
    while (1) {
        printf("Enter '1' to start capture, '0' to stop: ");
        fflush(stdout); // 刷新输出，确保提示信息立即显示
        if (scanf("%c", &input) != 1) {
            continue; // 读取错误，继续循环
        }

        pthread_mutex_lock(&mutex);
        if (input == '1') {
            run_capture = 1; // 设置标志，开始捕获
        } else if (input == '0') {
            run_capture = 0; // 设置标志，准备停止捕获
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    // 初始化FFmpeg库
    avformat_network_init();

    // 输入流上下文
    AVFormatContext *fmt_ctx = NULL;
    // 设备名称
    char *devicename = "hw:0,0";

    // 注册所有设备
    avdevice_register_all();

    // 查找输入格式为alsa
    AVInputFormat *iformat = av_find_input_format("alsa");

    // 打开设备
    int ret = avformat_open_input(&fmt_ctx, devicename, iformat, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open input device: %s\n", av_err2str(ret));
        return -1;
    }

    // 创建并启动监控输入的线程
    pthread_t input_thread;
    if (pthread_create(&input_thread, NULL, monitor_input, NULL) != 0) {
        perror("Failed to create thread");
        return -1;
    }

    // 打开输出文件
    int fd = open("out.pcm", O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        perror("Could not open output file");
        avformat_close_input(&fmt_ctx);
        return -1;
    }

    // 等待输入线程设置run_capture标志
    pthread_mutex_lock(&mutex);
    while (!run_capture) {
        pthread_mutex_unlock(&mutex); // 解锁以避免优先级反转
        usleep(100000); // 稍作延时，避免CPU占用过高
        pthread_mutex_lock(&mutex);
    }
    pthread_mutex_unlock(&mutex);

    // 初始化AVPacket结构体
    AVPacket pkt;
    av_init_packet(&pkt);

    // 循环读取数据帧直到结束
    while (run_capture && (ret = av_read_frame(fmt_ctx, &pkt)) >= 0) {
        // 将数据写入文件
        if (write(fd, pkt.data, pkt.size) < 0) {
            perror("Error writing to file");
        }
        av_packet_unref(&pkt);
    }

    // 清理
    pthread_mutex_lock(&mutex);
    run_capture = 0; // 确保不会重新启动
    pthread_mutex_unlock(&mutex);

    // 等待输入线程结束
    pthread_join(input_thread, NULL);

    // 关闭文件和输入设备
    close(fd);
    av_packet_unref(&pkt);
    avformat_close_input(&fmt_ctx);

    // 清理FFmpeg网络库
    avformat_network_deinit();

    return 0;
}