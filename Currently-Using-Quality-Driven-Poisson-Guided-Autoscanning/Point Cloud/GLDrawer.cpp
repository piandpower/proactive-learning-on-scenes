#include "GLDrawer.h"

extern MeshFace meshFace;
extern MeshVertex vertexShow;

extern MeshFace meshFaceExtra;
extern MeshVertex vertexExtraShow;
extern int g_switchModel;

GLDrawer::GLDrawer(RichParameterSet* _para)
{
	para = _para;
	generateRandomColorList();
}


GLDrawer::~GLDrawer(void)
{
}

void GLDrawer::updateDrawer(vector<int>& pickList)
{
	bCullFace = para->getBool("Need Cull Points");
	useNormalColor = para->getBool("Use Color From Normal");
	useDifferBranchColor = para->getBool("Use Differ Branch Color");
	bShowSlice = global_paraMgr.poisson.getBool("Show Slices Mode");

	original_draw_width = para->getDouble("Original Draw Width");
	sample_draw_width = 50 * para->getDouble("Sample Draw Width");

	original_color = para->getColor("Original Point Color");
	sample_color = para->getColor("Sample Point Color");
	normal_width = para->getDouble("Normal Line Width");
	normal_length  = para->getDouble("Normal Line Length");
	sample_dot_size = para->getDouble("Sample Dot Size");
	iso_dot_size = para->getDouble("ISO Dot Size");
	original_dot_size = para->getDouble("Original Dot Size");
	normal_color = para->getColor("Normal Line Color");
	feature_color = para->getColor("Feature Color");
	pick_color = para->getColor("Pick Point Color");

	skel_bone_color = para->getColor("Skeleton Bone Color");
	skel_node_color = para->getColor("Skeleton Node Color");
	skel_branch_color = para->getColor("Skeleton Branch Color");
	skel_bone_width = para->getDouble("Skeleton Bone Width") / 10000.;
	skel_node_size = para->getDouble("Skeleton Node Size") / 10000.;
  	
	iso_step_size = global_paraMgr.glarea.getDouble("ISO Interval Size");
	bUseIsoInteral = global_paraMgr.glarea.getBool("Use ISO Interval");
	bUseConfidenceColor = global_paraMgr.drawer.getBool("Show Confidence Color");

	bShowGridCenters = global_paraMgr.glarea.getBool("Show View Grids");
	bShowNBVCandidates = global_paraMgr.glarea.getBool("Show NBV Candidates");

	bUseConfidenceSeparation = global_paraMgr.nbv.getBool("Use Confidence Separation");
	confidence_Separation_value = global_paraMgr.nbv.getDouble("Confidence Separation Value");

  sample_cofidence_color_scale = global_paraMgr.glarea.getDouble("Sample Confidence Color Scale");
  grid_color_scale = global_paraMgr.glarea.getDouble("Grid ISO Color Scale");
  grid_value_shift = global_paraMgr.glarea.getDouble("Grid ISO Value Shift");
  iso_color_scale = global_paraMgr.glarea.getDouble("Point ISO Color Scale");
  iso_value_shift = global_paraMgr.glarea.getDouble("Point ISO Value Shift");

	if (!pickList.empty())
	{
		curr_pick_indx = pickList[0];
	}
}


void GLDrawer::draw(DrawType type, CMesh* _mesh)
{
	if (_mesh == NULL)
		return;

	bool doPick = para->getBool("Doing Pick");

	int qcnt = 0;
	CMesh::VertexIterator vi;
	for(vi = _mesh->vert.begin(); vi != _mesh->vert.end(); ++vi) 
	{
		if (doPick)
			glLoadName(qcnt);

		if (vi->is_ignore)
			continue;

		if (bUseConfidenceSeparation && vi->is_view_grid)
			if (vi->eigen_confidence < confidence_Separation_value)
				continue;

    ////don't draw free points
    //if (abs(vi->pvs_value - 1) < 1e-7)
    //  continue;

		Point3f& p = vi->P();      
		Point3f& normal = vi->N();

		if(!(bCullFace /*&& !vi->is_original*/) || isCanSee(p, normal))		
		{
			switch(type)
			{
			case DOT:
				drawDot(*vi);
				break;
			case CIRCLE:
				drawCircle(*vi);
				break;
			case QUADE:
				drawQuade(*vi);
				break;
			case NORMAL:
				drawNormal(*vi);
				break;
			case SPHERE:
				drawSphere(*vi);
				break;
			default:
				break;
			}
		}

		if (doPick) 
		{
			qcnt++;
		}
	}

	para->setValue("Doing Pick", BoolValue(false));
}

bool GLDrawer::isCanSee(const Point3f& pos, const Point3f& normal)
{
	return  ( (view_point - pos) * normal >= 0 );
}

GLColor GLDrawer::isoValue2Color(double iso_value, 
	                               double scale_threshold,
	                               double shift,
	                               bool need_negative)
{
	iso_value += shift;
	//if (!bShowSlice)
	//{
	//  iso_value += shift;
	//}

	if (scale_threshold <= 0)
	{
		scale_threshold = 1;
	}

	iso_value = (std::min)((std::max)(iso_value, -scale_threshold), scale_threshold);

	bool is_inside = true;
	if (iso_value < 0)
	{
		if (!need_negative)
		{
			iso_value = 0;
		}
		else
		{
			iso_value /= -scale_threshold;
		}
	}
	else
	{
		iso_value /= scale_threshold;
		is_inside = false;
	}

	vector<Point3f> base_colors(5);
	double step_size = 1.0 / (base_colors.size()-1);
	int base_id = iso_value / step_size;
	if (base_id == base_colors.size()-1)
	{
		base_id = base_colors.size()-2;
	}
	Point3f mixed_color;

	if (is_inside && need_negative)
	{
		base_colors[4] = Point3f(0.0, 0.0, 1.0);
		base_colors[3] = Point3f(0.0, 0.7, 1.0);
		base_colors[2] = Point3f(0.0, 1.0, 1.0);
		base_colors[1] = Point3f(0.0, 1.0, 0.7);
		base_colors[0] = Point3f(0.0, 1.0, 0.0);
	}
	else
	{
		base_colors[4] = Point3f(1.0, 0.0, 0.0);
		base_colors[3] = Point3f(1.0, 0.7, 0.0);
		base_colors[2] = Point3f(1.0, 1.0, 0.0);
		base_colors[1] = Point3f(0.7, 1.0, 0.0);
		base_colors[0] = Point3f(0.0, 1.0, 0.0);
	}

	mixed_color = base_colors[base_id] * (base_id * step_size + step_size - iso_value)
		            + base_colors[base_id + 1] * (iso_value - base_id * step_size);

	return GLColor(mixed_color.X() / float(step_size), 
		             mixed_color.Y() / float(step_size), 
		             mixed_color.Z() / float(step_size));
}

GLColor GLDrawer::pvsValue2Color(double value)
{
  if (abs(value - (0.5)) < 1e-7)   { return cGray; } //pvs default color
  if (abs(value) < 1e-7)          { return cRed;}   //occupied pvs grid color
  if (abs(value - 1.0) < 1e-4)      { return cGreen;} //free pvs grid color
  /* else if (value >= 0 && value <= 1 ) 
  return GLColor(1 - value, 1 - value, 1 - value);
  else std::cout<<"wrong pvs value! for pvsValue2Colcor" <<std::endl;*/
}

GLColor GLDrawer::getColorByType(CVertex& v)
{
	if (v.is_model)
		return cBlack;

  //shiyifei show graph point color
  if (v.is_graphcut_related){
    return GLColor(v.C()[0] / 255.0f, v.C()[1] / 255.0f, v.C()[2] / 255.0f);
  }

  if (v.is_original)
  {
    if (v.is_barely_visible)
       return cYellow;

    return original_color;
  }

	if (v.is_scanned && v.is_scanned_visible)
		return cRed;

	if (useNormalColor)
	{
		if (RGB_normals.empty())
		{
			//return GLColor( v.cN()[0] , v.cN()[1] , v.cN()[2] );
			return GLColor( (v.cN()[0] + 1) / 2.0 , (v.cN()[1] + 1) / 2.0 , (v.cN()[2]+ 1) / 2.0 );
		}
		else
		{
			Point3f normal = Point3f(v.cN());
			vector<double> color(3, 0.0);
			for(int i = 0; i < 3; i++)
			{
				color[i] = normal * RGB_normals[i];
				if(color[i] < 0)
				{
					color[i] = 0.;		
				}
				//color[i] = (normal * RGB_normals[i] + 1.5) / 2.0;
			}
			return GLColor( color[0], color[1], color[2]);
		}
	}

  if (v.is_view_grid)
  {
    //if (v.is_ray_hit) 
    //	return isoValue2color(v.eigen_confidence, iso_color_scale, iso_value_shift, true);
    if (v.is_ray_stop) return cOrange;
    else return isoValue2Color(v.eigen_confidence, grid_color_scale, grid_value_shift, true);
  }
  
	if (v.is_iso)
	{
    if (bUseConfidenceColor)
      return isoValue2Color(v.eigen_confidence, iso_color_scale, iso_value_shift, true);
    else
      return cGreen;
	}

  if (v.is_poisson)
  {
    if (bUseConfidenceColor)
    {
      GLColor c = isoValue2Color(1 - v.eigen_confidence, iso_color_scale, iso_value_shift, true);
      unsigned int r = 255 * c.r;
      unsigned int g = 255 * c.g;
      unsigned int b = 255 * c.b;
      //v.C().SetRGB(r, g, b);
      return isoValue2Color(1 - v.eigen_confidence, iso_color_scale, iso_value_shift, true);
    }
    else
      return cGreen;
  }

  if (v.is_fixed_sample)
	{
    if (v.IsB())  return cPink;
    else return cRed;
	}

  if (v.is_field_grid)
    return isoValue2Color(v.eigen_confidence, iso_color_scale, iso_value_shift, true);

  if (v.is_scanned)
    return cRed;

  if(v.is_sdf){
    return isoValue2Color(v.eigen_confidence, iso_color_scale, iso_value_shift, true);
  }

  if (bUseConfidenceColor)
    return isoValue2Color(v.eigen_confidence, sample_cofidence_color_scale, iso_value_shift, true);
  else
	  return sample_color;
}

void GLDrawer::drawDot(CVertex& v)
{
	int size;
	if (v.is_model)
	{
		size = original_dot_size;
	}
	else if (v.is_original)
	{
		size = original_dot_size;
	}
	else if(v.is_iso)
	{
		size = iso_dot_size;
	}
	else
	{
		size = sample_dot_size;
	}
//	glPointSize(size);

	if(v.C()[0] > 0)	
		glPointSize(double(size)/20);
	else			
		glPointSize(double(size)/100);
	
	glBegin(GL_POINTS);
	GLColor color = getColorByType(v);
	glColor4f(float(v.C()[0])/255, float(v.C()[1])/255, float(v.C()[2])/255, 1);
//	glColor4f(1, 0, 0, 1);

	Point3f p = v.P();

	//p.X() += 2 * int(v.eigen_confidence / iso_step_size);
	if (bUseIsoInteral)
	{
		p.X() += 2 * int(v.eigen_confidence / iso_step_size);
	}

	glVertex3f(p[0], p[1], p[2]);
	glEnd(); 
}

void GLDrawer::drawSphere(CVertex& v)
{
	double radius;
	if (v.is_original)
	{
		radius = original_draw_width;
	}
	else
	{
		radius = sample_draw_width;
	}

	GLColor color = getColorByType(v);
	glColor4f(color.r, color.g, color.b, 1);

	Point3f p = v.P();

	glDrawSphere(p, color, radius, 20);
}

void GLDrawer::drawCircle(CVertex& v)
{
	double radius = sample_draw_width;
	GLColor color = getColorByType(v);

	Point3f p = v.P(); 
	Point3f normal = v.cN();
	Point3f V1 = v.eigen_vector0;
	Point3f V0 = v.eigen_vector1;

	Point3f temp_V = V1 ^ normal;
	float temp = temp_V * V0;

	glNormal3f(normal[0], normal[1], normal[2]);
	glBegin(GL_POLYGON);
	glColor4f(color.r, color.g, color.b, 1);

	int nn = 30;
	for (int i = 0; i < nn; i++)
	{
		float tt1, tt2;
		float rad = 2*3.1415926/nn*i;
		float a, b; 
		a = radius;
		b = radius;

		tt2 = b * sin(rad);
		tt1 = a * cos(rad);

		Point3f vv;

		if (temp < 0)
		{
			vv = V1 * tt1 + V0 * tt2;
		}
		else
		{
			vv = V1 * tt1 - V0 * tt2;
		}
		glVertex3d(p[0]+vv[0], p[1]+vv[1], p[2]+vv[2]);
	}

	glEnd();
}

void GLDrawer::drawQuade(CVertex& v)
{
	double h = sample_draw_width;

	GLColor color = getColorByType(v);
	glColor4f(color.r, color.g, color.b, 1);

	Point3f p = v.P(); 
	Point3f normal = v.cN();
	Point3f V1 = v.eigen_vector0;
	Point3f V0 = v.eigen_vector1;

	//const double *m = v.m;
	glBegin(GL_QUADS);

	Point3f temp_V = V1 ^ normal;
	float temp = temp_V * V0;

	if (temp > 0)
	{
		Point3f p0 = p + V0 * h;
		Point3f p1 = p + V1 * h;
		Point3f p2 = p + (-V0) * h;
		Point3f p3 = p + (-V1) * h;

		glNormal3d(normal.X(), normal.Y(), normal.Z());
		glVertex3d(p0.X(), p0.Y(), p0.Z());
		glVertex3d(p1.X(), p1.Y(), p1.Z());
		glVertex3d(p2.X(), p2.Y(), p2.Z());
		glVertex3d(p3.X(), p3.Y(), p3.Z());
	}
	else
	{
		//cout << "negtive" << endl;
		Point3f p0 = p + (-V1) * h;
		Point3f p1 = p + (-V0) * h;
		Point3f p2 = p + V1 * h;
		Point3f p3 = p + V0 * h;

		glNormal3d(normal.X(), normal.Y(), normal.Z());
		glVertex3d(p0.X(), p0.Y(), p0.Z());
		glVertex3d(p1.X(), p1.Y(), p1.Z());
		glVertex3d(p2.X(), p2.Y(), p2.Z());
		glVertex3d(p3.X(), p3.Y(), p3.Z());
	}

	glEnd();  
}

void GLDrawer::drawNormal(CVertex& v)
{
	double width = normal_width;
	double length = normal_length;
	QColor qcolor = normal_color;

	//glDisable(GL_LIGHTING);

	glLineWidth(width); 
	GLColor color(qcolor);
	glColor4f(color.r, color.g, color.b, 1);  

	Point3f p = v.P(); 
	Point3f m = v.cN();

	glBegin(GL_LINES);	
	glVertex3d(p[0], p[1], p[2]);
	glVertex3f(p[0] + m[0]*length, p[1]+m[1]*length, p[2]+m[2]*length);
	glEnd(); 

	//glBegin(GL_LINES);	
	//glVertex3d(p[0], p[1], p[2]);
	//glVertex3f(p[0] - m[0]*length, p[1] - m[1]*length, p[2] - m[2]*length);
	//glEnd(); 

	//glEnable(GL_LIGHTING);
}

void GLDrawer::drawCamera(vcc::Camera& camera, bool is_draw_border)
{
	//get the five control points of the cone
	Point3f far_end = camera.pos + camera.direction * camera.far_distance;
	Point3f far_top_right = far_end + camera.right * camera.far_horizon_dist / 2 
		+ camera.up * camera.far_vertical_dist / 2;
	Point3f far_top_left = far_end + camera.right * (-camera.far_horizon_dist) / 2
		+ camera.up * camera.far_vertical_dist / 2; 
	Point3f far_bottom_right = far_end + camera.right * camera.far_horizon_dist / 2
		+ camera.up * (-camera.far_vertical_dist / 2);
	Point3f far_bottom_left = far_end + camera.right * (-camera.far_horizon_dist / 2)
		+ camera.up * (-camera.far_vertical_dist / 2);

  Point3f near_end = camera.pos + camera.direction * camera.near_distance;
  Point3f near_top_right = near_end + camera.right * camera.near_horizon_dist / 2 
    + camera.up * camera.near_vertical_dist / 2;
  Point3f near_top_left = near_end + camera.right * (-camera.near_horizon_dist) / 2
    + camera.up * camera.near_vertical_dist / 2; 
  Point3f near_bottom_right = near_end + camera.right * camera.near_horizon_dist / 2
    + camera.up * (-camera.near_vertical_dist / 2);
  Point3f near_bottom_left = near_end + camera.right * (-camera.near_horizon_dist / 2)
    + camera.up * (-camera.near_vertical_dist / 2);

  glBegin(GL_LINES);
  //draw direction
  glColor3f(0.0, 1.0f, 0.0f);
  glVertex(camera.pos); glVertex(far_end);

  if (is_draw_border)
  {
    glColor3f(1.0, 0.0, 0.0);
    glVertex(camera.pos); glVertex(far_top_left);
    glVertex(camera.pos); glVertex(far_top_right);
    glVertex(camera.pos); glVertex(far_bottom_left);
    glVertex(camera.pos); glVertex(far_bottom_right);
     //draw far face
    glVertex(far_top_right); glVertex(far_top_left);
    glVertex(far_top_left); glVertex(far_bottom_left);
    glVertex(far_bottom_left); glVertex(far_bottom_right);
    glVertex(far_bottom_right); glVertex(far_top_right);
     //draw near face
    glVertex(near_top_right); glVertex(near_top_left);
    glVertex(near_top_left); glVertex(near_bottom_left);
    glVertex(near_bottom_left); glVertex(near_bottom_right);
    glVertex(near_bottom_right); glVertex(near_top_right);
  }
	glEnd();
}

void GLDrawer::drawPickPoint(CMesh* samples, vector<int>& pickList, bool bShow_as_dot)
{
	double width = para->getDouble("Sample Draw Width");
	GLColor pick_color = para->getColor("Pick Point Color");
	glColor3f(pick_color.r, pick_color.g, pick_color.b);

	for(int ii = 0; ii < pickList.size(); ii++) 
	{
		int i = pickList[ii];

		if(i < 0 || i >= samples->vert.size())
			continue;

		CVertex &v = samples->vert[i];
		Point3f &p = v.P();     

		if(bShow_as_dot)
		{
			glPointSize(sample_dot_size * 1.2);
			glBegin(GL_POINTS);

			GLColor color = pick_color;
			glColor4f(color.r, color.g, color.b, 1);

			glVertex3d(p[0], p[1], p[2]);

			glEnd(); 
		}
		else
		{
			glDrawSphere(p, pick_color, sample_draw_width * 1.2, 40);
		}
	}    
}

void GLDrawer::glDrawLine(Point3f& p0, Point3f& p1, GLColor color, double width)
{
	glColor3f(color.r, color.g, color.b);
	glLineWidth(/*0.1*/2);
	glBegin(GL_LINES);
	glVertex3f(p0[0], p0[1], p0[2]);
	glVertex3f(p1[0], p1[1], p1[2]);
	glEnd();
}

void  DrawChannel(GLfloat   x1, GLfloat   y1, GLfloat   z1,   GLfloat   x2,   GLfloat   y2,   GLfloat   z2   ,double size)   
{   
	static GLUquadricObj *  quad_obj = NULL;  
	if ( quad_obj == NULL )  
		quad_obj = gluNewQuadric();  
	gluQuadricDrawStyle( quad_obj, GLU_FILL );  
	gluQuadricNormals( quad_obj, GLU_SMOOTH );  

	// 起始线段：以(0,1,0)为起点,它的长度(distance)通过目标线段计算,  
	//           终点坐标为(0,1-distance,0)  
	// 目标线段：以(x1,y1,z1)为起点，以(x2,y2,z2)为终点  
	// 计算目标向量  
	GLfloat   dx   =   x2   -   x1;   
	GLfloat   dy   =   y2   -   y1;   
	GLfloat   dz   =   z2   -   z1;  
	// 算出目标向量模(即AB长度)  
	GLfloat   distance  =  sqrt(dx*dx + dy*dy + dz*dz);   
	// 计算平移量  
	GLfloat  px = x1;  
	GLfloat  py = y1 - 1;  
	GLfloat  pz = z1;  
	// 起始线段的末端点  
	GLfloat  bx = px;    
	GLfloat  by = (1-distance) + py;  
	GLfloat  bz = pz;  
	// 计算起始向量  
	GLfloat  sx = bx - x1;  
	GLfloat  sy = by - y1;  
	GLfloat  sz = bz - z1;  
	// 计算向量(sx,sy,sz)与向量(dx,dy,dz)的法向量(sy*dz - dy*sz,sz*dx - sx*dz,sx*dy - dx*sy)  
	GLfloat fx = sy*dz - dy*sz;  
	GLfloat fy = sz*dx - sx*dz;  
	GLfloat fz = sx*dy - dx*sy;  
	// 求两向量间的夹角  
	// 计算第三条边的长度  
	GLfloat ax = fabs(x2 - bx);  
	GLfloat ay = fabs(y2 - by);  
	GLfloat az = fabs(z2 - bz);  
	GLfloat length = sqrt(ax*ax + ay*ay + az*az);  
	// 根据余弦定理计算夹角  
	GLfloat angle = acos((distance*distance*2 - length*length)/(2*distance*distance))*180.0f/3.14159;  
	// 绘制第一个点A   
	// 	glPushMatrix();   
	// 	glTranslatef(x1,y1,z1);   
	// 	auxSolidSphere(0.002);     
	// 	glPopMatrix();   
	// 绘制圆柱   
	glPushMatrix();   
	// 变换的顺序与函数书写的顺序相反，  
	// 即先平移(0,-1,0)，再绕法向量旋转，最后再平移  
	glTranslatef(x1,y1,z1);  
	glRotatef(angle,fx,fy,fz);    
	glTranslatef(0,-1,0);  

	// 圆柱体参数   
	GLdouble radius= size;        // 半径   
	GLdouble slices = 10.0;      //  段数   
	GLdouble stack = 10.0;       // 递归次数   
	gluCylinder( quad_obj, radius, radius, distance, slices, stack );   
//	auxSolidCylinder(size,distance);  
	glPopMatrix();  
	// 绘制第二个点B  
	// 	glPushMatrix();   
	// 	glTranslatef(x2,y2,z2);   
	// 	auxSolidSphere(0.002);       
	// 	glPopMatrix();   
};  


void RenderBone(float x0, float y0, float z0, float x1, float y1, float z1, double width = 20)  
{  
	GLdouble  dir_x = x1 - x0;  
	GLdouble  dir_y = y1 - y0;  
	GLdouble  dir_z = z1 - z0;  
	GLdouble  bone_length = sqrt( dir_x*dir_x + dir_y*dir_y + dir_z*dir_z );  
	static GLUquadricObj *  quad_obj = NULL;  
	if ( quad_obj == NULL )  
		quad_obj = gluNewQuadric();  
	gluQuadricDrawStyle( quad_obj, GLU_FILL );  
	gluQuadricNormals( quad_obj, GLU_SMOOTH );  
	glPushMatrix();  
	// 平移到起始点   
	glTranslated( x0, y0, z0 );  
	// 计算长度   
	double  length;  
	length = sqrt( dir_x*dir_x + dir_y*dir_y + dir_z*dir_z );  
	if ( length < 0.0001 ) {   
		dir_x = 0.0; dir_y = 0.0; dir_z = 1.0;  length = 1.0;  
	}  
	dir_x /= length;  dir_y /= length;  dir_z /= length;  
	GLdouble  up_x, up_y, up_z;  
	up_x = 0.0;  
	up_y = 1.0;  
	up_z = 0.0;  
	double  side_x, side_y, side_z;  
	side_x = up_y * dir_z - up_z * dir_y;  
	side_y = up_z * dir_x - up_x * dir_z;  
	side_z = up_x * dir_y - up_y * dir_x;  
	length = sqrt( side_x*side_x + side_y*side_y + side_z*side_z );  
	if ( length < 0.0001 ) {  
		side_x = 1.0; side_y = 0.0; side_z = 0.0;  length = 1.0;  
	}  
	side_x /= length;  side_y /= length;  side_z /= length;  
	up_x = dir_y * side_z - dir_z * side_y;  
	up_y = dir_z * side_x - dir_x * side_z;  
	up_z = dir_x * side_y - dir_y * side_x;  
	// 计算变换矩阵   
	GLdouble  m[16] = { side_x, side_y, side_z, 0.0,  
		up_x,   up_y,   up_z,   0.0,  
		dir_x,  dir_y,  dir_z,  0.0,  
		0.0,    0.0,    0.0,    1.0 };  
	glMultMatrixd( m );  
	// 圆柱体参数   
	GLdouble radius= width;        // 半径   
	GLdouble slices = 40.0;      //  段数   
	GLdouble stack = 3.0;       // 递归次数   
	gluCylinder( quad_obj, radius, radius, bone_length, slices, stack );   
	glPopMatrix();  
}  

void GLDrawer::glDrawCylinder(Point3f& p0, Point3f& p1, GLColor color, double width)
{
	glColor3f(color.r, color.g, color.b);
	RenderBone(p0[0], p0[1], p0[2], p1[0], p1[1], p1[2], width);
//	DrawChannel(p0[0], p0[1], p0[2], p1[0], p1[1], p1[2], width);
}


void GLDrawer::glDrawSphere(Point3f& p, GLColor color, double radius, int slide)
{
	if (radius < 0.0001)
		radius = 0.01;

	glColor3f(color.r, color.g, color.b);
	glPushMatrix();      
	glTranslatef(p[0], p[1], p[2]);
	glPointSize(radius);
// 	glBegin(GL_POINTS);
// 	glVertex3f(0,0,0);
// 	glEnd();
	glutSolidSphere(radius, slide, slide);
	glPopMatrix();
}

void GLDrawer::cleanPickPoint()
{
	curr_pick_indx = 0;
	prevPickIndex = 0;
}

void GLDrawer::generateRandomColorList(int num)
{
	if (num <= 1000)
		num = 1000;

	random_color_list.clear();

	srand(time(NULL));
	for(int i = 0; i < num; i++)
	{
		double r = (rand() % 1000) * 0.001;
		double g = (rand() % 1000) * 0.001;
		double b = (rand() % 1000) * 0.001;
		random_color_list.push_back(GLColor(r, g, b));
	}
}

void GLDrawer::drawGrid(const CMesh *cube_mesh, const int grid_num_each_edge = 3)
{
  Point3f cube_box_max = cube_mesh->bbox.max;
  Point3f cube_box_min = cube_mesh->bbox.min;
  double grid_length = (cube_box_max - cube_box_min).X() / grid_num_each_edge;
  Point3f grid_diagonal = Point3f(grid_length, grid_length, grid_length);

  for (int i = 0; i < grid_num_each_edge; ++i)
  {
    for (int j = 0; j < grid_num_each_edge; ++j)
    {
      for (int k = 0; k < grid_num_each_edge; ++k)
      {
        Point3f grid_min = cube_box_min + Point3f(i * grid_length, j * grid_length, k * grid_length);
        Point3f grid_max = grid_min + grid_diagonal;
        double min_x = grid_min.X(), min_y = grid_min.Y(), min_z = grid_min.Z();
        double max_x = grid_max.X(), max_y = grid_max.Y(), max_z = grid_max.Z();
        glBegin(GL_LINES);
        glColor3f(0.0, 0.0, 1.0);
        glVertex3f(min_x, min_y, min_z); glVertex3f(max_x, min_y, min_z);
        glVertex3f(max_x, min_y, min_z); glVertex3f(max_x, min_y, max_z);
        glVertex3f(max_x, min_y, max_z); glVertex3f(min_x, min_y, max_z);
        glVertex3f(min_x, min_y, max_z); glVertex3f(min_x, min_y, min_z);

        glVertex3f(min_x, max_y, min_z); glVertex3f(max_x, max_y, min_z);
        glVertex3f(max_x, max_y, min_z); glVertex3f(max_x, max_y, max_z);
        glVertex3f(max_x, max_y, max_z); glVertex3f(min_x, max_y, max_z);
        glVertex3f(min_x, max_y, max_z); glVertex3f(min_x, max_y, min_z);

        glVertex3f(min_x, min_y, min_z); glVertex3f(min_x, max_y, min_z);
        glVertex3f(max_x, min_y, min_z); glVertex3f(max_x, max_y, min_z);
        glVertex3f(max_x, min_y, max_z); glVertex3f(max_x, max_y, max_z);
        glVertex3f(min_x, min_y, max_z); glVertex3f(min_x, max_y, max_z);
        glEnd();
      }
    }
  }
}

void GLDrawer::drawSlice(Slice& slice, double trans_val)
{
	if (slice.slice_nodes.empty())
	{
		return;
	}
	Point3f p0, p1, p2, p3;

	//glPolygonMode(GL_SMOOTH);
	glShadeModel(GL_SMOOTH);   

  bool show_view_grid_slice = global_paraMgr.glarea.getBool("Show View Grid Slice");

	for (int i = 0; i < slice.res -1; i++)
	{
		for (int j = 0; j < slice.res -1; j++)
		{
			CVertex v0 = slice.slice_nodes[i * slice.res + j];
			CVertex v1 = slice.slice_nodes[i * slice.res + j + 1];
			CVertex v2 = slice.slice_nodes[(i+1) * slice.res + j + 1];
			CVertex v3 = slice.slice_nodes[(i+1) * slice.res + j];
      
      if (show_view_grid_slice)
      {
        GLColor c0 = isoValue2Color(v0.eigen_confidence, grid_color_scale, grid_value_shift, true);
        GLColor c1 = isoValue2Color(v1.eigen_confidence, grid_color_scale, grid_value_shift, true);
        GLColor c2 = isoValue2Color(v2.eigen_confidence, grid_color_scale, grid_value_shift, true);
        GLColor c3 = isoValue2Color(v3.eigen_confidence, grid_color_scale, grid_value_shift, true);
        glBegin(GL_QUADS);
        glColor4f(c0.r, c0.g, c0.b, trans_val); glVertex3f(v0.P().X(), v0.P().Y(), v0.P().Z());
        glColor4f(c1.r, c1.g, c1.b, trans_val); glVertex3f(v1.P().X(), v1.P().Y(), v1.P().Z());
        glColor4f(c2.r, c2.g, c2.b, trans_val); glVertex3f(v2.P().X(), v2.P().Y(), v2.P().Z());
        glColor4f(c3.r, c3.g, c3.b, trans_val); glVertex3f(v3.P().X(), v3.P().Y(), v3.P().Z());
        glEnd();
      }
      else
      {
        GLColor c0 = isoValue2Color(v0.eigen_confidence, iso_color_scale, iso_value_shift, true);
        GLColor c1 = isoValue2Color(v1.eigen_confidence, iso_color_scale, iso_value_shift, true);
        GLColor c2 = isoValue2Color(v2.eigen_confidence, iso_color_scale, iso_value_shift, true);
        GLColor c3 = isoValue2Color(v3.eigen_confidence, iso_color_scale, iso_value_shift, true);
        glBegin(GL_QUADS);
        glColor4f(c0.r, c0.g, c0.b, trans_val); glVertex3f(v0.P().X(), v0.P().Y(), v0.P().Z());
        glColor4f(c1.r, c1.g, c1.b, trans_val); glVertex3f(v1.P().X(), v1.P().Y(), v1.P().Z());
        glColor4f(c2.r, c2.g, c2.b, trans_val); glVertex3f(v2.P().X(), v2.P().Y(), v2.P().Z());
        glColor4f(c3.r, c3.g, c3.b, trans_val); glVertex3f(v3.P().X(), v3.P().Y(), v3.P().Z());
        glEnd();
      }

		}
	}
}

void GLDrawer::drawSDFSlice( CMesh *slice_plane, int res, double trans_val )
{
  if (slice_plane->vert.empty()){
    return;
  }

  Point3f p0, p1, p2, p3;

  //glPolygonMode(GL_SMOOTH);
  glShadeModel(GL_SMOOTH);   

  for (int i = 0; i < res - 1; i++)
  {
    for (int j = 0; j < res -1; j++)
    {
      CVertex v0 = slice_plane->vert[i * res + j];
      CVertex v1 = slice_plane->vert[i * res + j + 1];
      CVertex v2 = slice_plane->vert[(i+1) * res + j + 1];
      CVertex v3 = slice_plane->vert[(i+1) * res + j];

      GLColor c0 = isoValue2Color(v0.eigen_confidence, iso_color_scale, iso_value_shift, true);
      GLColor c1 = isoValue2Color(v1.eigen_confidence, iso_color_scale, iso_value_shift, true);
      GLColor c2 = isoValue2Color(v2.eigen_confidence, iso_color_scale, iso_value_shift, true);
      GLColor c3 = isoValue2Color(v3.eigen_confidence, iso_color_scale, iso_value_shift, true);

      glBegin(GL_QUADS);
      glColor4f(c0.r, c0.g, c0.b, trans_val); glVertex3f(v0.P().X(), v0.P().Y(), v0.P().Z());
      glColor4f(c1.r, c1.g, c1.b, trans_val); glVertex3f(v1.P().X(), v1.P().Y(), v1.P().Z());
      glColor4f(c2.r, c2.g, c2.b, trans_val); glVertex3f(v2.P().X(), v2.P().Y(), v2.P().Z());
      glColor4f(c3.r, c3.g, c3.b, trans_val); glVertex3f(v3.P().X(), v3.P().Y(), v3.P().Z());
      glEnd();
    }
  }
}

void GLDrawer::drawSDFSliceDot(CMesh *sdf_slice)
{
  if (sdf_slice->vert.empty()){
    return;
  }

  for(int i = 0; i < sdf_slice->vert.size(); ++i){
    drawDot(sdf_slice->vert[i]);
  }
}

//shiyifei
void GLDrawer::drawGraphShow(GRAPHSHOW *graphcut, int graphType)
{
  if(graphcut == NULL) return;

  double sphere_radius = 0.0f;
  double line_thickness = 0.0f;

  if (graphType == 0/*PATCH_GRAPH*/){
	  sphere_radius = 0.005;
	  line_thickness = 0.001;
  }else if(graphType == 1/*CONTRACTION_GRAPH*/){
    sphere_radius = 0.01;
    line_thickness = 0.002;
  }

  //draw dot in sphere
  for (int i = 0; i < graphcut->vecNodes.size(); ++i)
  {
    MyPt_RGB_NORMAL &pt = graphcut->vecNodes[i];
    glDrawSphere(Point3f(pt.x, pt.y, pt.z), GLColor(pt.r, pt.g, pt.b), sphere_radius, 100);
  }

  //draw edges
  for(int i = 0; i < graphcut->vecEdges.size(); ++i)
  {
    MyPt_RGB_NORMAL &s_pt = graphcut->vecEdges[i].first;
    MyPt_RGB_NORMAL &e_pt = graphcut->vecEdges[i].second;

	float r,g,b;
	if(/*graphcut->vecEdgeColor.size()*/0)
	{
		r = graphcut->vecEdgeColor[3 * i];
		g = graphcut->vecEdgeColor[3 * i + 1];
		b = graphcut->vecEdgeColor[3 * i + 2];
	}
	else
	{
		r = 0.0;
		g = 0.0;
		b = 1.0;
	}

    glDrawCylinder(Point3f(s_pt.x, s_pt.y, s_pt.z), Point3f(e_pt.x, e_pt.y, e_pt.z), GLColor(r, g, b), line_thickness);
	//glDrawLine(Point3f(s_pt.x, s_pt.y, s_pt.z), Point3f(e_pt.x, e_pt.y, e_pt.z), GLColor(r, g, b), line_thickness);
  }
}

void GLDrawer::drawMesh()
{
	glShadeModel(GL_SMOOTH);
	glMaterialf(GL_FRONT, GL_SHININESS, 100);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glBegin(GL_TRIANGLES);

	for(int i=0;i<meshFace.vecFace.size();i++)
	{
		if(!meshFace.vecFace[i].validFlag)  continue;
		if(g_switchModel%5 > 2)	continue;

		int p0,p1,p2;
		p0 = meshFace.vecFace[i].p0;
		p1 = meshFace.vecFace[i].p1;
		p2 = meshFace.vecFace[i].p2;

		Point_RGB_NORMAL pv0,pv1,pv2,pCenter;
		pv0 = vertexShow.vecVertex[p0];
		pv1 = vertexShow.vecVertex[p1];
		pv2 = vertexShow.vecVertex[p2];

		ColorType color0,color1,color2;

		if(g_switchModel%5 == 0)
		{
			color0 = vertexShow.objectColor[p0];
			color1 = vertexShow.objectColor[p1];
			color2 = vertexShow.objectColor[p2];
		}
		else if(g_switchModel%5 == 1)
		{
			color0 = vertexShow.objectnessColor[p0];
			color1 = vertexShow.objectnessColor[p1];
			color2 = vertexShow.objectnessColor[p2];
		}
		else if(g_switchModel%5 == 2)
		{
			color0 = vertexShow.patchColor[p0];
			color1 = vertexShow.patchColor[p1];
			color2 = vertexShow.patchColor[p2];
		}

		glColor4f(float(color0.mRed)/255,float(color0.mGreen)/255,float(color0.mBlue)/255,1);
		glNormal3f(pv0.normal_x, pv0.normal_y,pv0.normal_z);
		glVertex3f(pv0.x, pv0.y, pv0.z);

		glColor4f(float(color1.mRed)/255,float(color1.mGreen)/255,float(color1.mBlue)/255,1);
		glNormal3f(pv1.normal_x, pv1.normal_y, pv1.normal_z);
		glVertex3f(pv1.x, pv1.y, pv1.z);

		glColor4f(float(color2.mRed)/255,float(color2.mGreen)/255,float(color2.mBlue)/255,1);
		glNormal3f(pv2.normal_x, pv2.normal_y, pv2.normal_z);
		glVertex3f(pv2.x, pv2.y, pv2.z);
	}

	
// 	for(int i=0;i<meshFaceExtra.vecFace.size();i++)
// 	{
// 		int p0,p1,p2;
// 		p0 = meshFaceExtra.vecFace[i].p0;
// 		p1 = meshFaceExtra.vecFace[i].p1;
// 		p2 = meshFaceExtra.vecFace[i].p2;
// 
// 		Point_RGB_NORMAL pv0,pv1,pv2,pCenter;
// 		pv0 = vertexExtraShow.vecVertex[p0];
// 		pv1 = vertexExtraShow.vecVertex[p1];
// 		pv2 = vertexExtraShow.vecVertex[p2];
// 
// 		glColor4f(float(meshFaceExtra.vecFace[i].r)/255,float(meshFaceExtra.vecFace[i].g)/255,float(meshFaceExtra.vecFace[i].b)/255,1);
// 
// 		glNormal3f(pv0.normal_x, pv0.normal_y,pv0.normal_z);
// 		glVertex3f(pv0.x, pv0.y, pv0.z);
// 
// 		glNormal3f(pv1.normal_x, pv1.normal_y, pv1.normal_z);
// 		glVertex3f(pv1.x, pv1.y, pv1.z);
// 
// 		glNormal3f(pv2.normal_x, pv2.normal_y, pv2.normal_z);
// 		glVertex3f(pv2.x, pv2.y, pv2.z);
// 	}
	glEnd(); 
}

void GLDrawer::drawMeshLables(CMesh *mesh, QPainter *painter)
{
  for (int i = 0; i < mesh->vert.size(); i++)
  {
    CVertex& v = mesh->vert[i];
    
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDepthFunc(GL_ALWAYS);
    glDisable(GL_LIGHTING);

    QString str = QString::number(i, 10);
    glLabel::render(painter, v.P(),str);
    glPopAttrib();	
  }
}

void GLDrawer::drawCandidatesAxis(CMesh *mesh)
{
  double width = normal_width;
  double length = normal_length;
  double half_length = normal_length / 2.0;
  QColor qcolor = normal_color;

  for (int i = 0; i < mesh->vert.size(); i++)
  {
    CVertex& v = mesh->vert[i];

    glLineWidth(width); 
    GLColor color(qcolor);
    glColor4f(color.r, color.g, color.b, 1);  

    Point3f p = v.P(); 
    Point3f m0 = v.N();
    Point3f m1 = v.eigen_vector0;
    Point3f m2 = v.eigen_vector1;

    // Z
    glBegin(GL_LINES);	
    glVertex3d(p[0], p[1], p[2]);
    glVertex3f(p[0] + m0[0]*length, p[1]+m0[1]*length, p[2]+m0[2]*length);
    glEnd(); 

    // X
    glColor4f(1, 0, 0, 1);
    glBegin(GL_LINES);	
    glVertex3d(p[0], p[1], p[2]);
    glVertex3f(p[0] + m1[0]*half_length, p[1]+m1[1]*half_length, p[2]+m1[2]*half_length);
    glEnd(); 

    // Y
    glColor4f(0, 1, 0, 1);
    glBegin(GL_LINES);	
    glVertex3d(p[0], p[1], p[2]);
    glVertex3f(p[0] + m2[0]*half_length, p[1]+m2[1]*half_length, p[2]+m2[2]*half_length);
    glEnd(); 
		
  }
}
