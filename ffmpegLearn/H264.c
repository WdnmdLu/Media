#include <stdio.h>
#include <libavutil/log.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>

// 从视频文件中提取处视频流，将其输出到其他文件中
static char err_buff[128] = {0};
static char* av_get_err(int errnum){
    av_strerror(errnum,err_buff,128);
    return err_buff;
}

int main(int argc,char *argv[]){
    AVFormatContext *ifmt_ctx = NULL;
    int videoIndex = -1;
    AVPacket *pkt = NULL;
    int ret = -1;
    int file_end = 0;

    if(argc < 3){
        printf("Inputfile Outputfile \n");
        return -1;
    }
    FILE *outfp = fopen(argv[2],"wb");
    printf("int:%s out:%s\n",argv[1],argv[2]);
    // 分配上下文
    ifmt_ctx = avformat_alloc_context();
    if(!ifmt_ctx){
        printf("[error] Could not allocate context\n");
        return -1;
    }
    // 打开视频文件
    ret = avformat_open_input(&ifmt_ctx,argv[1],NULL,NULL);
    if(ret != 0){
        printf("[error] Could open input file\n");
        return -1;
    }
    //获取流信息
    ret = avformat_find_stream_info(ifmt_ctx,NULL);
    if(ret < 0){
        printf("[error] Get Stream_info faile\n");
        return -1;
    }
    // 查找视频流在流信息中的索引
    videoIndex = av_find_best_stream(ifmt_ctx,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
    if(videoIndex == -1){
        printf("[error] Find Video_Stream faile\n");
        avformat_close_input(&ifmt_ctx);
        return -1;
    }
    // 分配AVPacket
    pkt = av_packet_alloc();
    av_init_packet(pkt);
    // 获取h264比特率过滤器
    const AVBitStreamFilter *bsfilter = av_bsf_get_by_name("h264_mp4toannexb");
    AVBSFContext *bsf_ctx = NULL;
    //初始化比特流过滤器上下文
    av_bsf_alloc(bsfilter, &bsf_ctx);
    avcodec_parameters_copy(bsf_ctx->par_in,ifmt_ctx->streams[videoIndex]->codecpar);
    av_bsf_init(bsf_ctx);

    file_end = 0;
    while (0 == file_end)
    {
        // 读取帧
        if ((ret = av_read_frame(ifmt_ctx,pkt))<0){
            file_end = 1;
            printf("read file end: ret:%d\n",ret);
            break;
        }
        if(ret == 0 && pkt->stream_index == videoIndex){
            size_t size = fwrite(pkt->data, 1, pkt->size, outfp);
            if(size != pkt->size) {
                printf("fwrite failed -> write:%u, pkt_size:%u\n",(unsigned int)size,pkt->size);
            }
            av_packet_unref(pkt);
        }
        else{
            if(ret == 0)
                av_packet_unref(pkt);
        }
    }
     // 关闭输出文件
    if (outfp)
        fclose(outfp);

    // 释放比特流过滤器上下文
    if (bsf_ctx)
        av_bsf_free(&bsf_ctx);

    // 释放AVPacket
    if (pkt)
        av_packet_free(&pkt);

    // 关闭输入文件
    if (ifmt_ctx)
        avformat_close_input(&ifmt_ctx);
    printf("finish\n");
    return 0;
}