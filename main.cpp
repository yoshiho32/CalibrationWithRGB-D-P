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

// ssbo�̃f�[�^�Z�b�g
struct DataSet {
	GLfloat data_xyz[4];
	GLfloat data_xy2[4];
	GLfloat data_xyzx[4];
};

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

  // OpenGL Version 3.2 Core Profile ��I������
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  

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

  ComputeShader comp(width, height, "calc.comp");

  const GLint count(10);

  // SSBO �����
  //   �E�������m�ۂ� glBufferData() �ōs�������x��������s���Ă����Ȃ��ƃ_���D
  //   �E��Q������ size �͊m�ۂ��郁�����̃T�C�Y���o�C�g���Ŏw�肷��D
  //   �E�Ō�̑�S���� usage �͎g�����̃q���g�œǂݏ����֎~�Ƃ��ł͂Ȃ��D
  //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBufferData.xhtml
  GLuint ssbo;
  glGenBuffers(1, &ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(DataSet), NULL, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


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
	
	comp.use();

	glUniform1i(0, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, positionTexture[0]);

	// �v�Z���ʂ̏o�͐�
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo);

	comp.calculate(width, height);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, 0);
	//�����̎��s:���ʂ�float��SSBO�ɓ���
	// �f�[�^�̊i�[��
	std::vector<GLfloat> output(count * sizeof(DataSet));
	// SSBO ����l�����o��
	//   �EglMapbuffer() �Ŏ��o�����|�C���^�� glUnmapBuffer() ����Ɩ����ɂȂ�E
	//   �EglUnmapBuffer() ����O�ɏ������I���邩�f�[�^���R�s�[���Ă����D
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	for (int i = 0; i < 12; i++) {
		std::cout << output[i] << " - ";
	}
	std::cout << sizeof(DataSet) << std::endl;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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
