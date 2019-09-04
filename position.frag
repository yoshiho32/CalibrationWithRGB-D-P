#version 430 core

#define MILLIMETER 0.001
#define DEPTH_SCALE (-65535.0 * MILLIMETER)
#define DEPTH_MAXIMUM (-10.0)

#define FAR  300
#define NEAR 20

// スケール
const vec2 scale = vec2(
  1.546592,
  1.222434
);

// テクスチャ
layout (location = 0) uniform sampler2D depth;

layout (location = 4) uniform sampler2D depth2;

// テクスチャ座標
in vec2 texcoord;

// フレームバッファに出力するデータ
layout (location = 0) out vec3 position;

struct DataSet{
  vec4 data_xyz;
  vec4 data_xy2;
  vec4 data_xyzx;
 };

// 計算結果の出力
layout (std430, binding = 7) buffer Input{
  DataSet data;
};

// デプス値をスケーリングする
float s(in float z)
{
  return z == 0.0 ? DEPTH_MAXIMUM : z * DEPTH_SCALE;
}

void main(void)
{
  // デプス値を取り出す
  //float z = s(texture(depth, texcoord).r);
  float z = s(texture(depth2, texcoord).r);

  vec2 place = (texcoord - 0.5) * scale * z;

  float zy = data.data_xyz[0] + place.x * data.data_xyz[1] + place.y * data.data_xyz[2];

  // デプス値からカメラ座標値を求める // デプスデータの有効範囲を決めて描画
  position = vec3(place, z);// * step(-FAR, -z *  DEPTH_SCALE) * step (NEAR, z * DEPTH_SCALE), z);
}
