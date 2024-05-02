#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include <sys/ioctl.h>

#include "main.h"

struct winsize term_size;
char* framebuf;
double rect1[8 * 3] = {
//a1=(1,-1,-1)
	1.0, -1.0, -1.0,
//b1=(1,1,-1)
	1.0, 1.0, -1.0,
//c1=(-1,1,-1)
	-1.0, 1.0, -1.0,
//d1=(-1,-1,-1)
	-1.0, -1.0, -1.0,
//a2=(1,-1,1)
	1.0, -1.0, 1.0,
//b2=(1,1,1)
	1.0, 1.0, 1.0,
//c2=(-1,1,1)
	-1.0, 1.0, 1.0,
//d2=(-1,-1,1)
	-1.0, -1.0, 1.0,
};

double airplane1[10 * 3] = {
//a=(2.5,0,0)
	2.5, 0.0, 0.0,
//b=(-1.5,0,0)
	-1.5, 0.0, 0.0,
//c=(-2.5,0,0)
	-2.5, 0.0, 0.0,
//d=(3,1,0)
	3.0, 1.0, 0.0,
//e=(3,-1,0)
	3.0, -1.0, 0.0,
//f=(1,4,0)
	1.0, 4.0, 0.0,
//g=(1,-4,0)
	1.0, -4.0, 0.0,
//h=(-1,4,0)
	-1.0, 4.0, 0.0,
//i=(-1,-4,0)
	-1.0, -4.0, 0.0,
//j=(-2.5,0,1)
	-2.5, 0.0, 1.0,
};

double dot_product(double *v1, double *v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void cross_product(double *v1, double *v2, double *result)
{
	result[0] = v1[1]*v2[2] - v1[2]*v2[1];
	result[1] = v1[2]*v2[0] - v1[0]*v2[2];
	result[2] = v1[0]*v2[1] - v1[1]*v2[0];
	return;
}

void vec_sub(double *v1, double *v2, double *result)
{
	result[0] = v1[0] - v2[0];
	result[1] = v1[1] - v2[1];
	result[2] = v1[2] - v2[2];
	return;
}

void vec_add(double *v1, double *v2, double *result)
{
	result[0] = v1[0] + v2[0];
	result[1] = v1[1] + v2[1];
	result[2] = v1[2] + v2[2];
	return;
}

void matrix_mul(double *left, double *right, double *ret)
{
//	left:3cx3r right:1cx3r ret:1cx3r
	ret[0] = left[0*3 + 0]*right[0] + left[1*3 + 0]*right[1] + left[2*3 + 0]*right[2];
	ret[1] = left[0*3 + 1]*right[0] + left[1*3 + 1]*right[1] + left[2*3 + 1]*right[2];
	ret[2] = left[0*3 + 2]*right[0] + left[1*3 + 2]*right[1] + left[2*3 + 2]*right[2];
}

void drw_dot(int x, int y, char *framebuf, struct winsize* sz)
{
	if(x >= 0 && x < sz->ws_col && y >= 0 && y < sz->ws_row) {
		framebuf[(sz->ws_row - 1 - y) * sz->ws_col + x] = '#';
	}
	return;
}

void drw_line(int *p1, int *p2, char *framebuf, struct winsize* sz)
{
	int A[2];
	int B[2];
	int steep = abs(p2[1]-p1[1]) > abs(p2[0]-p1[0]);

//	printf("[%d, %d] [%d, %d]\n", p1[0], p1[1], p2[0], p2[1]);

	if (steep) {
		A[0]=p1[1];
		A[1]=p1[0];
		B[0]=p2[1];
		B[1]=p2[0];
	} else {
		A[0]=p1[0];
		A[1]=p1[1];
		B[0]=p2[0];
		B[1]=p2[1];
	}
	if (A[0] > B[0]) {//保证xA <= xB
		int temp;
		temp = A[0];
		A[0] = B[0];
		B[0] = temp;
		temp = A[1];
		A[1] = B[1];
		B[1] = temp;
	}
	int dx = B[0] - A[0];
	int dy = abs(B[1] - A[1]);
	int error = dx / 2;
	int ystep = (A[1] < B[1]) ? 1 : -1;
	int y = A[1];

	for (int x = A[0]; x <= B[0]; x++) {
		if (steep) {
			drw_dot(y, x, framebuf, sz);
		} else {
			drw_dot(x, y, framebuf, sz);
		}
		error = error - dy;
		if (error < 0) {
			y = y + ystep;
			error = error + dx;
		}
	}
	return;
}
/*
void fill_triangle(int *p1, int *p2, int *p3, char *framebuf, struct winsize* sz)
{
	int temp[2];//三个点按照y坐标排序
	if (p1[1] > p2[1]) {
		temp[0] = p1[0];
		temp[1] = p1[1];
		p1[0] = p2[0];
		p1[1] = p2[1];
		p2[0] = temp[0];
		p2[1] = temp[1];
	}
	if (p2[1] > p3[1]) {
		temp[0] = p2[0];
		temp[1] = p2[1];
		p2[0] = p3[0];
		p2[1] = p3[1];
		p3[0] = temp[0];
		p3[1] = temp[1];
	}
	if (p1[1] > p2[1]) {
		temp[0] = p1[0];
		temp[1] = p1[1];
		p1[0] = p2[0];
		p1[1] = p2[1];
		p2[0] = temp[0];
		p2[1] = temp[1];
	}

	for (int y = 0; y <= p2[1]-p1[1]; y++) {//dy1, dy2都不为零
		int left_p[2];
		int right_p[2];
		if ((p2[0]-p1[0])*(p3[1]-p1[1]) > (p3[0]-p1[0])*(p2[1]-p1[0])) {
		} else {
		}
		for (int x = ; x <= ; x++) {
			
		}
	}

	for (int i = p2[0]>p3[0] ? p3[0] : p2[0]; i <= p2[0]>p3[0] ? p2[0] : p3[0]; i++) {
		
	}
	return;
}
*/
int render_point(double *point, int *rendered, struct winsize *sz, struct camera *cam)
{
	double cam_dir[3];//指向相机镜头方向的单位向量
	double dist;//点离相机平面的距离
	double vec[3];
	vec_sub(point, cam->position, vec);
	cross_product(cam->vec_i, cam->vec_j, cam_dir);
	dist = -dot_product(cam_dir, vec);//因为平面内x，y轴单位向量叉乘后指向后方，所以加负号
	if (dist <= 0)//如果点在摄像机后面，渲染失败
		return -1;

	rendered[0] = cam->focal*dot_product(vec, cam->vec_i)/dist + sz->ws_col/2;
	rendered[1] = cam->focal*dot_product(vec, cam->vec_j)/dist + sz->ws_row/2;

	if (rendered[0] >= sz->ws_col + 10 || rendered[1] >= sz->ws_row + 10 || rendered[0] < -10 || rendered[1] < -10) {//超出边界太多就抛弃
		return -1;
	}

	return 0;
}

void render_line(double *p1, double *p2, char *framebuf, struct winsize *sz, struct camera *cam)
{
	int ret1[2];
	int ret2[2];
	int err;

	err = render_point(p1, ret1, sz, cam);
	if (err < 0)//如果点在摄像机后面就放弃
		return;
	err = render_point(p2, ret2, sz, cam);
	if (err < 0)
		return;

	drw_line(ret1, ret2, framebuf, sz);

	return;
}

//模型特殊代码区域
//**********************************************************************************************************************************8
void render_rect(double *rect, char *framebuf, struct winsize* sz)
{
	int rendered_rect[2 * 8];
	struct camera cam = {
		.focal = 60.0,
		.position = {5.0, 0.0, 0.0},
		.vec_i = {0.0, 1.0, 0.0},
		.vec_j = {0.0, 0.0, 1.0},
	};
//渲染正方体12条棱
	for (int i = 0; i < 8; i++) {
		render_point(&rect[i*3], &rendered_rect[i*2], sz, &cam);
	}

	drw_line(&rendered_rect[0*2], &rendered_rect[1*2], framebuf, sz);
	drw_line(&rendered_rect[1*2], &rendered_rect[2*2], framebuf, sz);
	drw_line(&rendered_rect[2*2], &rendered_rect[3*2], framebuf, sz);
	drw_line(&rendered_rect[3*2], &rendered_rect[0*2], framebuf, sz);

	drw_line(&rendered_rect[4*2], &rendered_rect[5*2], framebuf, sz);
	drw_line(&rendered_rect[5*2], &rendered_rect[6*2], framebuf, sz);
	drw_line(&rendered_rect[6*2], &rendered_rect[7*2], framebuf, sz);
	drw_line(&rendered_rect[7*2], &rendered_rect[4*2], framebuf, sz);

	drw_line(&rendered_rect[0*2], &rendered_rect[4*2], framebuf, sz);
	drw_line(&rendered_rect[1*2], &rendered_rect[5*2], framebuf, sz);
	drw_line(&rendered_rect[2*2], &rendered_rect[6*2], framebuf, sz);
	drw_line(&rendered_rect[3*2], &rendered_rect[7*2], framebuf, sz);

	return;
}

void render_airplane(double *airplane, char *framebuf, struct winsize* sz)
{
	int rendered_airplane[2 * 10];
	struct camera cam = {
		.focal = 150.0,
		.position = {8.0, 0.0, 2.0},
		.vec_i = {0.0, 1.0, 0.0},
		.vec_j = {-0.258819, 0.0, 0.965925},
	};


//渲染飞机10个点
	for (int i = 0; i < 10; i++) {
		render_point(&airplane[i*3], &rendered_airplane[i*2], sz, &cam);
	}

//连ed
//	drw_line(&rendered_airplane[3*2], &rendered_airplane[4*2], framebuf, sz);
//连ac
	drw_line(&rendered_airplane[0*2], &rendered_airplane[2*2], framebuf, sz);
//连gf
	drw_line(&rendered_airplane[5*2], &rendered_airplane[6*2], framebuf, sz);
//连hi
	drw_line(&rendered_airplane[7*2], &rendered_airplane[8*2], framebuf, sz);
//连gi
	drw_line(&rendered_airplane[6*2], &rendered_airplane[8*2], framebuf, sz);
//连fh
	drw_line(&rendered_airplane[5*2], &rendered_airplane[7*2], framebuf, sz);
//连jb
	drw_line(&rendered_airplane[9*2], &rendered_airplane[1*2], framebuf, sz);
//连jc
	drw_line(&rendered_airplane[9*2], &rendered_airplane[2*2], framebuf, sz);

	return;
}

void rotate(double *points, double *new_points, int num, double yaw, double pitch, double roll)
{
	double rotate_matrix[9] = {
		cos(yaw)*cos(pitch), sin(yaw)*cos(pitch), -sin(pitch),
		cos(yaw)*sin(pitch)*sin(roll)-sin(yaw)*cos(roll), sin(yaw)*sin(pitch)*sin(roll)+cos(yaw)*cos(roll), cos(pitch)*sin(roll),
		cos(yaw)*sin(pitch)*cos(roll)+sin(yaw)*sin(roll), sin(yaw)*sin(pitch)*cos(roll)-cos(yaw)*sin(roll), cos(pitch)*cos(roll)
	};

	for (int i=0; i < num; i++) {
		matrix_mul(rotate_matrix, &points[i*3], &new_points[i*3]);
	}
	return;
}

void draw(int sig)
{
	static double yaw = 0;
	static double pitch = 0;
	static double roll = 0;
//	double rect1_rotated[8*3];
	double airplane1_rotated[10*3];
	memset(framebuf, ' ', term_size.ws_row * term_size.ws_col);
//	rotate(rect1, rect1_rotated, 8, yaw, pitch, roll);
	rotate(airplane1, airplane1_rotated, 10, yaw, pitch, roll);
//	render_rect(rect1_rotated, framebuf, &term_size);
	render_airplane(airplane1_rotated, framebuf, &term_size);
	write(STDOUT_FILENO, framebuf, term_size.ws_row * term_size.ws_col);

	yaw = yaw + 0.1;
	if (yaw >= 2*M_PI) {
		yaw = yaw - 2*M_PI;
	}
/*
	pitch = pitch + 0.1;
	if (pitch >= 2*M_PI) {
		pitch = pitch - 2*M_PI;
	}

	roll = roll + 0.1;
	if (roll >= 2*M_PI) {
		roll = roll - 2*M_PI;
	}
*/
	return;
}

int main(int argc, char *argv[])//负责设置定时器循环调用draw函数
{
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &term_size) == -1) {
		perror("ioctl");
		exit(-1);
	}
//	rect_make(rect1);
	//printf("%d %d\n", term_size.ws_row, term_size.ws_col);
	framebuf = malloc(term_size.ws_row * term_size.ws_col);

	struct sigaction sigact = {
		.sa_handler = draw,
		.sa_flags = 0,
	};
	sigemptyset(&sigact.sa_mask);
	sigaction(SIGUSR1, &sigact, NULL);

	struct sigevent sev = {
		.sigev_notify = SIGEV_SIGNAL,
		.sigev_signo = SIGUSR1,
	};
	timer_t timer;
	timer_create(CLOCK_MONOTONIC, &sev, &timer);
	struct itimerspec its = {
		.it_interval = {
			.tv_sec = 0,
			.tv_nsec = 1000000 * 100,
		},
		.it_value = {
			.tv_sec = 0,
			.tv_nsec = 1000000 * 100,
		},
	};
	timer_settime(timer, 0, &its, NULL);

	while (1) {
		sleep(1);
	}

	free(framebuf);
	return 0;
}
