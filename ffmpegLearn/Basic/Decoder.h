#ifndef DECODER_H
#define DECODER_H
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <SDL.h>
#include <string.h>
void Decoder();
void Dexumer();
void extract_h264(const char *filename);
#endif // DECODER_H
