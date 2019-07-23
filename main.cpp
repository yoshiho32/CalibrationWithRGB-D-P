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

// �听������(Principal Component Analysis)�ŕ��ʂ��o���Ȃ� 1�@��������
#define PCA_OPTION 0

// �ŏ����@(Least Squares Method)�ŕ��ʂ��o���Ȃ� 1
#define LSM_OPTION 1

// �f�v�X�Z���T�[�̃f�[�^�͈͂̎w��
#define NEAR_RENGE 100
#define FAR_RENGE 4000

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

  // OpenGL Version 3.2 Core Profile ��I������
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
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


#if PCA_OPTION
  // �f�v�X�J�����őΏە��ʂ��听������
  ComputeShader pca(width, height, "pca.comp");
#endif

#if LSM_OPTION
  // �f�v�X�J�����őΏە��ʂ��ŏ����@�Ōv�Z
  ComputeShader calc_xyz(width, height, "calc_xyz.comp");
  ComputeShader lsm(width, height, "lsm.comp");

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
	  //pca�ŕ��ʐ���@�����쐬
#if PCA_OPTION 
	  pca.use();
#endif

#if LSM_OPTION
	  calc_xyz.use();
	  // depth�f�[�^�̑��M
	  glUniform1i(0, 0);
	  glActiveTexture(GL_TEXTURE0);
	  sensor.getDepth();

	  // �������ʂ̕ۑ�
	  // x^2 �Ɓ@y^2 �̕ۑ�
	  glActiveTexture(GL_TEXTURE1);
	  glBindImageTexture(1, calc_xyz.tex_A, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	  // xy yz xz �̕ۑ�
	  glActiveTexture(GL_TEXTURE2);
	  glBindImageTexture(2, calc_xyz.tex_B, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	  calc_xyz.calculate();

	  lsm.use();
	  
	  glActiveTexture(GL_TEXTURE0);
	  glBindImageTexture(0, calc_xyz.tex_C, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	  glActiveTexture(GL_TEXTURE1);
	  glBindImageTexture(1, calc_xyz.tex_A, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	  glActiveTexture(GL_TEXTURE2);
	  glBindImageTexture(2, calc_xyz.tex_B, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	  lsm.use();

#endif

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
