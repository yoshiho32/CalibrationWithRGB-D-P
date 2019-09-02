//
// Kinect (v2) �̃f�v�X�}�b�v�擾
//

// �W�����C�u����
#include <Windows.h>

// �E�B���h�E�֘A�̏���
#include "Window.h"

// �Z���T�֘A�̏���
#include "KinectV2.h"

// �`��ɗp���郁�b�V��
#include "Mesh.h"

// �v�Z�ɗp����V�F�[�_
#include "Calculate.h"

// �听�����͂��s�����߂̃V�F�[�_
#include "ComputeShader.h"

// ���_�ʒu�̐������V�F�[�_ (position.frag) �ōs���Ȃ� 1
#define GENERATE_POSITION 1

// SSBO�̃f�[�^�̃`�F�b�N���s���ۂ� 1
#define DATACHECK 1
#define DATACHECK_CALC 0

#define FAR  300
#define NEAR 20

// lsm��p����ۂ� 1
#define LSM_OPTION 1

// LU�������s���ۂ� 1
#define LU_DEP 1

// ssbo�̃f�[�^�Z�b�g
struct DataSet {
	GLfloat data_xyz[4];
	GLfloat data_xy2[4];
	GLfloat data_xyzx[4];
};

// �e�N�X�`�����쐬����
// CLAMP_MODE�Ńe�N�X�`���̒[���w��
// 0 : GL_CLAMP_TO_EDGE  �i�܂�Ԃ�
// �f�t�H���g	 1 : GL_CLAMP_TO_BORDER (border�œh��i�^�����j
void makeTex(GLuint p, int width, int height, int CLAMP_MODE = 1) {

	// �e�N�X�`���̍쐬
	glBindTexture(GL_TEXTURE_2D, p);

	// LAP_MODE�̒l�ɂ���ċ�����ς���
	if (CLAMP_MODE == 0) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	if (CLAMP_MODE == 1) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		static const GLfloat border[] = { 0.0, 0.0, 0.0, 0.0 };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}


//
// ���C���v���O����
//
int main()
{
  // GLFW ������������
  if (glfwInit() == GL_FALSE)
  {
    // GLFW �̏������Ɏ��s����
    MessageBox(NULL, TEXT("GLFW �̏������Ɏ��s���܂����B"), TEXT("���܂�̂�"), MB_OK);
    return EXIT_FAILURE;
  }

  // �v���O�����I�����ɂ� GLFW ���I������
  atexit(glfwTerminate);
  // �f�[�^��

  // OpenGL Version 4.3 Core Profile ��I������
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // �E�B���h�E���J��
  Window window(640, 480, "Depth Map Viewer");
  if (!window.get())
  {
    // �E�B���h�E���쐬�ł��Ȃ�����
    MessageBox(NULL, TEXT("GLFW �̃E�B���h�E���J���܂���ł����B"), TEXT("���܂�̂�"), MB_OK);
    return EXIT_FAILURE;
  }

  // �[�x�Z���T��L���ɂ���
  KinectV2 sensor;
  if (sensor.getActivated() == 0)
  {
    // �Z���T���g���Ȃ�����
    MessageBox(NULL, TEXT("�[�x�Z���T��L���ɂł��܂���ł����B"), TEXT("���܂�̂�"), MB_OK);
    return EXIT_FAILURE;
  }

  // �[�x�Z���T�̉𑜓x
  int width, height;
  sensor.getDepthResolution(&width, &height);

  // �`��Ɏg�����b�V��
  const Mesh mesh(width, height, sensor.getCoordBuffer());

  // �`��p�̃V�F�[�_
  GgSimpleShader simple("simple.vert", "simple.frag");

  // �f�v�X�f�[�^���璸�_�ʒu���v�Z����V�F�[�_
  const Calculate position(width, height, "position.frag");

  // ���_�ʒu����@���x�N�g�����v�Z����V�F�[�_
  const Calculate normal(width, height, "normal.frag");

#if LSM_OPTION
  // �f�v�X�f�[�^����v�Z�ɕK�v�Ȓl���v�Z����V�F�[�_
  ComputeShader comp(width, height, "calc.comp");

  // lsm���v�Z����V�F�[�_
  ComputeShader lsm(width, width, "lsm.comp");
  ComputeShader lsmOutput(2, 2, "lsmOutput.comp");

  // LU�������v�Z����V�F�[�_
  ComputeShader ludep(1, 1, "ludep.comp");

  // �f�[�^�o�͗p��SSBO
  const GLint count(1);

  // SSBO �����
  //   �E�������m�ۂ� glBufferData() �ōs�������x��������s���Ă����Ȃ��ƃ_���D
  //   �E��Q������ size �͊m�ۂ��郁�����̃T�C�Y���o�C�g���Ŏw�肷��D
  //   �E�Ō�̑�S���� usage �͎g�����̃q���g�œǂݏ����֎~�Ƃ��ł͂Ȃ��D
  //   �E�Ō�̑�S���� usage �͎g�����̃q���g�œǂݏ����֎~�Ƃ��ł͂Ȃ��D
  //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBufferData.xhtml
  GLuint ssbo;
  glGenBuffers(1, &ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(DataSet), NULL, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  // �f�[�^�̊i�[��
  std::vector<GLfloat> output(count * sizeof(DataSet));

  // �v�Z�p�̃e�N�X�`��
  GLuint tex1;
  glGenTextures(1, &tex1);
  makeTex(tex1, width, height);

  GLuint tex2;
  glGenTextures(1, &tex2);
  makeTex(tex2, width, height);

  GLuint tex3;
  glGenTextures(1, &tex3);
  makeTex(tex3, width, height);

  GLuint tex4;
  glGenTextures(1, &tex4);
  makeTex(tex4, 32, 32);

  GLuint tex5;
  glGenTextures(1, &tex5);
  makeTex(tex5, 32, 32);

  GLuint tex6;
  glGenTextures(1, &tex6);
  makeTex(tex6, 32, 32);

  GLuint tex7;
  glGenTextures(1, &tex7);
  makeTex(tex7, 2, 2);

  GLuint tex8;
  glGenTextures(1, &tex8);
  makeTex(tex8, 2, 2);

  GLuint tex9;
  glGenTextures(1, &tex9);
  makeTex(tex9, 2, 2);

#endif

  // �w�i�F��ݒ肷��
  glClearColor(background[0], background[1], background[2], background[3]);

  // �B�ʏ���������L���ɂ���
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  // �E�B���h�E���J���Ă���Ԃ���Ԃ��`�悷��
  while (!window.shouldClose())
  {

#if GENERATE_POSITION
    // ���_�ʒu�̌v�Z
    position.use();
    glUniform1i(0, 0);
    glActiveTexture(GL_TEXTURE0);
    sensor.getDepth();
    const std::vector<GLuint> &positionTexture(position.calculate());
#endif

    // �@���x�N�g���̌v�Z
    normal.use();
    glUniform1i(0, 0);
    glActiveTexture(GL_TEXTURE0);
#if GENERATE_POSITION
    glBindTexture(GL_TEXTURE_2D, positionTexture[0]);
#else
    sensor.getPoint();
#endif
    const std::vector<GLuint> &normalTexture(normal.calculate());

	// LSM�̌v�Z���s��
#if LSM_OPTION 
	// computeshader���g�p����
	comp.use();

	glUniform1i(0, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, positionTexture[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindImageTexture(1, tex1, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE2);
	glBindImageTexture(2, tex2, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE3);
	glBindImageTexture(3, tex3, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

#if DATACHECK_CALC
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo);

	comp.calculate(width, height);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);
	// SSBO ����l�����o��
	//   �EglMapbuffer() �Ŏ��o�����|�C���^�� glUnmapBuffer() ����Ɩ����ɂȂ�E
	//   �EglUnmapBuffer() ����O�ɏ������I���邩�f�[�^���R�s�[���Ă����D
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	std::cout << "calc : ";
	for (int i = 0; i < 12; i++) {
		std::cout << output[i] << " - ";
	}
	std::cout << sizeof(DataSet) << std::endl;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#else
	comp.calculate(width, height);
#endif
	
	// lsm-computeshader���g�p����
	// 512-512 �� 32-32 �ɂ���
	lsm.use();
	glActiveTexture(GL_TEXTURE1);
	glBindImageTexture(1, tex1, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE2);
	glBindImageTexture(2, tex2, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE3);
	glBindImageTexture(3, tex3, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE4);
	glBindImageTexture(4, tex4, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE5);
	glBindImageTexture(5, tex5, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE6);
	glBindImageTexture(6, tex6, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

#if DATACHECK
	// �v�Z���ʂ̏o�͐�
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo);

	lsm.calculate(32, 32);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);
	//�����̎��s:���ʂ�float��SSBO�ɓ���
	// �f�[�^�̊i�[��
	//std::vector<GLfloat> output(count * sizeof(DataSet));
	// SSBO ����l�����o��
	//   �EglMapbuffer() �Ŏ��o�����|�C���^�� glUnmapBuffer() ����Ɩ����ɂȂ�E
	//   �EglUnmapBuffer() ����O�ɏ������I���邩�f�[�^���R�s�[���Ă����D
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	std::cout << "first lsm : ";
	for (int i = 0; i < 12; i++) {
		std::cout << output[i] << " ";
	}
	std::cout << std::endl;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#else
	lsm.calculate(32, 32);
#endif
	
	// ���ڂ̌v�Z 32-32 �� 2-2 �ɂ���
	lsm.use();

	glActiveTexture(GL_TEXTURE1);
	glBindImageTexture(1, tex4, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE2);
	glBindImageTexture(2, tex5, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE3);
	glBindImageTexture(3, tex6, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE4);
	glBindImageTexture(4, tex7, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE5);
	glBindImageTexture(5, tex8, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE6);
	glBindImageTexture(6, tex9, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

#if DATACHECK
	// �v�Z���ʂ̏o�͐�
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo);

	lsm.calculate(2, 2);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);
	//�����̎��s:���ʂ�float��SSBO�ɓ���
	// �f�[�^�̊i�[��
	//std::vector<GLfloat> output(count * sizeof(DataSet));
	// SSBO ����l�����o��
	//   �EglMapbuffer() �Ŏ��o�����|�C���^�� glUnmapBuffer() ����Ɩ����ɂȂ�E
	//   �EglUnmapBuffer() ����O�ɏ������I���邩�f�[�^���R�s�[���Ă����D
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	std::cout << "second lsm : ";
	for (int i = 0; i < 12; i++) {
		std::cout << output[i] << " ";
	}
	std::cout << std::endl;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#else
	lsm.calculate(2, 2);
#endif
	lsmOutput.use();
	glActiveTexture(GL_TEXTURE4);
	glBindImageTexture(1, tex7, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE5);
	glBindImageTexture(2, tex8, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE6);
	glBindImageTexture(3, tex9, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	// �v�Z���ʂ̏o�͐�
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo);

	lsmOutput.calculate(1, 1);

#if DATACHECK
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);
	// SSBO ����l�����o��
	//   �EglMapbuffer() �Ŏ��o�����|�C���^�� glUnmapBuffer() ����Ɩ����ɂȂ�E
	//   �EglUnmapBuffer() ����O�ɏ������I���邩�f�[�^���R�s�[���Ă����D
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	// �v�Z���ʂ��R�}���h���C���ɏo�͂���
	std::cout << "lsm : ";
	for (int i = 0; i < 12; i++) {
		std::cout << output[i] << " ";
	}
	std::cout << std::endl;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#endif

	// LU�������s���Ă��镔��
#if LU_DEP 
	
	ludep.use();
	// �v�Z���ʂ̏o�͐�
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo);

	ludep.calculate(1, 1);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);

	// SSBO ����l�����o��
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	// �v�Z���ʂ��R�}���h���C���ɏo�͂���
	std::cout << "x = " << output[0] << ", y = " << output[1] << ", z = " << output[2] << std::endl;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

#endif

#endif

	// ��ʏ���
    window.clear();

    // �`��p�̃V�F�[�_�v���O�����̎g�p�J�n
    simple.use();
    simple.loadMatrix(window.getMp(), window.getMw());
    simple.setLight(light);
    simple.setMaterial(material);

#if GENERATE_POSITION
    // ���_�ʒu�e�N�X�`��
    glUniform1i(0, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, positionTexture[0]);
#endif

    // ���_�@���e�N�X�`��
    glUniform1i(1, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture[0]);

    // �J���[�e�N�X�`��
    glUniform1i(2, 2);
    glActiveTexture(GL_TEXTURE2);
    sensor.getColor();

    // �}�`�`��
    mesh.draw();

    // �o�b�t�@�����ւ���
    window.swapBuffers();
  }
}
