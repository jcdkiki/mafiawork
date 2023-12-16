#include "exmath.h"
#include <math.h>

mat4 *mat4_mul(mat4 *res, mat4 *a, mat4 *b)
{
	for (int j = 0; j < 4; j++)
		for (int i = 0; i < 4; i++)
			res->v[j*4 + i] = a->v[j*4]*b->v[i] + a->v[j*4+1]*b->v[i+4] + a->v[j*4+2]*b->v[i+8] + a->v[j*4+3]*b->v[i+12];
		
	return res;
}

mat4 *mat4_perspective(mat4 *res, float fov, float W, float H, float N, float F) {
    float f  = 1.0f / tanf(fov);
	
	*res = (mat4) { 0 };
	res->v[ 0] = f / W * H;
	res->v[ 5] = f;
	res->v[10] = -F / (F - N);
	res->v[11] = -1.0f;
	res->v[14] = (-N * F) / (F - N);

	return res;
}

mat4 *mat4_rotation_x(mat4 *res, float ang) {
	float c = cosf(ang);
	float s = sinf(ang);

	*res = (mat4) { 0 };
	res->v[0] = 1.f; res->v[5] = c;
	res->v[6] = -s;  res->v[9] = s;
	res->v[10] = c;  res->v[15] = 1.f;
	
	return res;
}

mat4 *mat4_rotation_y(mat4 *res, float ang) {
	float c = cosf(ang);
	float s = sinf(ang);

	*res = (mat4) { 0 };
	res->v[5] = 1;  res->v[15] = 1;
	res->v[0] = c;  res->v[2] = s;
	res->v[8] = -s; res->v[10] = c;

	return res;
}

mat4 *mat4_rotation_z(mat4 *res, float ang) {
	float c = cosf(ang);
	float s = sinf(ang);

	*res = (mat4) { 0 };
	res->v[10] = 1; res->v[15] = 1;
	res->v[0] = c;  res->v[1] = -s;
	res->v[4] = s;  res->v[5] = c;
	
	return res;
}

mat4 *mat4_scaling(mat4 *res, vec3f a)
{
	*res = (mat4) { 0 };
	res->v[0] = a.x; res->v[5] = a.y;
	res->v[10] = a.z; res->v[15] = 1;

	return res;
}

mat4 *mat4_translation(mat4 *res, vec3f a)
{
	*res = (mat4) { 0 };
	res->v[0] = res->v[5] = res->v[10] = res->v[15] = 1;
	res->v[3] = a.x;
	res->v[7] = a.y;
	res->v[11] = a.z;

	return res;
}

vec3f vec3f_apply_mat4(vec3f a, mat4 *b)
{
	float w = a.x*b->v[12] + a.y*b->v[13] + a.z*b->v[14] + 1*b->v[15];

	return VEC3F(
		(a.x*b->v[0] + a.y*b->v[1] + a.z*b->v[2] + 1*b->v[3]) / w,
		(a.x*b->v[4] + a.y*b->v[5] + a.z*b->v[6] + 1*b->v[7]) / w,
		(a.x*b->v[8] + a.y*b->v[9] + a.z*b->v[10] + 1*b->v[11]) / w
	);
}

mat4 *mat4_ortho_projection(mat4 *res, float left, float right, float top, float bottom, float zn, float zf)
{
	res->v[0] = 2.f / (right - left);
	res->v[4] = res->v[8] = res->v[12] = 0;

	res->v[1] = res->v[9] = res->v[13] = 0;
	res->v[5] = 2.f / (top - bottom);
	
	res->v[2] = res->v[6] = res->v[14] = 0;
	res->v[10] = 2.f / (zf - zn);

	res->v[3] = -(right+left)/(right-left);
	res->v[7] = -(top+bottom)/(top-bottom);
	res->v[11] = -(zf+zn)/(zf-zn);
	res->v[15] = 1.f;

	return res;
}