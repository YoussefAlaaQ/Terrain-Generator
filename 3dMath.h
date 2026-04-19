// will implement my own math later
#include <math.h>

#define PI 3.14159265358979323846

struct v2{
	float x;
	float y;
};

struct v3{
	float x;
	float y;
	float z;
};

struct v4{
	float x;
	float y;
	float z;
	float w;
};

struct mat4{
	float m[4][4];
};

inline v2 make_vec2(float x, float y);
inline v3 make_vec3(float x, float y, float z);
inline v4 make_vec3(float x, float y, float z, float w);

inline mat4 mat4_identity();
inline mat4 mat4_multiply(mat4 m1, mat4 m2);
inline mat4 mat4_translate(v3 v);
inline mat4 mat4_Scale(float x, float y, float z);
inline mat4 mat4_rotate(v3 axis, float radians);
inline v4 v4_multiply_mat4(v4 vec, mat4 mat);

inline float square(float x);
inline float radians(float degrees);
