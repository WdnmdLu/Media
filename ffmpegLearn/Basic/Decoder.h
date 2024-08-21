#ifndef DECODER_H
#define DECODER_H
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavutil/log.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/mathematics.h>
#include <libavutil/avutil.h>

// 解析MP4文件生成对应的aac码流并输出到文件中
int adts_header(char * const p_adts_header, const int data_length,
                const int profile, const int samplerate,
                const int channels);

void ADTS_Header();

// 解码MP4文件生成对应的h264码流并输出到文件中
void Extract_h264(const char *input_filename, const char *output_filename);

void DecodeMP4(const char *filename);

void Dexumer();
#endif // DECODER_H
