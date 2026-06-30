#include <math.h>

#define PI 3.14159265358979323846

struct Rect{
	float x, y, w, h;
};

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


struct mat2{
	float m[2][2];
};


struct mat3{
	float m[3][3];
};


struct mat4{
	float m[4][4];
};

inline float flipY(float posY, float height);

inline float clamp_float(float value, float min_val, float max_val);

inline internal float ConvertToScreenSpace(float cameraDim, float targetDim);
inline internal float flipYInMeters(float posYm, float heightm);

inline internal bool32 point_in_box(v2 point, SDL_FRect box);

internal bool checkCollision(SDL_FRect a, SDL_FRect b);

inline float square(float x);
inline float radians(float degrees);
inline v2 make_vec2(float x, float y);
inline v3 make_vec3(float x, float y, float z);
inline v3 make_vec3(float xyz);
inline v4 make_vec4(float x, float y, float z, float w);
inline v4 make_vec4(float xyzw);
inline v3 v3_cross(v3 vec1, v3 vec2);
inline v3 v3_subtract(v3 vec1, v3 vec2);
inline v3 v3_add(v3 vec1, v3 vec2);
inline v3 v3_scale(v3 v, float scale);
inline v3 v3_divide(v3 v, float dividor);
inline v3 v3_component_multiply(v3 vec1, v3 vec2);
inline float v3_length(v3 vec);
inline bool v3_isNormalized(v3 vec);
inline void v3_normalize(v3* vec);
inline v3 v3_normalize(v3 vec);
inline v3 operator*(v3 v, float scale);
inline v3 operator*(v3 vec1, v3 vec2);
inline v3 operator/(v3 v, float dividor);
v3 operator + (v3 vec1, v3 vec2);
v3 operator - (v3 vec1, v3 vec2);
v3& operator += (v3& vec1, const v3& vec2);
v3& operator -= (v3& vec1, const v3& vec2);

inline mat4 mat4_identity();
inline mat4 mat4_multiply(mat4 m1, mat4 m2);
inline mat4 mat4_translate(v3 v);
inline mat4 mat4_translate(float x, float y, float z);
inline mat4 mat4_scale(float x, float y, float z);
inline mat4 mat4_scale(v3 scaler);
inline mat4 mat4_rotate(v3 axis, float radians);
inline v4 v4_multiply_mat4(v4 vec, mat4 mat);
mat4 operator * (mat4 m1, mat4 m2);
mat4& operator *= (mat4& m1, const mat4& m2);
inline mat4 mat4_perspective(float fov, float aspectRatio, float nearPlane, float far);
inline mat4 mat4_orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
mat4 mat4_lookAt(v3 camPos, v3 target, v3 up);
inline mat3 mat4_to_mat3(mat4 m);
inline mat4 mat3_to_mat4(mat3 m);




