#include "buffer.h"
#include <iostream>
#include <stb_image_write.h>

FrameBufferObject::FrameBufferObject(int width, int height, GLint internalformat1, GLint internalformat2)
        : width(width), height(height),
          internalformat1(internalformat1), internalformat2(internalformat2),
          FBO(0), texture1(0), texture2(0), depthStencil(0)
{
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	// ---------- 创建第一个颜色附件 ----------
	texture1 = createColorTexture(width, height, internalformat1);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture1, 0);

	// ---------- 可选第二个颜色附件 ----------
	if (internalformat2 != 0) {
		texture2 = createColorTexture(width, height, internalformat2);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texture2, 0);

		GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, drawBuffers);
	} else {
		texture2 = 0;
		GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, drawBuffers);
	}

	// ---------- 深度模板附件 ----------
	glGenRenderbuffers(1, &depthStencil);
	glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH32F_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil);

	// ---------- 检查完整性 ----------
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "[FBO Error] Framebuffer is not complete: 0x"
					<< std::hex << status << std::dec << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FrameBufferObject::~FrameBufferObject(){
	if (depthStencil) glDeleteRenderbuffers(1, &depthStencil);
	if (texture1) glDeleteTextures(1, &texture1);
	if (texture2) glDeleteTextures(1, &texture2);
	if (FBO) glDeleteFramebuffers(1, &FBO);
}

void FrameBufferObject::use(void){
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glViewport(0, 0, width, height);
}

void FrameBufferObject::read1(void *tgt) const {
    readColorAttachment(GL_COLOR_ATTACHMENT0, internalformat1, tgt);
}

void FrameBufferObject::read2(void *tgt) const {
	if (texture2 == 0) return;
	readColorAttachment(GL_COLOR_ATTACHMENT1, internalformat2, tgt);
}

GLuint FrameBufferObject::createColorTexture(int w, int h, GLint internalformat) {
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	GLenum format, type;
	inferFormatType(internalformat, format, type);

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, w, h, 0, format, type, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return tex;
}


void FrameBufferObject::inferFormatType(GLint internalformat, GLenum &format, GLenum &type) const {
	switch (internalformat) {
	case GL_RGB:
		format = GL_RGB;
		type = GL_UNSIGNED_BYTE;
		break;
	case GL_RGB32F:
		format = GL_RGB;
		type = GL_FLOAT;
		break;
	case GL_R32F:
		format = GL_RED;
		type = GL_FLOAT;
		break;
	default:
		std::cerr << "[FBO Warning] Unsupported internalformat: " << internalformat << std::endl;
		format = GL_RGB;
		type = GL_UNSIGNED_BYTE;
		break;
	}
}

void FrameBufferObject::readColorAttachment(GLenum attachment, GLint internalformat, void *tgt) const {
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	GLenum format, type;
	inferFormatType(internalformat, format, type);
	glReadBuffer(attachment);
	glReadPixels(0, 0, width, height, format, type, tgt);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


CPUImageBuffer::CPUImageBuffer(int width, int height, int pixel_size) : width(width), height(height), pixel_size(pixel_size)
{
	data = (unsigned char*) malloc(width*height*pixel_size);
}

CPUImageBuffer::~CPUImageBuffer()
{
	free(data);
}

void CPUImageBuffer::write_to_png(std::string path)
{
	stbi_write_png(path.c_str(), width, height, 3, (void*)data, width * 3);
}

void CPUImageBuffer::flip(void)
{
	unsigned char *buf = (unsigned char*) malloc(width * pixel_size);
	for(int i = 0 ; i < height / 2; i++){
		int j = height - 1 - i;
		memcpy(buf, data + i*width*pixel_size, width*pixel_size);
		memcpy(data + i*width*pixel_size, data + j*width*pixel_size, width*pixel_size);
		memcpy(data +j*width*pixel_size, buf, width*pixel_size);
	}
	free(buf);
}