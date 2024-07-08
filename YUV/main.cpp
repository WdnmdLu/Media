#include <stdio.h>
#include <stdlib.h>

// 从代码可以看出，如果视频帧的宽高分别位w和h，那么一帧YUV420P像素数据一共占用w*h*3/2 Btye的数据
// 其中前w*h Byte存储y 接着w*h*1/4 Byte存储U 最后w*h*1/4 Byte存储V
int simplest_yuv420_split(char *url, int w, int h,int num){
    FILE *fp = fopen(url,"rb+");
    FILE *fp1 = fopen("output_420_y.yuv", "wb+");  // 以写入二进制方式打开输出的Y分量文件
	FILE *fp2 = fopen("output_420_u.yuv", "wb+");  // 以写入二进制方式打开输出的U分量文件
	FILE *fp3 = fopen("output_420_v.yuv", "wb+");  // 以写入二进制方式打开输出的V分量文件
    unsigned char *pic=(unsigned char*)malloc(w*h*3/2);
    //num表示帧数，读取一帧数据即可
    for(int i=0;i<num;i++){
        fread(pic,1,w*h*3/2,fp);
        // 从读取到的一帧数据分别将Y U V三个分量提取出来
        //Y   提取Y分量并写入
        fwrite(pic,1,w*h,fp1);
        //U   提取U分量并写入
        fwrite(pic+w*h,1,w*h/4,fp2);
        //V   提取V分量并写入
        fwrite(pic+w*h*5/4,1,w*h/4,fp3);
    }
    free(pic);
    fclose(fp);
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
    return 0;
}
// yuv444格式先y再u再v
int simplest_yuv444_split(char *url, int w, int h,int num){
    FILE *fp = fopen(url,"rb+");
    FILE *fp1 = fopen("output_444_y.y", "wb+");  // 以写入二进制方式打开输出的Y分量文件
	FILE *fp2 = fopen("output_444_u.y", "wb+");  // 以写入二进制方式打开输出的U分量文件
	FILE *fp3 = fopen("output_444_v.y", "wb+");  // 以写入二进制方式打开输出的V分量文件
    unsigned char *pic=(unsigned char*)malloc(w*h*3/2);
    //num表示帧数，读取一帧数据即可
    for(int i=0;i<num;i++){
        //从fp中读取w*h*3个Byte的数据到pic中
        fread(pic,1,w*h*3,fp);
        // 从读取到的一帧数据分别将Y U V三个分量提取出来
        //Y   提取Y分量并写入 
        fwrite(pic,1,w*h,fp1);
        //U   提取U分量并写入
        fwrite(pic+w*h,1,w*h,fp2);
        //V   提取V分量并写入
        fwrite(pic+w*h*2,1,w*h,fp3);
    }
    free(pic);
    fclose(fp);
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
    return 0;
}

int main(){
    simplest_yuv420_split("aaa.yuv",176,144,1);
    return 0;
}