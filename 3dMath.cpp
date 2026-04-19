#include "3dMath.h"

//TODO: implement vector normalizing

inline float
square(float x){
	return x * x;
}

inline float
radians(float degrees){
	float rad = degrees * (PI / 180.0f);
	
	return rad;
}

inline v2
make_vec2(float x, float y){
	v2 result = {0};
	result.x = x;
	result.y = y;
	return result;
}

inline v3
make_vec3(float x, float y, float z){
	v3 result = {0};
	result.x = x;
	result.y = y;
	result.z = z;
	return result;
}

inline v3
make_vec3(float xyz){
	v3 result = {xyz};
	// result.x = xyz;
	// result.y = xyz;
	// result.z = xyz;
	return result;
}



inline v4
make_vec4(float x, float y, float z, float w){
	v4 result = {0};
	result.x = x;
	result.y = y;
	result.z = z;
	result.w = w;
	return result;
}

inline v4
make_vec4(float xyzw){
	v4 result = {0};
	result.x = xyzw;
	result.y = xyzw;
	result.z = xyzw;
	result.w = xyzw;
	return result;
}

inline v3 
v3_cross(v3 vec1, v3 vec2){
	v3 result = {};
	result.x = vec1.y * vec2.z - vec1.z * vec2.y;
	result.y = vec1.z * vec2.x - vec1.x * vec2.z;
	result.z = vec1.x * vec2.y - vec1.y * vec2.x;
	
	return result;
}

inline v3 
v3_subtract(v3 vec1, v3 vec2){
	v3 result = {};
	result.x = vec1.x - vec2.x;
	result.y = vec1.y - vec2.y;
	result.z = vec1.z - vec2.z;
	
	return result;
}

inline v3 
v3_add(v3 vec1, v3 vec2){
	v3 result = {};
	result.x = vec1.x + vec2.x;
	result.y = vec1.y + vec2.y;
	result.z = vec1.z + vec2.z;
	
	return result;
}

inline v3 
v3_scale(v3 v, float scale){
	v3 result = make_vec3(v.x * scale, v.y * scale, v.z * scale);
	return result;
}

inline v3
v3_divide(v3 v, float dividor){
	v3 result = {};
	result.x = v.x / dividor;
	result.y = v.y / dividor;
	result.z = v.z / dividor;

	return result;
}

inline v3
v3_component_multiply(v3 vec1, v3 vec2){
	return make_vec3(vec1.x * vec2.x, vec1.y * vec2.y, vec1.z * vec2.z);
}

inline float
v3_length(v3 vec){
	float result = sqrt(square(vec.x) + square(vec.y) + square(vec.z));
	return result;
}

inline v3 
operator*(v3 v, float scale){
	return v3_scale(v, scale);
}

inline v3 
operator*(v3 vec1, v3 vec2){
	return v3_component_multiply(vec1, vec2);
}

inline v3
operator/(v3 v, float dividor){
	return v3_divide(v, dividor);
}

v3 
operator + (v3 vec1, v3 vec2){
	return v3_add(vec1, vec2);
}

v3 
operator - (v3 vec1, v3 vec2){
	return v3_subtract(vec1, vec2);
}

v3&
operator += (v3& vec1, const v3& vec2){
	vec1 = v3_add(vec1, vec2);
    return vec1;
}

v3&
operator -= (v3& vec1, const v3& vec2){
	vec1 = v3_subtract(vec1, vec2);
    return vec1;
}

inline mat4
mat4_identity(){
	mat4 result = {0};
	result.m[0][0] = 1.0f; 
	result.m[1][1] = 1.0f; 
	result.m[2][2] = 1.0f; 
	result.m[3][3] = 1.0f;
	
	return result; 
}

inline mat4
mat4_multiply(mat4 m1, mat4 m2){
	mat4 result = {0};
	
	for(int row = 0; row < 4; ++row){
		for(int col = 0; col < 4; ++col){
			for(int k = 0; k < 4; ++k){
				result.m[row][col] += m1.m[row][k] * m2.m[k][col]; 
			}
		}
	}
	
	return result;
}

inline void
v3_normalize(v3* vec){
	//todo: see if i can optimize the power and sqrt
	float length = sqrtf(vec->x * vec->x +
                         vec->y * vec->y +
                         vec->z * vec->z);
    if (length != 0.0f){
        vec->x /= length;
        vec->y /= length;
        vec->z /= length;
    } 
}

inline v3
v3_normalize(v3 vec){
	//todo: see if i can optimize the power and sqrt
	v3 normalized = {};
	float length = sqrtf(vec.x * vec.x +
                         vec.y * vec.y +
                         vec.z * vec.z);
    if (length != 0.0f){
        normalized.x = vec.x / length;
        normalized.y = vec.y / length;
        normalized.z = vec.z / length;
		
		return normalized;
    }
    
    return vec;
}

inline mat4
mat4_translate(v3 v){
	mat4 result = mat4_identity();
	result.m[3][0] = v.x;
	result.m[3][1] = v.y;
	result.m[3][2] = v.z;
	
	return result;
}

inline mat4
mat4_Scale(float x, float y, float z){
	mat4 result = mat4_identity();
	result.m[0][0] = x;
	result.m[1][1] = y;
	result.m[2][2] = z;
	
	return result;
}

inline mat4
mat4_Scale(v3 scaler){
	mat4 result = mat4_identity();
	result.m[0][0] = scaler.x;
	result.m[1][1] = scaler.y;
	result.m[2][2] = scaler.z;
	
	return result;
}

inline bool 
v3_isNormalized(v3 vec){
	float lengthSq = vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
	
	const float epsilon = 1e-6f;
	
	//normaly we will check if lengthSq == 1 but the vectorlength may be very close to 1 ex: 0.99999998
	//we will handle it as a normalized vector 
	if (fabsf(lengthSq - 1.0f) > epsilon) {
		return false;
	}
	return true;
}

//this is implementation of Rodrigue's Rotation Formula
//i really don't understand this
//study: rodrigue's rotation Formula

inline mat4
mat4_rotate(v3 axis, float radians){
	if(!v3_isNormalized(axis)){
		v3_normalize(&axis);
	}
 
    mat4 Result = mat4_identity();
    float c = cosf(radians);
    float s = sinf(radians);
    float oneMinusC = 1.0f - c;

    float x = axis.x;
    float y = axis.y;
    float z = axis.z;

    Result.m[0][0] = c + oneMinusC * x * x;
    Result.m[0][1] = oneMinusC * x * y + s * z;
    Result.m[0][2] = oneMinusC * x * z - s * y;

    Result.m[1][0] = oneMinusC * y * x - s * z;
    Result.m[1][1] = c + oneMinusC * y * y;
    Result.m[1][2] = oneMinusC * y * z + s * x;

    Result.m[2][0] = oneMinusC * z * x + s * y;
    Result.m[2][1] = oneMinusC * z * y - s * x;
    Result.m[2][2] = c + oneMinusC * z * z;

    return Result;
}

inline v4
v4_multiply_mat4(v4 vec, mat4 mat){
	v4 result = {0};
	result.x = vec.x * mat.m[0][0] + vec.y * mat.m[0][1] + vec.z * mat.m[0][2] + vec.w * mat.m[0][3];
    result.y = vec.x * mat.m[1][0] + vec.y * mat.m[1][1] + vec.z * mat.m[1][2] + vec.w * mat.m[1][3];
    result.z = vec.x * mat.m[2][0] + vec.y * mat.m[2][1] + vec.z * mat.m[2][2] + vec.w * mat.m[2][3];
    result.w = vec.x * mat.m[3][0] + vec.y * mat.m[3][1] + vec.z * mat.m[3][2] + vec.w * mat.m[3][3];
    
	return result;
}

mat4
operator * (mat4 m1, mat4 m2){
	return mat4_multiply(m1, m2);
}

mat4&
operator *= (mat4& m1, const mat4& m2){
	m1 = mat4_multiply(m1, m2);
    return m1;
}

//NOTE: this implementation will only work for a right handed system meaning
//it will give wrong result for something like directx or vulken  
inline mat4
mat4_perspective(float fov, float aspectRatio, float nearPlane, float far){
	mat4 result = {0};
	
	float tanHalfView = tanf(fov / 2);
	float top = nearPlane * tanHalfView;
	float right = top * aspectRatio;
	result.m[0][0] = nearPlane / right;
	result.m[1][1] = nearPlane / top;
	result.m[2][2] = -1;
	
	result.m[2][3] = -1;
	result.m[3][2] = -2 * nearPlane;
		
	return result;
}

mat4 
mat4_lookAt(v3 camPos, v3 target, v3 up){
	mat4 lookAt = mat4_identity();
	
	v3 camDirection = v3_normalize(camPos - target);
	v3 camRight = v3_normalize(v3_cross(up, camDirection));
	v3 camUp = {};
	camUp = v3_cross(camDirection, camRight);
	
	mat4 rotate = {};
	mat4 translate = mat4_identity();
	translate = translate * mat4_translate(make_vec3(-camPos.x, -camPos.y, -camPos.z));
	
	
	rotate.m[0][0] = camRight.x;
	rotate.m[0][1] = camUp.x;
	rotate.m[0][2] = camDirection.x;
	rotate.m[0][3] = 0;
	
	rotate.m[1][0] = camRight.y;
	rotate.m[1][1] = camUp.y;
	rotate.m[1][2] = camDirection.y;
	rotate.m[1][3] = 0;
	
	rotate.m[2][0] = camRight.z;
	rotate.m[2][1] = camUp.z;
	rotate.m[2][2] = camDirection.z;
	rotate.m[2][3] = 0;
	
	 
	rotate.m[3][0] = 0;
	rotate.m[3][1] = 0;
	rotate.m[3][2] = 0;
	rotate.m[3][3] = 1;
	
	lookAt = translate * rotate;
	return lookAt;
}

