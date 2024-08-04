#include <stdio.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// 音频重采样
int main() {
    // 输入流上下文
    AVFormatContext *fmt_ctx = NULL;
    // 字典参数
    AVDictionary *option = NULL;
    // 设备名称
    char *devicename = "hw:0,0";

    printf("avdevice_register_all\n");
    // 注册所有设备
    avdevice_register_all();

    // 查找输入格式为alsa（Advanced Linux Sound Architecture）
    AVInputFormat *iformat = av_find_input_format("alsa");

    // 打开设备
    int ret = avformat_open_input(&fmt_ctx, devicename, iformat, &option);
    if (ret < 0) {
        printf("Open device error\n");
        return -1;
    }

    // 初始化AVPacket结构体
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    int count = 0;

    // 打开输出文件out.pcm
    int fd = open("out.pcm", O_WRONLY | O_CREAT, 0666);
    if (fd < 0) {
        printf("Could not open output file\n");
        avformat_close_input(&fmt_ctx);
        return -1;
    }

    SwrContext *swr_ctx = NULL;
    swr_ctx = swr_alloc_set_opts(NULL,
        AV_CH_LAYOUT_STEREO,AV_SAMPLE_FMT_S16,44100,
        AV_CH_LAYOUT_STEREO,AV_SAMPLE_FMT_FLT,44100,
        0,NULL);

    swr_init(swr_ctx);
    uint8_t **src_data;
    int src_linesize = 0;

    uint8_t **dts_data;
    int dts_linesize = 0;
    // 创建输入缓冲区
    av_samples_alloc_array_and_samples(&src_data,     // 输出缓冲区地址
                                       &src_linesize, // 缓冲区大小
                                        2,            // 通道个数
                                        512,          // 单通道个数
                                        AV_SAMPLE_FMT_FLT, //采样格式
                                        0);

    // 创建输出缓冲区
    av_samples_alloc_array_and_samples(&dts_data,     // 输出缓冲区地址
                                       &dts_linesize, // 缓冲区大小
                                        2,            // 通道个数
                                        512,          // 单通道个数
                                        AV_SAMPLE_FMT_S16, //采样格式
                                        0);
    // 循环读取数据帧直到结束
    while ((ret = av_read_frame(fmt_ctx, &pkt)) >= 0) {
        printf("pkt size is %d  data: %p\n", pkt.size, pkt.data);
        memcpy((void*)src_data[0],(void*)pkt.data,pkt.size);
        // 重采样
        swr_convert(swr_ctx,
                    dts_data,
                    512,
                    (const uint8_t **)src_data,
                    512);
        // 将数据写入文件
        //write(fd, pkt.data, pkt.size);
        write(fd,dts_data[0],dts_linesize);
        // 清理数据包
        av_packet_unref(&pkt);
    }

    // 资源释放
    if(src_data){
        av_freep(&src_data[0]);
    }
    av_freep(&src_data);

    if(dts_data){
        av_freep(&dts_data[0]);
    }
    av_freep(&dts_data);
    swr_free(&swr_ctx);

    // 关闭文件
    close(fd);
    // 再次清理数据包（虽然上面循环内已经清理过了）
    av_packet_unref(&pkt);
    // 关闭设备
    avformat_close_input(&fmt_ctx);
    
    return 0; 
}