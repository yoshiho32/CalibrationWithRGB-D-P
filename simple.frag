#version 430 core

// �e�N�X�`��
layout (location = 2) uniform sampler2D color;      // �J���[�̃e�N�X�`��
layout (rgba32f, binding = 3) uniform readonly image2D depth; // �I�u�W�F�N�g�̍��W

// ���X�^���C�U����󂯎�钸�_�����̕�Ԓl
in vec4 idiff;                                      // �g�U���ˌ����x
in vec4 ispec;                                      // ���ʔ��ˌ����x
in vec2 texcoord;                                   // �e�N�X�`�����W

// �t���[���o�b�t�@�ɏo�͂���f�[�^
layout (location = 0) out vec4 fc;                  // �t���O�����g�̐F


void main(void)
{
  // �f�v�X���L�����ǂ����Ńe�N�X�`���̐F��ς���
  ivec2 tex = ivec2(texcoord.x * 512, texcoord.y * 424);

  vec4 p = imageLoad(depth, tex);

  // �e�N�X�`���}�b�s���O���s���ĉA�e�����߂�
  //fc = idiff + ispec;
  fc = texture(color , texcoord)*p.w;
  //fc = vec4(1 - texcoord.x * 256 / 512, 1 - texcoord.y * 256 / 424, 0, 1) * step(ispec.x, 0.9);//texture(color, texcoord);// * idiff + ispec;
}

