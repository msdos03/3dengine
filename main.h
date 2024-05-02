struct camera {
	double focal;//焦距
	double position[3];//相机坐标
	double vec_i[3];//平面x轴单位向量
	double vec_j[3];//平面y轴单位向量
};

double dot_product(double *v1, double *v2);
void cross_product(double *v1, double *v2, double *result);
void vec_sub(double *v1, double *v2, double *result);
void vec_add(double *v1, double *v2, double *result);
void matrix_mul(double *left, double *right, double *ret);
void drw_dot(int x, int y, char *framebuf, struct winsize* sz);
void drw_line(int *p1, int *p2, char *framebuf, struct winsize* sz);
int render_point(double *point, int *rendered, struct winsize* sz, struct camera *cam);
void render_line(double *p1, double *p2, char *framebuf, struct winsize* sz, struct camera *cam);
void render_rect(double *rect, char *framebuf, struct winsize* sz);
void rect_make(double *ret);
void rotate(double *points, double *new_points, int num, double yaw, double pitch, double roll);
void draw(int sig);

