#version 430 core

#define MILLIMETER 0.001
#define DEPTH_SCALE (-65535.0 * MILLIMETER)
#define DEPTH_MAXIMUM (-10.0)

// スケール
const vec2 scale = vec2(
  1.546592,
  1.222434
);

// テクスチャ
layout (rgba32f, binding = 0) uniform readonly image2D depth;

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
  return z == 0.0 ? DEPTH_MAXIMUM : z;
}

void main(void)
{
  // カラーのテクスチャ座標(0,0)-(1,1)をimage2Dのテクスチャ座標(0,0)-(512,424)に置き換える(globalInvocationIDがないので)
  ivec2 itexcoord = ivec2(texcoord * ivec2(512, 424));
  // デプスデータの取得
  vec4 pos = imageLoad(depth, itexcoord);

  // カメラ座標での位置
  vec2 tex = texcoord - 0.5;

  // 最小2乗平面におけるzの式
  // z = a + bx + cy 
  float zy =  data.data_xyz[0] + tex.x * data.data_xyz[1] + tex.y * data.data_xyz[2];////(data.data_xyz[0] * tex.x + data.data_xyz[1] * tex.y + data.data_xyz[3]);
  
  // O(0,0,0)を通るならこれでいい、普通の式。今回はOを通るかわからないのでダメ
  // z = -(a'x + b'y) / c'
  //float zy = -( data.data_xyz[0] * tex.x + data.data_xyz[1] * tex.y ) / data.data_xyz[2];

  // デプス値からカメラ座標値を求める 
  position = pos.z==0 ? vec3(0,0,0) : vec3(-tex*scale, zy);

}

