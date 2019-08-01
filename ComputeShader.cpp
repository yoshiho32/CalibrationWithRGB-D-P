
#include "ComputeShader.h"



ComputeShader::ComputeShader(int width, int height, const char *source, int uniforms, int targets)
	: width(width)
	, height(height)
	, program(gg2::loadCSProgram(source))
	, uniforms(uniforms)
{
	/*
	static const GLfloat border[] = { 0.0, 0.0, 0.0, 0.0 };
	
	//アウトプット用のテクスチャを作成する
	glGenTextures(1, &tex_A);
	glBindTexture(GL_TEXTURE_2D, tex_A);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

	glGenTextures(1, &tex_B);
	glBindTexture(GL_TEXTURE_2D, tex_B);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	
	glGenTextures(1, &tex_C);
	glBindTexture(GL_TEXTURE_2D, tex_C);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	*/
	ggError(__FILE__, __LINE__);

	

}


ComputeShader::~ComputeShader() {

	// シェーダプログラムを削除する
	glDeleteShader(program);

}

// 計算を実行する
const std::vector<GLuint> &ComputeShader::calculate(int karnel_w, int karnel_h) const
{
	// 処理の実行
	glDispatchCompute(width, height, 1);
	
	//画素の書き込みが終わるまで待つ
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	//std::cout << tex_output << std::endl;
	return texture;
}
