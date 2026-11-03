#ifndef BUFFER_H
#define BUFFER_H

#include <glad/glad.h>
#include <string>

class FrameBufferObject {
public:
    GLuint FBO;         // FBO 对象 ID
    GLuint texture1;    // 第一个颜色附件纹理 ID
    GLuint texture2;    // 第二个颜色附件纹理 ID（可选）
    GLuint depthStencil; // 深度模板附件
    GLint internalformat1;
    GLint internalformat2;
    int width;
    int height;

    FrameBufferObject(int width, int height, GLint internalformat1, GLint internalformat2 = 0);

    ~FrameBufferObject();

    void use(void);

    void read1(void *tgt) const;

    void read2(void *tgt) const;

private:
    GLuint createColorTexture(int w, int h, GLint internalformat);

    void inferFormatType(GLint internalformat, GLenum &format, GLenum &type) const;

    void readColorAttachment(GLenum attachment, GLint internalformat, void *tgt) const;
};

class CPUImageBuffer {
public:
	int width;
	int height;
	int pixel_size; // 一个 pixel 占的字节数
	unsigned char *data;
	
	CPUImageBuffer(int width, int height, int pixel_size = 3);
    ~CPUImageBuffer();

	void write_to_png(std::string path);

	void flip(void);

};

#endif
