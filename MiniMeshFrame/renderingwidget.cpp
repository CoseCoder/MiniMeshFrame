#define GLUT_BUILDING_LIB
#include "renderingwidget.h"
#include <QKeyEvent>
#include <QColorDialog>
#include <QFileDialog>
#include <iostream>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include <QTextCodec>
#include <gl/GLU.h>
#include <gl/glut.h>
#include <algorithm>
#include "mainwindow.h"
#include "ArcBall.h"
#include "globalFunctions.h"
#include <Eigen/Dense>
#include<Eigen/Sparse>
#include <stdlib.h> 
#include <ctime>
using namespace Eigen;
using namespace std;

RenderingWidget::RenderingWidget(QWidget *parent, MainWindow* mainwindow)
	: QGLWidget(parent), ptr_mainwindow_(mainwindow), eye_distance_(5.0),
	has_lighting_(false), is_draw_point_(true), is_draw_edge_(false), is_draw_face_(false), is_draw_texture_(false)
{
	ptr_arcball_ = new CArcBall(width(), height());
	ptr_mesh_ = new Mesh3D();

	is_load_texture_ = false;
	is_draw_axes_ = false;

	eye_goal_[0] = eye_goal_[1] = eye_goal_[2] = 0.0;
	eye_direction_[0] = eye_direction_[1] = 0.0;
	eye_direction_[2] = 1.0;

	is_draw_minimal_surface_local_ = false;
	is_draw_minimal_surface_global_ = false;
}

RenderingWidget::~RenderingWidget()
{
	SafeDelete(ptr_arcball_);
	SafeDelete(ptr_mesh_);
}

void RenderingWidget::initializeGL()
{
	glClearColor(0.3, 0.3, 0.3, 0.0);
	glShadeModel(GL_SMOOTH);

	glEnable(GL_DOUBLEBUFFER);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_DEPTH_TEST);
	glClearDepth(1);

	SetLight();

}

void RenderingWidget::resizeGL(int w, int h)
{
	h = (h == 0) ? 1 : h;

	ptr_arcball_->reSetBound(w, h);

	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(45.0, GLdouble(w) / GLdouble(h), 0.001, 1000);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void RenderingWidget::paintGL()
{
	glShadeModel(GL_SMOOTH);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (has_lighting_)
	{
		SetLight();
	}
	else
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	register vec eyepos = eye_distance_*eye_direction_;
	gluLookAt(eyepos[0], eyepos[1], eyepos[2],
		eye_goal_[0], eye_goal_[1], eye_goal_[2],
		0.0, 1.0, 0.0);
	glPushMatrix();

	glMultMatrixf(ptr_arcball_->GetBallMatrix());

	Render();
	glPopMatrix();
}

void RenderingWidget::timerEvent(QTimerEvent * e)
{
	updateGL();
}

void RenderingWidget::mousePressEvent(QMouseEvent *e)
{
	switch (e->button())
	{
	case Qt::LeftButton:
		ptr_arcball_->MouseDown(e->pos());
		break;
	case Qt::MidButton:
		current_position_ = e->pos();
		break;
	default:
		break;
	}

	updateGL();
}
void RenderingWidget::mouseMoveEvent(QMouseEvent *e)
{
	switch (e->buttons())
	{
		setCursor(Qt::ClosedHandCursor);
	case Qt::LeftButton:
		ptr_arcball_->MouseMove(e->pos());
		break;
	case Qt::MidButton:
		eye_goal_[0] -= 4.0*GLfloat(e->x() - current_position_.x()) / GLfloat(width());
		eye_goal_[1] += 4.0*GLfloat(e->y() - current_position_.y()) / GLfloat(height());
		current_position_ = e->pos();
		break;
	default:
		break;
	}

	updateGL();
}
void RenderingWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
	switch (e->button())
	{
	case Qt::LeftButton:
		break;
	default:
		break;
	}
	updateGL();
}
void RenderingWidget::mouseReleaseEvent(QMouseEvent *e)
{
	switch (e->button())
	{
	case Qt::LeftButton:
		ptr_arcball_->MouseUp(e->pos());
		setCursor(Qt::ArrowCursor);
		break;

	case Qt::RightButton:
		break;
	default:
		break;
	}
}

void RenderingWidget::wheelEvent(QWheelEvent *e)
{
	eye_distance_ += e->delta()*0.001;
	eye_distance_ = eye_distance_ < 0 ? 0 : eye_distance_;

	updateGL();
}

void RenderingWidget::keyPressEvent(QKeyEvent *e)
{
	switch (e->key())
	{
	case Qt::Key_A:
		break;
	default:
		break;
	}
}

void RenderingWidget::keyReleaseEvent(QKeyEvent *e)
{
	switch (e->key())
	{
	case Qt::Key_A:
		break;
	default:
		break;
	}
}

void RenderingWidget::Render()
{
	DrawAxes(is_draw_axes_);

	DrawPoints(is_draw_point_);
	DrawEdge(is_draw_edge_);
	DrawFace(is_draw_face_);
	DrawTexture(is_draw_texture_);

	DrawMinimalSurface_Local(is_draw_minimal_surface_local_);
	DrawMinimalSurface_Global(is_draw_minimal_surface_global_);
}

void RenderingWidget::SetLight()
{
	static GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	static GLfloat mat_shininess[] = { 50.0 };
	static GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
	static GLfloat white_light[] = { 0.8, 0.8, 0.8, 1.0 };
	static GLfloat lmodel_ambient[] = { 0.3, 0.3, 0.3, 1.0 };

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
}

void RenderingWidget::SetBackground()
{
	QColor color = QColorDialog::getColor(Qt::white, this, tr("background color"));
	GLfloat r = (color.red()) / 255.0f;
	GLfloat g = (color.green()) / 255.0f;
	GLfloat b = (color.blue()) / 255.0f;
	GLfloat alpha = color.alpha() / 255.0f;
	glClearColor(r, g, b, alpha);
	updateGL();
}

void RenderingWidget::ReadMesh()
{
	QString filename = QFileDialog::
		getOpenFileName(this, tr("Read Mesh"),
			"..", tr("Meshes (*.obj)"));

	if (filename.isEmpty())
	{
		emit(operatorInfo(QString("Read Mesh Failed!")));
		return;
	}
	//中文路径支持
	QTextCodec *code = QTextCodec::codecForName("gd18030");
	QTextCodec::setCodecForLocale(code);

	QByteArray byfilename = filename.toLocal8Bit();
	ptr_mesh_->LoadFromOBJFile(byfilename.data());

	//	m_pMesh->LoadFromOBJFile(filename.toLatin1().data());
	emit(operatorInfo(QString("Read Mesh from") + filename + QString(" Done")));
	emit(meshInfo(ptr_mesh_->num_of_vertex_list(), ptr_mesh_->num_of_edge_list(), ptr_mesh_->num_of_face_list()));
	updateGL();
}

void RenderingWidget::WriteMesh()
{
	if (ptr_mesh_->num_of_vertex_list() == 0)
	{
		emit(QString("The Mesh is Empty !"));
		return;
	}
	QString filename = QFileDialog::
		getSaveFileName(this, tr("Write Mesh"),
			"..", tr("Meshes (*.obj)"));

	if (filename.isEmpty())
		return;

	ptr_mesh_->WriteToOBJFile(filename.toLatin1().data());

	emit(operatorInfo(QString("Write Mesh to ") + filename + QString(" Done")));
}

void RenderingWidget::LoadTexture()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Load Texture"),
		"..", tr("Images(*.bmp *.jpg *.png *.jpeg)"));
	if (filename.isEmpty())
	{
		emit(operatorInfo(QString("Load Texture Failed!")));
		return;
	}


	glGenTextures(1, &texture_[0]);
	QImage tex1, buf;
	if (!buf.load(filename))
	{
		//        QMessageBox::warning(this, tr("Load Fialed!"), tr("Cannot Load Image %1").arg(filenames.at(0)));
		emit(operatorInfo(QString("Load Texture Failed!")));
		return;
		/*
		QImage dummy(128, 128, QImage::Format_ARGB32);
		dummy.fill(Qt::green);
		buf = dummy;
		*/
	}

	tex1 = QGLWidget::convertToGLFormat(buf);
	glBindTexture(GL_TEXTURE_2D, texture_[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, tex1.width(), tex1.height(),
		GL_RGBA, GL_UNSIGNED_BYTE, tex1.bits());

	is_load_texture_ = true;
	emit(operatorInfo(QString("Load Texture from ") + filename + QString(" Done")));
}

void RenderingWidget::CheckDrawPoint(bool bv)
{
	is_draw_point_ = bv;
	updateGL();
}
void RenderingWidget::CheckDrawEdge(bool bv)
{
	is_draw_edge_ = bv;
	updateGL();
}
void RenderingWidget::CheckDrawFace(bool bv)
{
	is_draw_face_ = bv;
	updateGL();
}
void RenderingWidget::CheckLight(bool bv)
{
	has_lighting_ = bv;
	updateGL();
}
void RenderingWidget::CheckDrawTexture(bool bv)
{
	is_draw_texture_ = bv;
	if (is_draw_texture_)
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);

	updateGL();
}
void RenderingWidget::CheckDrawAxes(bool bV)
{
	is_draw_axes_ = bV;
	updateGL();
}

void RenderingWidget::CheckDrawMinimalSurfaceLocal(bool bv)
{
	is_draw_minimal_surface_local_ = bv;
	updateGL();
}

void RenderingWidget::CheckDrawMinimalSurfaceGlobal(bool bv)
{
	is_draw_minimal_surface_global_ = bv;
	updateGL();
}

void RenderingWidget::DrawAxes(bool bV)
{
	if (!bV)
		return;
	//x axis
	glColor3f(1.0, 0.0, 0.0);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(0.7, 0.0, 0.0);
	glEnd();
	glPushMatrix();
	glTranslatef(0.7, 0, 0);
	glRotatef(90, 0.0, 1.0, 0.0);
	glutSolidCone(0.02, 0.06, 20, 10);
	glPopMatrix();

	//y axis
	glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(0.0, 0.7, 0.0);
	glEnd();

	glPushMatrix();
	glTranslatef(0.0, 0.7, 0);
	glRotatef(90, -1.0, 0.0, 0.0);
	glutSolidCone(0.02, 0.06, 20, 10);
	glPopMatrix();

	//z axis
	glColor3f(0.0, 0.0, 1.0);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(0.0, 0.0, 0.7);
	glEnd();
	glPushMatrix();
	glTranslatef(0.0, 0, 0.7);
	glutSolidCone(0.02, 0.06, 20, 10);
	glPopMatrix();

	glColor3f(1.0, 1.0, 1.0);
}

void RenderingWidget::DrawPoints(bool bv)
{
	if (!bv || ptr_mesh_ == NULL)
		return;
	if (ptr_mesh_->num_of_vertex_list() == 0)
	{
		return;
	}

	const std::vector<HE_vert*>& verts = *(ptr_mesh_->get_vertex_list());
	glBegin(GL_POINTS);
	for (size_t i = 0; i != ptr_mesh_->num_of_vertex_list(); ++i)
	{
		glNormal3fv(verts[i]->normal().data());
		glVertex3fv(verts[i]->position().data());
	}
	glEnd();
}

void RenderingWidget::DrawEdge(bool bv)
{
	if (!bv || ptr_mesh_ == NULL)
		return;

	if (ptr_mesh_->num_of_face_list() == 0)
	{
		return;
	}

	const std::vector<HE_face *>& faces = *(ptr_mesh_->get_faces_list());
	for (size_t i = 0; i != faces.size(); ++i)
	{
		glBegin(GL_LINE_LOOP);
		HE_edge *pedge(faces.at(i)->pedge_);
		do
		{
			glNormal3fv(pedge->pvert_->normal().data());
			glVertex3fv(pedge->pvert_->position().data());

			pedge = pedge->pnext_;

		} while (pedge != faces.at(i)->pedge_);
		glEnd();
	}
}

void RenderingWidget::DrawFace(bool bv)
{
	if (!bv || ptr_mesh_ == NULL)
		return;

	if (ptr_mesh_->num_of_face_list() == 0)
	{
		return;
	}

	const std::vector<HE_face *>& faces = *(ptr_mesh_->get_faces_list());

	glBegin(GL_TRIANGLES);

	for (size_t i = 0; i != faces.size(); ++i)
	{
		HE_edge *pedge(faces.at(i)->pedge_);
		do
		{
			glNormal3fv(pedge->pvert_->normal().data());
			glVertex3fv(pedge->pvert_->position().data());

			pedge = pedge->pnext_;

		} while (pedge != faces.at(i)->pedge_);
	}

	glEnd();
}

void RenderingWidget::DrawTexture(bool bv)
{
	if (!bv)
		return;
	if (ptr_mesh_->num_of_face_list() == 0 || !is_load_texture_)
		return;

	//默认使用球面纹理映射，效果不好
	ptr_mesh_->SphereTex();

	const std::vector<HE_face *>& faces = *(ptr_mesh_->get_faces_list());

	glBindTexture(GL_TEXTURE_2D, texture_[0]);
	glBegin(GL_TRIANGLES);
	for (size_t i = 0; i != faces.size(); ++i)
	{
		HE_edge *pedge(faces.at(i)->pedge_);
		do
		{
			/*请在此处绘制纹理，添加纹理坐标即可*/
			glTexCoord2fv(pedge->pvert_->texCoord_.data());
			glNormal3fv(pedge->pvert_->normal().data());
			glVertex3fv(pedge->pvert_->position().data());

			pedge = pedge->pnext_;

		} while (pedge != faces.at(i)->pedge_);
	}

	glEnd();
}

void RenderingWidget::DrawMinimalSurface_Local(bool bv)
{
	if (!bv || ptr_mesh_ == NULL)
		return;
	if (ptr_mesh_->num_of_face_list() == 0 || !is_draw_minimal_surface_local_)
		return;
	const std::vector<HE_vert*>& verts = *(ptr_mesh_->get_vertex_list());
	Vec3f t, vi;
	int count;
	std::vector<Vec3f> newVertsPosition;
	std::vector<int> change;
	for (int times = 0; times < 3000; times++) {
		newVertsPosition.clear();
		change.clear();
		for (size_t i = 0; i != ptr_mesh_->num_of_vertex_list(); ++i)
		{
			if (!verts[i]->isOnBoundary()) {
				change.push_back(i);
				HE_edge* edge = verts[i]->pedge_;
				count = 0;
				t.fill(0.0);
				do {
					count++;
					vi = edge->pvert_->position();
					t += (verts[i]->position() - vi);
					// do something with edge, edge->pair or edge->face
					edge = edge->ppair_->pnext_;
				} while (edge != verts[i]->pedge_);
				t /= count;
				t *= 0.3;
				newVertsPosition.push_back(verts[i]->position() - t);
			}
		}
		bool closed_surface = verts.size() == newVertsPosition.size();

		if (closed_surface) {
			is_draw_minimal_surface_global_ = false;
			return;
		}

		for (size_t i = 0; i < change.size(); ++i) {
			verts[change[i]]->set_position(newVertsPosition[i]);
		}
	}

	is_draw_minimal_surface_local_ = false;
}

void RenderingWidget::DrawMinimalSurface_Global(bool bv)
{
	if (!bv || ptr_mesh_ == NULL)
		return;
	if (ptr_mesh_->num_of_face_list() == 0 || !is_draw_minimal_surface_global_)
		return;


	const std::vector<HE_vert*>& verts = *(ptr_mesh_->get_vertex_list());
	int m = verts.size();
	SparseMatrix<double>  A(m, m);
	std::vector<Triplet<double>> coef;
	VectorXd conX(m), conY(m), conZ(m);
	Vec3f vert;
	int boundary_verts = 0;

	for (size_t i = 0; i != ptr_mesh_->num_of_vertex_list(); ++i)
	{
		vert = verts[i]->position();
		if (verts[i]->isOnBoundary()) {
			coef.push_back(Triplet<double>(i, i, 1));
			conX(i) = vert.x();
			conY(i) = vert.y();
			conZ(i) = vert.z();
		}
		else {
			int count = 0, id;
			boundary_verts++;
			conX(i) = conY(i) = conZ(i) = 0;
			HE_edge* edge = verts[i]->pedge_;
			do {
				count++;
				id = edge->pvert_->id();
				coef.push_back(Triplet<double>(i, id, -1));
				// do something with edge, edge->pair or edge->face
				edge = edge->ppair_->pnext_;
			} while (edge != verts[i]->pedge_);
			coef.push_back(Triplet<double>(i, i, count));
		}
	}
	A.setFromTriplets(coef.begin(), coef.end());
	//A.makeCompressed();
	VectorXd x, y, z;

	// 求解
	SparseLU<SparseMatrix<double>> chol(A);  // 执行A的 LU分解
	x = chol.solve(conX);         // 使用A的Cholesky分解来求解等号右边的向量
	y = chol.solve(conY);
	z = chol.solve(conZ);


	bool closed_surface = verts.size() == boundary_verts;
	if (closed_surface) {
		is_draw_minimal_surface_global_ = false;
		return;
	}

	for (size_t i = 0; i < m; ++i) {
		vert.x() = x(i);
		vert.y() = y(i);
		vert.z() = z(i);
		//std::cout << verts[i]->position() << "\n" << vert << "\n\n";
		verts[i]->set_position(vert);
	}

	is_draw_minimal_surface_global_ = false;
}

int RenderingWidget::findVertId(std::vector<Vec3f> verts, Vec3f point)
{
	for (int i = 0; i < verts.size(); i++) {
		if (verts[i].x() == point.x() && verts[i].y() == point.y()) {
			return i;
		}
	}
	return -1;
}

void RenderingWidget::CreateSubdiv2D()
{
	double xMax = 3, yMax = 3;
	int n = 100;
	cv::Rect rect(-xMax / 2 - 0.01, -yMax / 2 - 0.01, xMax + 0.02, yMax + 0.02);
	vector<Vec3f> verts;
	vector<int> faces;
	cv::Subdiv2D subdiv2d(rect);
	double x, y, z[3], t;
	srand(time(0));
	/*for (int i = 0; i < n; i++) {
		x = rand() / (double)RAND_MAX * 2 - 1;
		y = rand() / (double)RAND_MAX * 2 - 1;
		subdiv2d.insert(cv::Point2f(x, y));
	}*/
	for (int j = 1; j < 20; j++) {
		t = 2 - 0.2*j;
		for (int i = 0; i < n; i++) {
			x = rand() / (double)RAND_MAX * t - t / 2;
			//x = rand() / (double)RAND_MAX * 1.8 - 0.9;
			y = sqrt((1 - j*0.1)*(1 - j*0.1) - x*x);
			//y = sqrt(0.9 - x*x);
			subdiv2d.insert(cv::Point2f(x, y));
			subdiv2d.insert(cv::Point2f(x, -y));
		}
	}
	double m = n;
	for (int i = 0; i < (int)m; i++) {
		x = i / (m - 1);
		//x = rand() / (double)RAND_MAX * 1.8 - 0.9;
		y = sqrt(1 - x*x);
		//y = sqrt(0.9 - x*x);
		subdiv2d.insert(cv::Point2f(x, y));
		subdiv2d.insert(cv::Point2f(x, -y));
		subdiv2d.insert(cv::Point2f(-x, y));
		subdiv2d.insert(cv::Point2f(-x, -y));
	}


	vector<cv::Vec6f> triangles;
	subdiv2d.getTriangleList(triangles);
	int id[3];
	Vec3f v[3];
	bool in;
	for (int i = 0; i < triangles.size(); i++) {
		in = true;
		for (int j = 0; j < 3; j++) {
			x = triangles[i][2 * j];
			y = triangles[i][2 * j + 1];
			if (x*x + y*y > 1.1) {
				in = false;
				break;
			}
		}
		if (!in)
			continue;

		for (int j = 0; j < 3; j++) {
			x = triangles[i][2 * j];
			y = triangles[i][2 * j + 1];
			z[j] = sqrt(1 - x*x - y*y);
			//z[j] += rand() / (double)RAND_MAX* zfloat - zfloat / 2;
			if (x*x + y*y > 1)
				z[j] = 0;
			v[j] = Vec3f(x, y, z[j]);
			id[j] = findVertId(verts, v[j]);
			if (id[j] == -1) {
				id[j] = verts.size();
				verts.push_back(v[j]);
				if (z[j] > 0) {
					v[j].z() = -z[j];
					verts.push_back(v[j]);
				}
			}
			faces.push_back(id[j]);
		}
		for (int j = 0; j < 3; j++) {
			if (z[2 - j] > 0)
				id[2 - j]++;
			faces.push_back(id[2 - j]);
		}
	}

	ptr_mesh_->CreateMesh(verts, faces);
	updateGL();
}
