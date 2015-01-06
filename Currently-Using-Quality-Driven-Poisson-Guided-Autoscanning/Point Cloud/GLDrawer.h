#pragma once
#include "gl/glew.h"
#include <wrap/gl/space.h>
#include <wrap/qt/gl_label.h>
#include "cmesh.h"
#include "ParameterMgr.h"

#include <QtOpenGL/QGLWidget>
#include <iostream>
#include <GL/glut.h>
#include "Algorithm/Camera.h"

#include "Algorithm/Common/common_type.h"

using namespace std;
using namespace vcg;

class GLColor
{
public:
	GLColor(const float& _r = 0, const float& _g = 0, const float& _b = 0, const float& _a = 1.0):r(_r),g(_g),b(_b),a(_a){}
	GLColor(const QColor & qcolor)
	{
		int _r;
		int _g;
		int _b;
		qcolor.getRgb(&_r, &_g, &_b);
		r = _r / 255.0;
		g = _g / 255.0;
		b = _b / 255.0;
	}
	float r;
	float g;
	float b;
	float a;
};

static GLColor cBrown(0.7, 0, 0);
static GLColor cRed(1, 0, 0);
static GLColor cGreen(0, 1, 0);
static GLColor cBlue(0, 0, 1);
static GLColor cGray(0.5, 0.5, 0.5);
static GLColor cWhite(1, 1, 1);
static GLColor cBlack(0, 0, 0);
static GLColor cYellow(1, 1, 0);
static GLColor cOrange(1, 0.55, 0);
static GLColor cSnow(0.5, 0.5, 0.5);
static GLColor cPink(0.8, 0.1, 0.5);

class GLDrawer
{
public:
	GLDrawer(RichParameterSet* _para);
	~GLDrawer(void);
	typedef enum {DOT, QUADE, CIRCLE, NORMAL, SPHERE}DrawType;
  typedef enum {PATCH_GRAPH, CONTRACTION_GRAPH} GraphType;

	void setViewPoint(const Point3f& view){ view_point = view; }
	void draw(DrawType type, CMesh* mesh);
  void drawCamera(vcc::Camera& camera, bool is_draw_border = true);
  void drawSlice(Slice& slice, double trans_val);
  void drawSDFSlice(CMesh *slice_plane, int res, double trans_val);
  void drawSDFSliceDot(CMesh *sdf_slice);
  void drawGrid(const CMesh *cube_mesh, const int grid_num_each_edge);

	void updateDrawer(vector<int>& pickList);

	void cleanPickPoint();
	void drawPickPoint(CMesh* samples, vector<int>& pickList, bool bShow_as_dot);
	void setRGBNormals(vector<Point3f>& normals){RGB_normals = normals; }

  GLColor isoValue2Color(double value, double scale_threshold, double shift, bool need_negative);
  GLColor pvsValue2Color(double value);
  //cut from private
  void glDrawLine(Point3f& p0, Point3f& p1, GLColor color, double width);
  void drawSphere(CVertex& v);

  void drawCandidatesAxis(CMesh *mesh);
  void drawMeshLables(CMesh *mesh, QPainter *painter);

  GLColor getColorByType(CVertex& v);
  //shiyifei
  void drawGraphShow(GRAPHSHOW *graphcut, GraphType graphType = PATCH_GRAPH);

private:
	
	void draw(DrawType type);
	bool isCanSee(const Point3f& pos,  const Point3f& normal);

	void drawDot(CVertex& v);
	void drawCircle(CVertex& v);
	void drawQuade(CVertex& v);
	void drawNormal(CVertex& v);

	void glDrawPoint(Point3f& p, GLColor color, double size);
	void glDrawSphere(Point3f& p, GLColor color, double radius, int slide = 0);
	void glDrawCylinder(Point3f& p0, Point3f& p1, GLColor color, double width);
	//void glDrawCurves(vector<Curve>& curves, GLColor gl_color);

private:
	int x1, y1, x2, y2;
	bool doPick;
	vector<int> pickList;
	int pickPoint(int x, int y, vector<int> &result, int width=4, int height=4, bool only_one = true);

public:
  vector<GLColor> random_color_list;
  void generateRandomColorList(int num = 1000);

private:
	bool bCullFace;
	bool useNormalColor;
  bool useDifferBranchColor;
  bool bUseIsoInteral;
  bool bUseConfidenceColor;
  bool bShowSlice;
  bool bShowGridCenters;
  bool bShowNBVCandidates;
  bool bUseConfidenceSeparation;

	double original_draw_width;
	double sample_draw_width;
	double normal_width;
	double normal_length;
	double sample_dot_size;
  double iso_dot_size;
	double original_dot_size;  
  double iso_step_size;
  double confidence_Separation_value;

  double sample_cofidence_color_scale;
  double iso_color_scale;
  double iso_value_shift;
  double grid_color_scale;
  double grid_value_shift;

	QColor original_color;
	QColor sample_color;
	QColor feature_color;
	QColor normal_color;
	GLColor pick_color;

	QColor skel_bone_color;
	QColor skel_node_color;
	QColor skel_branch_color;
	double skel_bone_width;
	double skel_node_size;
	double skel_branch_size;

	int prevPickIndex;
	Point3f curr_pick;
	int curr_pick_indx;
	vector<Point3f> RGB_normals;

public:
	RichParameterSet* para;
	Point3f view_point;
};


class SnapshotSetting
{
public:
	QString outdir;
	QString file_name;
	QString basename;
	int counter;
	int resolution;
	bool transparentBackground;

	SnapshotSetting()
	{
		outdir=".\\snapshot\\";
		basename="snapshot";
		counter=0;
		resolution = global_paraMgr.glarea.getDouble("Snapshot Resolution");
		transparentBackground=true;
	};

	void setOutDir(QString str)
	{
		outdir = str;
	}

	void GetSysTime()
	{
		basename = file_name + QDateTime::currentDateTime().toString(Qt::ISODate);
		basename.replace(":", "-");
	}
};
