#include "openglwidget.h"

#include <QDebug>

#include <QMouseEvent>

CCOpenGLWidget::CCOpenGLWidget(QWidget *parent):QOpenGLWidget(parent)
{

    m_pBufYuv420p = NULL;

    m_pShaderProgram = NULL;

    m_nVideoH = 0;
    m_nVideoW = 0;
    m_yFrameLength =0;
    m_uFrameLength =0;
    m_vFrameLength =0;


}

CCOpenGLWidget::~CCOpenGLWidget()
{

    if(m_pShaderProgram != NULL){
        delete m_pShaderProgram;
        m_pShaderProgram= NULL;
    }

    if(NULL != m_pBufYuv420p)
    {
        free(m_pBufYuv420p);
        m_pBufYuv420p=NULL;
    }

     glDeleteTextures(3, m_textures);

}

void CCOpenGLWidget::RendVideo(H264YUV_Frame* yuvFrame)
{

    if(yuvFrame == NULL ){
        return;
    }
    if(m_nVideoH != yuvFrame->height || m_nVideoW != yuvFrame->width){

        if(NULL != m_pBufYuv420p)
        {
            free(m_pBufYuv420p);
            m_pBufYuv420p=NULL;
        }
    }

    m_nVideoW = yuvFrame->width;
    m_nVideoH = yuvFrame->height;

    m_yFrameLength = yuvFrame->luma.length;
    m_uFrameLength = yuvFrame->chromaB.length;
    m_vFrameLength = yuvFrame->chromaR.length;

    //申请内存存一帧yuv图像数据,其大小为分辨率的1.5倍
    int nLen = m_yFrameLength + m_uFrameLength +m_vFrameLength;

    if(NULL == m_pBufYuv420p)
    {
        m_pBufYuv420p = ( unsigned char*) malloc(nLen);
    }

    memcpy(m_pBufYuv420p,yuvFrame->luma.dataBuffer,m_yFrameLength);
    memcpy(m_pBufYuv420p+m_yFrameLength,yuvFrame->chromaB.dataBuffer,m_uFrameLength);
    memcpy(m_pBufYuv420p+m_yFrameLength +m_uFrameLength,yuvFrame->chromaR.dataBuffer,m_vFrameLength);


    m_bUpdateData =true;

    update();


}

void CCOpenGLWidget::initializeGL()
{
    m_bUpdateData = false;

    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5,0.5,0.5,1.0);//设置背景色

    glGenTextures(3, m_textures);

    initializeGLSLShaders();

    return;

}

// 初始化OpenGL的GLSL着色器
void CCOpenGLWidget::initializeGLSLShaders()
{
    // 创建一个新的顶点着色器对象
    QOpenGLShader* vertexShader = new QOpenGLShader(QOpenGLShader::Vertex,this);
    // 编译顶点着色器的源码，该源码存储在文件"vertex.vert"中
    bool bCompileVS = vertexShader->compileSourceFile("./vertex.vert");
    // 检查顶点着色器是否编译成功
    if(bCompileVS == false){
        qDebug()<<"VS Compile ERROR:"<<vertexShader->log();
    }

    // 创建一个新的片段着色器对象
    QOpenGLShader* fragmentShader = new QOpenGLShader(QOpenGLShader::Fragment,this);
    // 编译片段着色器的源码，该源码存储在文件"fragment.frag"中
    bool bCompileFS = fragmentShader->compileSourceFile("./fragment.frag");
    // 检查片段着色器是否编译成功
    if(bCompileFS == false){
        qDebug()<<"FS Compile ERROR:"<<fragmentShader->log();
    }

    // 创建一个新的着色器程序对象
    m_pShaderProgram = new QOpenGLShaderProgram();
    // 将顶点着色器添加到着色器程序中
    m_pShaderProgram->addShader(vertexShader);
    // 将片段着色器添加到着色器程序中
    m_pShaderProgram->addShader(fragmentShader);

    // 链接着色器程序，将顶点着色器和片段着色器链接成一个完整的着色器程序
    bool linkStatus = m_pShaderProgram->link();
    // 检查着色器程序是否链接成功
    if(linkStatus == false){
        qDebug()<<"LINK ERROR:"<<m_pShaderProgram->log();
    }

    // 释放顶点着色器对象占用的内存
    if(vertexShader != NULL){
        delete vertexShader;
        vertexShader = NULL;
    }

    // 释放片段着色器对象占用的内存
    if(fragmentShader != NULL){
        delete fragmentShader;
        fragmentShader = NULL;
    }
}


void CCOpenGLWidget::resizeGL(int w, int h)
{
    printf("Width: %d  Height: %d\n",w,h);
    fflush(NULL);
    glViewport(0,0, w,h);
}

// 渲染OpenGL内容
void CCOpenGLWidget::paintGL()
{
    // 清除颜色和深度缓冲区，为新的绘制操作做准备
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // 重置当前的模型观察矩阵
    glLoadIdentity();

    // 如果没有新的数据要更新，直接返回
    if(m_bUpdateData == false){
        return;
    }

    // 定义一个包含顶点位置和纹理坐标数据的数组
    static CCVertex triangleVert[] = {
        {-1, 1,  0,     0,0}, // 左上角顶点
        {-1, -1, 0,    0,1}, // 左下角顶点
        {1,  1,  1,     1,0}, // 右上角顶点
        {1,  -1, -1,    1,1}  // 右下角顶点
    };

    // 创建一个变换矩阵，用于对渲染的物体进行变换操作
    QMatrix4x4 matrix;
    // 设置正交投影，定义视场和远近裁剪面
    matrix.ortho(-1,1,-1,1,0.1,1000);
    // 将物体沿Z轴负方向移动，以便在摄像机前渲染
    matrix.translate(0,0,-2);

    // 绑定着色器程序，以便使用定义好的顶点着色器和片段着色器
    m_pShaderProgram->bind();

    // 设置着色器中的统一变量，传递变换矩阵
    m_pShaderProgram->setUniformValue("uni_mat",matrix);

    // 启用顶点位置和UV属性数组，以便着色器可以访问这些数据
    m_pShaderProgram->enableAttributeArray("attr_position");
    m_pShaderProgram->enableAttributeArray("attr_uv");

    // 设置顶点位置和UV属性数组，告诉OpenGL如何找到这些数据
    m_pShaderProgram->setAttributeArray("attr_position",GL_FLOAT,triangleVert,3,sizeof(CCVertex));
    m_pShaderProgram->setAttributeArray("attr_uv",GL_FLOAT,&triangleVert[0].u,2,sizeof(CCVertex));

    // 激活并绑定Y纹理
    m_pShaderProgram->setUniformValue("uni_textureY",0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    // 设置纹理参数
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_nVideoW, m_nVideoH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_pBufYuv420p);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 重复上述步骤，绑定U纹理和V纹理
    m_pShaderProgram->setUniformValue("uni_textureU",1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textures[1]);
    // 设置纹理参数
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,m_nVideoW/2, m_nVideoH/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (char*)(m_pBufYuv420p+m_yFrameLength));
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_pShaderProgram->setUniformValue("uni_textureV",2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_textures[2]);
    // 设置纹理参数
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_nVideoW/2, m_nVideoH/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (char*)(m_pBufYuv420p+m_yFrameLength+m_uFrameLength));
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 绘制由两个三角形组成的四边形，用于展示视频帧
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // 禁用顶点位置和UV属性数组
    m_pShaderProgram->disableAttributeArray("attr_position");
    m_pShaderProgram->disableAttributeArray("attr_uv");

    // 释放着色器程序
    m_pShaderProgram->release();
}

GLuint CCOpenGLWidget::createImageTextures(QString &pathString)
 {
     unsigned int    textureId;
     glGenTextures(1,&textureId);//产生纹理索引
     glBindTexture(GL_TEXTURE_2D,textureId); //绑定纹理索引，之后的操作都针对当前纹理索引

     QImage texImage=QImage(pathString.toLocal8Bit().data());

     glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);//指当纹理图象被使用到一个大于它的形状上时
     glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);//指当纹理图象被使用到一个小于或等于它的形状上时
     glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,texImage.width(),texImage.height(),0,GL_RGBA,GL_UNSIGNED_BYTE,texImage.rgbSwapped().bits());//指定参数，生成纹理

     return textureId;
 }
