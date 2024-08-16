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

int main() {
    // 初始化FFmpeg库
    avformat_network_init();

    // 输入流上下文
    AVFormatContext *fmt_ctx = NULL;
    // 设备名称
    char *devicename = "hw:0,0";

    // 注册所有设备
    avdevice_register_all();

    // 查找输入格式为alsa  音频设备的输入格式
    AVInputFormat *iformat = av_find_input_format("alsa");

    // 打开设备
    int ret = avformat_open_input(&fmt_ctx, devicename, iformat, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open input device: %s\n", av_err2str(ret));
        return -1;
    }


    // 打开输出文件
    int fd = open("out.pcm", O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        perror("Could not open output file");
        avformat_close_input(&fmt_ctx);
        return -1;
    }

    // 初始化AVPacket结构体
    AVPacket pkt;
    av_init_packet(&pkt);

    // 循环读取数据帧直到结束
    while ((ret = av_read_frame(fmt_ctx, &pkt)) >= 0) {
        // 将数据写入文件
        if (write(fd, pkt.data, pkt.size) < 0) {
            perror("Error writing to file");
        }
        av_packet_unref(&pkt);
    }
    // 关闭文件和输入设备
    close(fd);
    av_packet_unref(&pkt);
    avformat_close_input(&fmt_ctx);

    // 清理FFmpeg网络库
    avformat_network_deinit();

    return 0;
}