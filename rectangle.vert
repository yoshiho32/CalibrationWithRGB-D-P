#version 430 core

// ���_���W
layout (location = 0) in vec4 pv;

// �e�N�X�`�����W
out vec2 texcoord;

void main()
{
  // ���_���W���e�N�X�`�����W�Ɋ��Z
  texcoord = pv.xy * 0.5 + 0.5;

  // ���_���W�����̂܂܏o��
  gl_Position = pv;
}
