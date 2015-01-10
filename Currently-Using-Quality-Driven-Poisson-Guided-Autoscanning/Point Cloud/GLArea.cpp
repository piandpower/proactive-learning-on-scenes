#include "GLArea.h"

//shiyifei
extern GRAPHSHOW graphInit;
extern GRAPHSHOW graphContract;

//#include "Poisson/MultiGridOctreeData.h"
#define PI 3.1415926535897932384

GLArea::GLArea(QWidget *parent): QGLWidget(/*QGLFormat(QGL::DoubleBuffer | QGL::DepthBuffer |QGL::SampleBuffers),*/ parent),
  para(global_paraMgr.getGlareaParameterSet()),
  glDrawer(global_paraMgr.getDrawerParameterSet()),
  dataMgr(global_paraMgr.getDataParameterSet()),
  norSmoother(global_paraMgr.getNormalSmootherParameterSet()),
  poisson(global_paraMgr.getPoissonParameterSet()),
  camera(global_paraMgr.getCameraParameterSet()),
  paintMutex(QMutex::NonRecursive),
  nbv(global_paraMgr.getNBVParameterSet())
{
  setMouseTracking(true); 
  isDragging = false;
  isRightPressed = false;

  trackball_light.center=Point3f(0, 0, 0);
  trackball_light.radius= 1;
  activeDefaultTrackball=true;

  fov = 60;
  clipRatioFar = 1;
  clipRatioNear = 1;
  nearPlane = .2f;
  farPlane = 5.f;
  takeSnapTile = false;
  default_snap_path = QString(".\\snapshot\\");
  current_snap_path = default_snap_path;
  snapDrawScal = 1;
  is_paintGL_locked = false;
  RGB_counter = 0;

  need_rotate = false;
  rotate_angle = 0.;
  rotate_pos = Point3f(0, 0, 0);
  rotate_normal = Point3f(1, 0, 0.);
  is_figure_shot = false;
  rotate_delta = 1;
  rotate_angle = 0.;
  nbv_ball_slice = 90;
  cout << "GLArea constructed" << endl;

  initial_light_have_set = false;

  CVertex v;
  cout << "Memory Size of each CVertex:  " << sizeof(v) << endl;
}

GLArea::~GLArea(void)
{
}


void GLArea::initializeGL()
{
  cout << "initializeGL" << endl;

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0); 
  glEnable(GL_LIGHT1);
  glEnable(GL_COLOR_MATERIAL);  

  glShadeModel (GL_SMOOTH);
  glShadeModel(GL_FLAT);

  //glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST ); 
  GLfloat mat_ambient[4] = {0.1745, 0.01175, 0.01175,1.0}; 
  GLfloat mat_diffuse[4] = {0.61424, 0.04136, 0.04136, 1.0 };
  GLfloat mat_specular[] = {0.727811, 0.626959, 0.626959, 1.0 };
  GLfloat shininess = 0.6*128;

  glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_ambient);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse); 
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialf(GL_FRONT, GL_SHININESS, shininess);

  glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT );
  glColorMaterial( GL_FRONT_AND_BACK, GL_DIFFUSE );
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

  glDisable(GL_BLEND);
  glEnable(GL_NORMALIZE);

  glDisable(GL_CULL_FACE);
  glColor4f(1, 1, 1, 1);

  glEnable(GL_LIGHTING);

  initLight();


  trackball.center = Point3f(0, 0, 0);
  trackball.radius = 1;

  // force to open anti
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //glBlendFunc(GL_ONE, GL_ZERO);  
  glEnable(GL_BLEND);
  glEnable(GL_POINT_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POLYGON_SMOOTH);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

  glDisable(GL_LIGHTING);

  glLoadIdentity(); 
}

void GLArea::initLight()
{
  GLColor color_ambient(para->getColor("Light Ambient Color"));
  float ambient[4] = {color_ambient.r, color_ambient.g, color_ambient.b, 1.0};
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
  glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);

  GLColor color_diffuse(para->getColor("Light Diffuse Color"));
  float diffuse[4] = {color_diffuse.r, color_diffuse.g, color_diffuse.b, 1.0};
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);

  GLColor color_specular(para->getColor("Light Specular Color"));
  float specular[4] = {color_specular.r, color_specular.g, color_specular.b, 1.0};
  glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
  glLightfv(GL_LIGHT1, GL_SPECULAR, specular);

}

void GLArea::resizeGL(int w, int h)
{
  //cout << "resizeGL" << endl;
  glViewport(0, 0, (GLint)w, (GLint)h);  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();  

  float r = w/(float)h;
  gluPerspective(30, r, 0.1, 10);
  glMatrixMode(GL_MODELVIEW);  
}

void GLArea::paintGL() 
{
  paintMutex.lock();{

    if (is_paintGL_locked){
      goto PAINT_RETURN;
    }
    
    lightOnOff(para->getBool("Light On or Off"));

    GLColor color(global_paraMgr.drawer.getColor("Background Color"));
    glClearColor(color.r, color.g, color.b, 1); 

    Point3f lightPos = para->getPoint3f("Light Position");
    float lpos[4];
    lpos[0] = lightPos[0];
    lpos[1] = lightPos[1];
    lpos[2] = lightPos[2];
    lpos[3] = 0;
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);
    lpos[0] = -lightPos[0];
    lpos[1] = -lightPos[1];
    lpos[2] = -lightPos[2];
    lpos[3] = 0;
    glLightfv(GL_LIGHT1, GL_POSITION, lpos);       
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    double SnapResolutionScale = global_paraMgr.glarea.getDouble("Snapshot Resolution");
    if (takeSnapTile)
    {
      double normal_width = global_paraMgr.drawer.getDouble("Normal Line Width");
      double dot_size = global_paraMgr.drawer.getDouble("Sample Dot Size");
      double iso_size = global_paraMgr.drawer.getDouble("ISO Dot Size");
      double original_dot_size = global_paraMgr.drawer.getDouble("Original Dot Size");

      global_paraMgr.drawer.setValue("Normal Line Width", DoubleValue(normal_width * SnapResolutionScale * SnapResolutionScale * snapDrawScal));
      global_paraMgr.drawer.setValue("Sample Dot Size", DoubleValue(dot_size * SnapResolutionScale * SnapResolutionScale * snapDrawScal));
      global_paraMgr.drawer.setValue("ISO Dot Size", DoubleValue(iso_size * SnapResolutionScale * SnapResolutionScale * snapDrawScal));		
      global_paraMgr.drawer.setValue("Original Dot Size", DoubleValue(original_dot_size * SnapResolutionScale * SnapResolutionScale * snapDrawScal));
    }

    glLoadIdentity();
    gluLookAt(0, 0, -3, 0, 0, 0, 0, 1, 0); 

    if (takeSnapTile)
    {
      setView();// high resolution snapshot
    }

    drawLightBall();

    //Drawing the scene 
    glPushMatrix();
    trackball.GetView();

    Point3f c = -gl_box.Center();
    double radius = 2.0f/gl_box.Diag();

    trackball.Apply(false);

    glPushMatrix();
    glScalef(radius, radius, radius);
    glTranslatef(c[0], c[1], c[2]);

    glTranslatef(rotate_pos[0], rotate_pos[1], rotate_pos[2]);
    glRotatef(rotate_angle, rotate_normal[0], rotate_normal[1], rotate_normal[2]);
    glTranslatef(-rotate_pos[0], -rotate_pos[1], -rotate_pos[2]);


    View<float> view;
    view.GetView();
    Point3f viewpoint = view.ViewPoint();
    glDrawer.setViewPoint(viewpoint);

    if (dataMgr.isSamplesEmpty() && dataMgr.isOriginalEmpty() && dataMgr.isModelEmpty() && dataMgr.isSDFVoxelsEmpty())
    {
      goto PAINT_RETURN;
    }

    glDrawer.updateDrawer(pickList);


    // have problems...
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_ONE, GL_ZERO)

    if (para->getBool("Show Model"))
    {
      //different from drawing dot
      if (!dataMgr.isModelEmpty())
      {
        glw.m = dataMgr.getCurrentModel();
        //glw.Draw(GLW::DMWire, GLW::CMPerMesh, GLW::TMNone);
        glw.Draw(GLW::DMSmooth, GLW::CMPerMesh, GLW::TMNone);
      }
    }

    if(para->getBool("Show Samples"))  
    {
      if (!dataMgr.isSamplesEmpty() && para->getBool("Show Model"))
      {
        glw.m = dataMgr.getCurrentSamples();
        glw.Draw(GLW::DMWire, GLW::CMPerMesh, GLW::TMNone);
        //for pvs debug
        //glDrawer.draw(GLDrawer::DOT, dataMgr.getCurrentSamples());
        /*if (!dataMgr.isRIMLSEmpty())
        {
          glDrawer.draw(GLDrawer::DOT, dataMgr.getRIMLS());
        }*/
      }
      else
      {
        if(para->getBool("Show Samples Quad"))
          glDrawer.draw(GLDrawer::QUADE, dataMgr.getCurrentSamples());
        if(para->getBool("Show Samples Dot"))
          glDrawer.draw(GLDrawer::DOT, dataMgr.getCurrentSamples());
        if(para->getBool("Show Samples Circle"))
          glDrawer.draw(GLDrawer::CIRCLE, dataMgr.getCurrentSamples());	
        if (para->getBool("Show Samples Sphere"))
          glDrawer.draw(GLDrawer::SPHERE, dataMgr.getCurrentSamples());	
      }
    }

    if (para->getBool("Show Normal")) 
    {
      if (para->getBool("Show NBV Candidates"))
      {
        if (!dataMgr.isNBVCandidatesEmpty())
        {
          glDrawer.draw(GLDrawer::NORMAL, dataMgr.getNbvCandidates());
          //glDrawer.drawCandidatesAxis(dataMgr.getNbvCandidates());
          //drawCandidatesConnectISO();

          if (para->getBool("Show NBV Label"))
          {
            QPainter painter(this);

            //painter.begin(this);
            glDrawer.drawMeshLables(dataMgr.getNbvCandidates(), &painter);
            //painter.end();
          }
        }
      }else if (para->getBool("Show View Grids"))
      {
        if (!dataMgr.isViewGridsEmpty())
        {
          glDrawer.draw(GLDrawer::NORMAL, dataMgr.getViewGridPoints());
        } 
      }
      else if (para->getBool("Show ISO Points"))
      {
        glDrawer.draw(GLDrawer::NORMAL, dataMgr.getCurrentIsoPoints());
      }
      else if(para->getBool("Show Samples"))
      {
        glDrawer.draw(GLDrawer::NORMAL, dataMgr.getCurrentSamples());
      }
      else
      {
        if(!dataMgr.isOriginalEmpty())
          glDrawer.draw(GLDrawer::NORMAL, dataMgr.getCurrentOriginal());
      }
    }
    
    if(!global_paraMgr.nbv.getBool("NBV Lock PaintGL") && para->getBool("Show Original"))
    {
      if(!dataMgr.isOriginalEmpty())
      {
        if(para->getBool("Show Original Quad"))
          glDrawer.draw(GLDrawer::QUADE, dataMgr.getCurrentOriginal());
        if(para->getBool("Show Original Dot"))
          glDrawer.draw(GLDrawer::DOT, dataMgr.getCurrentOriginal());
        if(para->getBool("Show Original Circle"))
          glDrawer.draw(GLDrawer::CIRCLE, dataMgr.getCurrentOriginal());
        if (para->getBool("Show Original Sphere"))
          glDrawer.draw(GLDrawer::SPHERE, dataMgr.getCurrentOriginal());	
      }
    }

    if (para->getBool("Show ISO Points"))
    {
      if (!dataMgr.isIsoPointsEmpty())
      {
        //glDrawer.draw(GLDrawer::DOT, dataMgr.getCurrentIsoPoints());
        if(para->getBool("Show Samples Quad"))
          glDrawer.draw(GLDrawer::QUADE, dataMgr.getCurrentIsoPoints());
        if(para->getBool("Show Samples Dot"))
          glDrawer.draw(GLDrawer::DOT, dataMgr.getCurrentIsoPoints());
        if(para->getBool("Show Samples Circle"))
          glDrawer.draw(GLDrawer::CIRCLE, dataMgr.getCurrentIsoPoints());	
        if (para->getBool("Show Samples Sphere"))
          glDrawer.draw(GLDrawer::SPHERE, dataMgr.getCurrentIsoPoints());	
      }
    }
    
    if (!(takeSnapTile && para->getBool("No Snap Radius")))
    {
      glDrawer.drawPickPoint(dataMgr.getCurrentSamples(), pickList, para->getBool("Show Samples Dot"));
    }

    if (isDragging && para->getBool("Multiply Pick Point"))
    {
      drawPickRect();
    }

    if (para->getBool("Show View Grids"))
    {
      CMesh *nbv_grids = dataMgr.getViewGridPoints();

      if (NULL == nbv_grids) return;

      if(!nbv_grids->vert.empty())
      {
        glDrawer.draw(GLDrawer::DOT, nbv_grids);
      }
      else 
      {
        CMesh* field_points = dataMgr.getCurrentFieldPoints();
        if (!dataMgr.isFieldPointsEmpty())
        {
          glDrawer.draw(GLDrawer::DOT, field_points);
        }
      }
    }

    if (para->getBool("Show NBV Candidates"))
    {
      CMesh *nbv_candidates = dataMgr.getNbvCandidates();

      if(!nbv_candidates->vert.empty()) 
        glDrawer.draw(GLDrawer::DOT, nbv_candidates);
    }

    if (para->getBool("Show Scan Candidates"))
    {
      vcc::Camera current_camera;
      //double predict_size = global_paraMgr.camera.getDouble("Predicted Model Size");
      double h_dist = global_paraMgr.camera.getDouble("Camera Horizon Dist") / global_paraMgr.data.getDouble("Max Normalize Length");        
      double v_dist = global_paraMgr.camera.getDouble("Camera Vertical Dist") / global_paraMgr.data.getDouble("Max Normalize Length");

      double far_dist = global_paraMgr.camera.getDouble("Camera Far Distance") / global_paraMgr.data.getDouble("Max Normalize Length");
      double near_dist = global_paraMgr.camera.getDouble("Camera Near Distance") / global_paraMgr.data.getDouble("Max Normalize Length"); 
      //double dist_to_model = global_paraMgr.camera.getDouble("Camera Dist To Model");
      double dist_to_model = global_paraMgr.camera.getDouble("Camera Dist To Model") / global_paraMgr.data.getDouble("Max Normalize Length");
      
      current_camera.far_horizon_dist = h_dist;
      current_camera.far_vertical_dist = v_dist;
      current_camera.near_horizon_dist = h_dist / far_dist * near_dist;
      current_camera.near_vertical_dist = v_dist / far_dist * near_dist;
      current_camera.far_distance = far_dist;
      current_camera.near_distance = near_dist;

      //draw selected scan candidates
      vector<ScanCandidate> *selected_candidates = dataMgr.getSelectedScanCandidates();
      if (!selected_candidates->empty())
      {
        vector<ScanCandidate>::iterator it = selected_candidates->begin();
        for (; it != selected_candidates->end(); ++it)
        {
          current_camera.pos = it->first;
          current_camera.direction = it->second;
          current_camera.computeUpAndRight();
          CVertex v;
          v.P() = current_camera.pos;
          //glDrawer.drawSphere(v);
          glDrawer.drawCamera(current_camera, global_paraMgr.camera.getBool("Show Camera Border"));
        }
      }
    }

    if (para->getBool("Show Scan History"))
    {
      vector<ScanCandidate> *history = dataMgr.getScanHistory();
      double far_dist = global_paraMgr.camera.getDouble("Camera Far Distance") 
        / global_paraMgr.data.getDouble("Max Normalize Length");
      if (!history->empty())
      {
        vector<ScanCandidate>::iterator it = history->begin();
        for (; it != history->end(); ++it)
        {
          CVertex v;
          v.P() = it->first;
          glDrawer.drawSphere(v);
          glDrawer.glDrawLine(it->first, it->first + it->second.Normalize() * far_dist, cBlue, 5);
        }
      }
    }

    //sdf related
    if (para->getBool("Show SDF Voxels"))
    {
      CMesh* sdf_voxels = dataMgr.getSDFVoxels();
      if (!dataMgr.isSDFVoxelsEmpty())
      {
        glDrawer.draw(GLDrawer::DOT, sdf_voxels);
      }
    }

     if (para->getBool("Show SDF Slices"))
    {
      int res = global_paraMgr.nbv.getInt("SDF Slice Plane Resolution");
      if (global_paraMgr.nbv.getBool("Show SDF Slice X"))
      {
        glDrawer.drawSDFSlice(&dataMgr.x_sdf_slice_plane, res, 1);
      }
      if (global_paraMgr.nbv.getBool("Show SDF Slice Y"))
      {
        glDrawer.drawSDFSlice(&dataMgr.y_sdf_slice_plane, res, 1);
      }
      if (global_paraMgr.nbv.getBool("Show SDF Slice Z"))
      {
        glDrawer.drawSDFSlice(&dataMgr.z_sdf_slice_plane, res, 1);
      }
    }

    if (para->getBool("Show Scanned Mesh"))
    {
      //draw scanned mesh
      if (!dataMgr.isScannedResultsEmpty())
      {
        vector<CMesh* > *scanned_results = dataMgr.getScannedResults();
        for (vector<CMesh* >::iterator it = scanned_results->begin(); 
          it != scanned_results->end(); ++it)
        {
          if ((*it)->vert.empty()) continue;
          //if the scanned mesh is invisible, then continue
          if (!((*it)->vert[0].is_scanned_visible)) continue;

          if(para->getBool("Show Samples Quad"))
            glDrawer.draw(GLDrawer::QUADE, *it);
          if(para->getBool("Show Samples Dot"))
            glDrawer.draw(GLDrawer::DOT, *it);
          if(para->getBool("Show Samples Circle"))
            glDrawer.draw(GLDrawer::CIRCLE, *it);	
          if (para->getBool("Show Samples Sphere"))
            glDrawer.draw(GLDrawer::SPHERE, *it);	
        }
      }
    }

    if (para->getBool("Show Poisson Surface"))
    {
      if (!dataMgr.isPoissonSurfaceEmpty())
      {
        //glw.m = dataMgr.getCurrentPoissonSurface();
        ////glw.Draw(GLW::DMWire, GLW::CMPerMesh, GLW::TMNone);
        //glw.Draw(GLW::DMSmooth, GLW::CMPerMesh, GLW::TMNone);

        if(para->getBool("Show Samples Quad"))
          glDrawer.draw(GLDrawer::QUADE, dataMgr.getCurrentPoissonSurface());
        if(para->getBool("Show Samples Dot"))
          glDrawer.draw(GLDrawer::DOT, dataMgr.getCurrentPoissonSurface());
        if(para->getBool("Show Samples Circle"))
          glDrawer.draw(GLDrawer::CIRCLE, dataMgr.getCurrentPoissonSurface());	
        if (para->getBool("Show Samples Sphere"))
          glDrawer.draw(GLDrawer::SPHERE, dataMgr.getCurrentPoissonSurface());	
      }
    }

    //shiyifei, show graph cut related
    if (para->getBool("Show GraphCut Related"))
    {
      //1. show graph cut result
      if(!dataMgr.isGraphCutResultEmpty())
      {
        if(para->getBool("Show Samples Quad"))
          glDrawer.draw(GLDrawer::QUADE, dataMgr.getCurrentGraphCutResult());
        if(para->getBool("Show Samples Dot"))
          glDrawer.draw(GLDrawer::DOT, dataMgr.getCurrentGraphCutResult());
        if(para->getBool("Show Samples Circle"))
          glDrawer.draw(GLDrawer::CIRCLE, dataMgr.getCurrentGraphCutResult());
        if (para->getBool("Show Samples Sphere"))
          glDrawer.draw(GLDrawer::SPHERE, dataMgr.getCurrentGraphCutResult());	
      }

      //2. show contraction graph
	  if(!dataMgr.isContractionGraphEmpty())
	  {
		  glDrawer.drawGraphShow(dataMgr.getContractionGraph(),1);
	  }

      //3. show patch graph
	  if(!dataMgr.isPatchGraphEmpty())
	  {
//		  glDrawer.drawGraphShow(dataMgr.getPatchGraph(),0);
	  }

    }

    if (para->getBool("Show Bounding Box") && para->getBool("Show View Grid Slice"))
    {
      glColor3f(0, 0, 0);
      glLineWidth(3);

      Box3f box = dataMgr.getCurrentSamples()->bbox;
      double radius = global_paraMgr.data.getDouble("CGrid Radius") * 2;
      Point3f shift_positive(radius, radius, radius);
      Point3f shift_negtive(-radius, -radius, -radius);
      Box3f shift_box;
      shift_box.Add(box.min+shift_negtive);
      shift_box.Add(box.max+shift_positive);

      glBoxWire(shift_box);
      glBoxWire(dataMgr.whole_space_box);
    }
    else if (para->getBool("Show Bounding Box"))
    {
      //glColor3f(0, 0, 0);

      Box3f box = dataMgr.getCurrentSamples()->bbox;
      glBoxWire(box);

      //Box3f standard_box;
      //standard_box.min = Point3f(-1, -1, -1);
      //standard_box.max = Point3f(1, 1, 1);
      //glBoxWire(standard_box);
      glBoxWire(dataMgr.whole_space_box);
      CoordinateFrame(dataMgr.whole_space_box.Diag()/2.0).Render(this, NULL);

      CMesh *view_grid_points = dataMgr.getViewGridPoints();
      if (NULL == view_grid_points) return;

      if(!view_grid_points->vert.empty())
      {
        glDrawer.drawGrid(view_grid_points, global_paraMgr.nbv.getInt("View Bin Each Axis"));
      }
    }


    /* The following are semitransparent, careful for the rendering order*/
    glDepthMask(GL_FALSE);

    if (global_paraMgr.poisson.getBool("Show Slices Mode"))
    {
      glDisable(GL_LIGHTING);
      glDisable(GL_CULL_FACE);
      if (!global_paraMgr.poisson.getBool("Show Transparent Slices"))
      {
        if (global_paraMgr.poisson.getBool("Show X Slices"))
        {
          glDrawer.drawSlice((*dataMgr.getCurrentSlices())[0], 1);
        }
        if (global_paraMgr.poisson.getBool("Show Y Slices"))
        {
          glDrawer.drawSlice((*dataMgr.getCurrentSlices())[1], 1);
        }
        if (global_paraMgr.poisson.getBool("Show Z Slices"))
        {
          glDrawer.drawSlice((*dataMgr.getCurrentSlices())[2], 1);
        }
      }
      else
      {
        double trans_value = para->getDouble("Radius Ball Transparency");
        if (global_paraMgr.poisson.getBool("Show X Slices"))
        {
          glDrawer.drawSlice((*dataMgr.getCurrentSlices())[0], trans_value);
        }
        if (global_paraMgr.poisson.getBool("Show Y Slices"))
        {
          glDrawer.drawSlice((*dataMgr.getCurrentSlices())[1], trans_value);
        }
        if (global_paraMgr.poisson.getBool("Show Z Slices"))
        {
          glDrawer.drawSlice((*dataMgr.getCurrentSlices())[2], trans_value);
        }
      }
      //glEnable(GL_CULL_FACE);
    }

    if (para->getBool("Show NBV Candidates")
      && para->getBool("Show NBV Ball")) 
    {
      drawNBVBall();
    }

    else if (para->getBool("Show Radius")&& !(takeSnapTile && para->getBool("No Snap Radius"))) 
    {
      drawNeighborhoodRadius();
    }

    glDepthMask(GL_TRUE);


    glPopMatrix();
    glPopMatrix();

    if (takeSnapTile){
      cout << "snap shot!" << endl;
      pasteTile();

      double normal_width = global_paraMgr.drawer.getDouble("Normal Line Width");
      double dot_size = global_paraMgr.drawer.getDouble("Sample Dot Size");
      double iso_size = global_paraMgr.drawer.getDouble("ISO Dot Size");
      double original_dot_size = global_paraMgr.drawer.getDouble("Original Dot Size");

      global_paraMgr.drawer.setValue("Normal Line Width", DoubleValue(normal_width / (SnapResolutionScale * SnapResolutionScale * snapDrawScal)));
      global_paraMgr.drawer.setValue("Sample Dot Size", DoubleValue(dot_size / (SnapResolutionScale * SnapResolutionScale * snapDrawScal)));
      global_paraMgr.drawer.setValue("ISO Dot Size", DoubleValue(iso_size / (SnapResolutionScale * SnapResolutionScale * snapDrawScal)));		
      global_paraMgr.drawer.setValue("Original Dot Size", DoubleValue(original_dot_size / (SnapResolutionScale * SnapResolutionScale * snapDrawScal)));
    }
  }

PAINT_RETURN:
  paintMutex.unlock();
}


void GLArea::lightOnOff(bool _val)
{
  if(_val)
  {
    glEnable(GL_LIGHTING);
  }
  else
  {
    glDisable(GL_LIGHTING);
  }
}

void GLArea::initAfterOpenFile()
{
  dataMgr.getInitRadiuse();
  initSetting();
  //emit needUpdateStatus();
}

void GLArea::initSetting()
{
  dataMgr.recomputeQuad();
  initView();
  emit needUpdateStatus();
}

void GLArea::initView()
{
  dataMgr.recomputeBox();
  if (!dataMgr.isOriginalEmpty())
  {
    gl_box = dataMgr.getCurrentOriginal()->bbox;
  }
  else if(!dataMgr.isSamplesEmpty())
  {
    gl_box = dataMgr.getCurrentSamples()->bbox;
  }
  else if(!dataMgr.isModelEmpty())
  {
    gl_box = dataMgr.getCurrentModel()->bbox;
  }
  else if(!dataMgr.isSDFVoxelsEmpty())
  {
    gl_box = dataMgr.getSDFVoxels()->bbox;
  }
}

void GLArea::runPointCloudAlgorithm(PointCloudAlgorithm& algorithm)
{
  paintMutex.lock();

  QString name = algorithm.getParameterSet()->getString("Algorithm Name");
  cout << "*********************** Start  " << name.toStdString() << "  *************" << endl;
  int starttime, stoptime, timeused;
  starttime = clock();

  algorithm.setInput(&dataMgr);
  algorithm.run();
  algorithm.clear();

  stoptime = clock();
  timeused = stoptime - starttime;

  int currentUsedTime = timeused/double(CLOCKS_PER_SEC);

  cout << "time used:  " << timeused/double(CLOCKS_PER_SEC) << " seconds." << endl;
  cout << "*********************** End  " << name.toStdString() << "  ****************" << endl;
  cout << endl << endl;

  paintMutex.unlock();
}

void GLArea::openByDrop(QString fileName)
{
  if(fileName.endsWith("ply"))
  {
    if (fileName.contains("model"))
    {
      dataMgr.loadPlyToModel(fileName);
    }else if (fileName.contains("camera"))
    {
      dataMgr.loadCameraModel(fileName);
    }else if (fileName.contains("original"))
    {
      dataMgr.loadPlyToOriginal(fileName);
    }
    else if (fileName.contains("_iso"))
    {
      dataMgr.loadPlyToISO(fileName);
    }
    else
    {
      dataMgr.loadPlyToSample(fileName);
    }
  }

  if(fileName.endsWith("View"))
  {
    loadView(fileName);
  }
  else if(fileName.endsWith("RGBN"))
  {
    readRGBNormal(fileName);
  }
  if (fileName.endsWith("jpg"))
  {
    dataMgr.loadImage(fileName);
  }
  if(fileName.endsWith("xyz"))
  {
    dataMgr.loadXYZN(fileName);
  }
  if (fileName.endsWith("para"))
  {
    dataMgr.loadParameters(fileName);
  }

  //initAfterOpenFile();
  dataMgr.getInitRadiuse();
  //dataMgr.recomputeQuad();
  initView();
  emit needUpdateStatus();

  updateGL();
}

void GLArea::loadDefaultModel()
{
  dataMgr.loadPlyToModel("model.ply");
  if (dataMgr.getCameraModel() == NULL){
    return;
  }
  vcg::tri::UpdateNormals<CMesh>::PerFace(*dataMgr.getCurrentModel());
  int f_size = dataMgr.getCurrentModel()->face.size();
  for (int f = 0; f < f_size; ++f)
    dataMgr.getCurrentModel()->face[f].N().Normalize();

  //dataMgr.loadPlyToModel("box_model.ply");  
  //dataMgr.loadSkeletonFromSkel("child.skel");
  //dataMgr.loadPlyToOriginal("child_original.ply");
  //dataMgr.loadPlyToSample("child_sample.ply");
  //dataMgr.loadPlyToISO("child_iso.ply");
  //dataMgr.loadPlyToSample("default.ply");
  //dataMgr.loadPlyToOriginal("default_original.ply");
  //dataMgr.loadSkeletonFromSkel("Yoga1 MC Labeled.skel");
  //dataMgr.loadSkeletonFromSkel("default.skel");

  //dataMgr.loadSkeletonFromSkel("6 figure test.skel");
  //dataMgr.loadSkeletonFromSkel("wlop2 + iso.skel");
  //dataMgr.loadSkeletonFromSkel("default.skel");
  //dataMgr.loadSkeletonFromSkel("cube3.skel");


  //dataMgr.loadPlyToModel("model.ply"); 
  //dataMgr.loadPlyToOriginal("model.ply");
  //dataMgr.loadCameraModel("camera.ply");

  initAfterOpenFile();
  updateGL();
}

void GLArea::drawPickRect()
{
  GLint viewport[4];
  glGetIntegerv (GL_VIEWPORT, viewport);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0,width(),height(),0,-1,1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_COLOR_LOGIC_OP);
  glLogicOp(GL_XOR);
  glColor3f(1,1,1);

  glBegin(GL_LINE_LOOP);
  glVertex2f(x1,viewport[3] - y1);
  glVertex2f(x2,viewport[3] - y1);
  glVertex2f(x2,viewport[3] - y2);
  glVertex2f(x1,viewport[3] - y2);
  glEnd();
  glDisable(GL_LOGIC_OP);

  // Closing 2D
  glPopAttrib();
  glPopMatrix(); // restore model view
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void GLArea::drawNBVBall()
{
  glMatrixMode(GL_MODELVIEW_MATRIX);

  glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
  //glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_DEPTH_TEST);
  //draw transparent ball
  //static const GLfloat light_position[] = {1.0f, 1.0f, -1.0f, 1.0f};
  //static const GLfloat light_ambient[]   = {0.2f, 0.2f, 0.2f, 1.0f};
  //static const GLfloat light_diffuse[]   = {1.0f, 1.0f, 1.0f, 1.0f};
  //static const GLfloat light_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

  static const GLfloat light_position[] = {1.0f, 1.0f, -1.0f, 1.0f};
  static const GLfloat light_ambient[]   = {0.2f, 0.2f, 0.2f, 1.0f};
  static const GLfloat light_diffuse[]   = {1.0f, 1.0f, 1.0f, 1.0f};
  static const GLfloat light_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

  glLightfv(GL_LIGHT0, GL_AMBIENT,   light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE,   light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

  //glEnable(GL_LIGHT0);
  //glEnable(GL_LIGHTING);

  double trans_value = para->getDouble("Radius Ball Transparency");
  glColor4f(0,0,1,trans_value);
  //glShadeModel(GL_SMOOTH);

  Box3f box = dataMgr.getCurrentOriginal()->bbox;
  Point3f center = (box.min + box.max) / 2.0;

  double max_normalize_length = global_paraMgr.data.getDouble("Max Normalize Length");
  Point3f scanner_position_normalize = dataMgr.scanner_position / max_normalize_length - dataMgr.original_center_point;

  double radius = GlobalFun::computeEulerDist(scanner_position_normalize, center);

  //2014-5-13
  radius = GlobalFun::computeEulerDist(scanner_position_normalize, Point3f(0., 0., 0.));
  center = Point3f(0., 0., 0.);
  //cout << max_normalize_length << endl;

  glPushMatrix();
  glTranslatef(center[0], center[1], center[2]);

  //glutSolidSphere(radius, 40, 40);
  glutWireSphere(radius, nbv_ball_slice, nbv_ball_slice);
  glutSolidSphere(radius, nbv_ball_slice, nbv_ball_slice);

  glPopMatrix();

  //glDisable(GL_LIGHTING);
  //glDisable(GL_LIGHT0);
  //glDisable(GL_BLEND);
  //glDisable(GL_CULL_FACE);
}


void GLArea::removeBadCandidates()
{
  Box3f box = dataMgr.getCurrentOriginal()->bbox;
  Point3f center = (box.min + box.max) / 2.0;

  //double max_normalize_length = global_paraMgr.data.getDouble("Max Normalize Length");
  //Point3f scanner_position_normalize = dataMgr.scanner_position / max_normalize_length - dataMgr.original_center_point;

  //double save_radius = GlobalFun::computeEulerDist(scanner_position_normalize, center);

  double radius_threshold =GlobalFun::computeEulerDist(box.min, box.max)/2.5;

  CMesh* nbv_candidates = dataMgr.getNbvCandidates();
  for (int i = 0; i < nbv_candidates->vert.size(); i++)
  {
    CVertex& v = nbv_candidates->vert[i];

    double nbv_dist = GlobalFun::computeEulerDist(v.P(), center);
    //double dist_diff = abs(nbv_dist - save_radius);

    if (nbv_dist < radius_threshold)
    {
      v.is_ignore = true;
    }
  }

  GlobalFun::deleteIgnore(nbv_candidates);
}

//void GLArea::removeBadCandidates()
//{
//  Box3f box = dataMgr.getCurrentOriginal()->bbox;
//  Point3f center = (box.min + box.max) / 2.0;
//
//  double max_normalize_length = global_paraMgr.data.getDouble("Max Normalize Length");
//  Point3f scanner_position_normalize = dataMgr.scanner_position / max_normalize_length - dataMgr.original_center_point;
//
//  double save_radius = GlobalFun::computeEulerDist(scanner_position_normalize, center);
//
//  double radius_threshold = global_paraMgr.data.getDouble("CGrid Radius") * 4.1;
//
//  CMesh* nbv_candidates = dataMgr.getNbvCandidates();
//  for (int i = 0; i < nbv_candidates->vert.size(); i++)
//  {
//    CVertex& v = nbv_candidates->vert[i];
//
//    double nbv_dist = GlobalFun::computeEulerDist(v.P(), center);
//    double dist_diff = abs(nbv_dist - save_radius);
//
//    if (dist_diff > radius_threshold)
//    {
//      v.is_ignore = true;
//    }
//  }
//
//  GlobalFun::deleteIgnore(nbv_candidates);
//}


void GLArea::drawNeighborhoodRadius()
{
  if (dataMgr.getCurrentSamples()->vert.empty()) return;

  Point3f p;
  if(!pickList.empty() && pickList[0] >= 0)
  {
    int id = pickList[0];
    if (id >= 0 && id < dataMgr.getCurrentSamples()->vert.size())
    {
      p = dataMgr.getCurrentSamples()->vert[id].P();
    }
    else
    {
      p = dataMgr.getCurrentSamples()->vert[0].P();
    }
  }
  else
  {
    p = dataMgr.getCurrentSamples()->vert[0].P();
  }

  double h_Gaussian_para = global_paraMgr.data.getDouble("H Gaussian Para");
  double grid_radius = global_paraMgr.data.getDouble("CGrid Radius");

  if (!takeSnapTile && para->getBool("Show Red Radius Line"))
  {
    glColor3f(1, 0, 0);
    glLineWidth(3);

    glBegin(GL_LINES);

    glVertex3f(p[0], p[1], p[2]);
    glVertex3f(p[0], p[1] + grid_radius, p[2]);
    glEnd();
  }

  //glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
  //glEnable(GL_DEPTH_TEST);
  //draw transparent ball
  //static const GLfloat light_position[] = {1.0f, 1.0f, -1.0f, 1.0f};
  //static const GLfloat light_ambient[]   = {0.2f, 0.2f, 0.2f, 1.0f};
  //static const GLfloat light_diffuse[]   = {1.0f, 1.0f, 1.0f, 1.0f};
  //static const GLfloat light_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

  //glLightfv(GL_LIGHT0, GL_AMBIENT,   light_ambient);
  //glLightfv(GL_LIGHT0, GL_DIFFUSE,   light_diffuse);
  //glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

  //glEnable(GL_LIGHT0);
  //glEnable(GL_LIGHTING);
  // glEnable(GL_DEPTH_TEST);

  glMatrixMode(GL_MODELVIEW_MATRIX);

  glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
  double trans_value = para->getDouble("Radius Ball Transparency");
  glColor4f(0,0,1,trans_value);
  glShadeModel(GL_SMOOTH);

  // if (para->getBool("Show Samples Dot"))
  // {
  //   glDepthMask(GL_FALSE);//not for quad
  // }
  // else
  // {
  //   glDepthMask(GL_FALSE);
  //   glDisable(GL_DEPTH_TEST);
  // }
  //glEnable(GL_CULL_FACE);

  CMesh* samples = dataMgr.getCurrentSamples();

  if (para->getBool("Show All Radius") && samples->vn < 1000)
  {
    for(int i = 0; i < samples->vert.size(); i++)
    {
      CVertex& v = samples->vert[i];
      glPushMatrix();
      glTranslatef(v.P()[0], v.P()[1], v.P()[2]);
      glutSolidSphere(grid_radius  / sqrt(h_Gaussian_para), 40, 40);
      glPopMatrix();
    }
  }

  if (para->getBool("Show Samples Dot"))
  {
    glDepthMask(GL_TRUE);
  }

  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  glDisable(GL_BLEND);
  glDisable(GL_CULL_FACE);

  //initLight();
  //glPopAttrib();
}

void GLArea::drawLightBall()
{
  // ============== LIGHT TRACKBALL ==============
  // Apply the trackball for the light direction

  glPushMatrix();
  trackball_light.GetView();
  trackball_light.Apply(!(isDefaultTrackBall()));

  static float lightPosF[]={0.0,0.0,1.0,0.0};
  glLightfv(GL_LIGHT0,GL_POSITION,lightPosF);
  static float lightPosB[]={0.0,0.0,-1.0,0.0};
  glLightfv(GL_LIGHT1,GL_POSITION,lightPosB);

  if (!(isDefaultTrackBall()))
  {
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glColor3f(1,1,0);
    glDisable(GL_LIGHTING);
    const unsigned int lineNum=3;
    glBegin(GL_LINES);
    for(unsigned int i=0;i<=lineNum;++i)
      for(unsigned int j=0;j<=lineNum;++j) {
        glVertex3f(-1.0f+i*2.0/lineNum,-1.0f+j*2.0/lineNum,-2);
        glVertex3f(-1.0f+i*2.0/lineNum,-1.0f+j*2.0/lineNum, 2);
      }
      glEnd();
      glPopAttrib();
  }
  glPopMatrix();
}

void GLArea::changeColor(QString paraName)
{
  QColor qcolor;
  if (global_paraMgr.drawer.hasParameter(paraName))
  {
    qcolor = global_paraMgr.drawer.getColor(paraName);
  }
  else if (global_paraMgr.glarea.hasParameter(paraName))
  {
    qcolor = global_paraMgr.glarea.getColor(paraName);
  }
  else
  {
    return;
  }

  qcolor = QColorDialog::getColor(qcolor);

  if(qcolor.isValid()){

    if(paraName.contains("Light"))
    {
      GLColor color(qcolor);
      float light_col[4] = {color.r, color.g, color.b, 1.0};

      if(paraName.contains("Ambient"))
      {
        glLightfv(GL_LIGHT0, GL_AMBIENT, light_col);
        glLightfv(GL_LIGHT1, GL_AMBIENT, light_col);

      }
      else if(paraName.contains("Diffuse"))
      {
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light_col);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, light_col);

      }
      else if(paraName.contains("Specular"))
      {
        glLightfv(GL_LIGHT0, GL_SPECULAR, light_col);
        glLightfv(GL_LIGHT1, GL_SPECULAR, light_col);
      }
      para->setValue(paraName, ColorValue(qcolor));
    }


    if (global_paraMgr.drawer.hasParameter(paraName))
    {
      global_paraMgr.drawer.setValue(paraName, ColorValue(qcolor));
    }
    else if (global_paraMgr.glarea.hasParameter(paraName))
    {
      global_paraMgr.glarea.setValue(paraName, ColorValue(qcolor));
    }


  } 
}

int GLArea::pickPoint(int x, int y, vector<int> &result, int width, int height,bool only_one)
{
  if((dataMgr.isSamplesEmpty() && !global_paraMgr.drawer.getBool("Use Pick Original"))
    && (dataMgr.isOriginalEmpty() && global_paraMgr.drawer.getBool("Use Pick Original")))
    return -1;

  if(width == 0 || height == 0) 
    return 0; 
  
  result.clear();
  long hits = 0;
  CMesh* target = NULL;
  if (global_paraMgr.drawer.getBool("Use Pick Original")){
    target = dataMgr.getCurrentOriginal();
  }else{
    target = dataMgr.getCurrentSamples();
  }

  int sz = target->vert.size();

  GLuint *selectBuf = new GLuint[sz * 5];

  //  static unsigned int selectBuf[16384];
  glSelectBuffer(sz * 5, selectBuf);
  glRenderMode(GL_SELECT);

  glInitNames();
  /* Because LoadName() won't work with no names on the stack */
  glPushName(-1);

  double mp[16];

  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT,viewport);
  glMatrixMode(GL_PROJECTION);
  glGetDoublev(GL_PROJECTION_MATRIX ,mp);
  //save former projection matrix
  glPushMatrix();
  glLoadIdentity();
  gluPickMatrix(x, y, width, height, viewport);
  glMultMatrixd(mp);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  //doPick = true;
  global_paraMgr.drawer.setValue("Doing Pick", BoolValue(true));
  //if (global_paraMgr.drawer.getBool("Use Pick Skeleton"))
  //{
  //	just_draw_skel_for_pick = true;
  //}
  paintGL();
  //just_draw_skel_for_pick = false;


  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  hits = glRenderMode(GL_RENDER);

  //xstring buf;
  if (hits <= 0)     return 0;

  vector<int> H;

  if (hits > 1 && global_paraMgr.drawer.getBool("Use Pick Mode2"))
  {
    hits--;
  }

  for(long ii = 0;ii < hits; ii++)
  {
    //H.push_back( std::pair<double,unsigned int>(selectBuf[ii*4+1]/4294967295.0,selectBuf[ii*4+3]));
    H.push_back(selectBuf[ii * 4 + 3]);
  }
  sort(H.begin(),H.end());


  //Only One Pick
  for(long i = 0 ;i < H.size();i++)
  {
    if(H[i] >= 0 && H[i] < sz)
    {
      result.push_back(H[i]);
      if(only_one)
        break;
    }
  }

  delete [] selectBuf;

  global_paraMgr.drawer.setValue("Doing Pick", BoolValue(false));

  if(only_one)
  {
    if(result.empty())
      return -1;
    else
      return result[0];
  }

  return result.size();
}


void GLArea::setView()
{
  glViewport(0,0, this->width(),this->height());
  curSiz.setWidth(this->width());
  curSiz.setHeight(this->height());

  GLfloat fAspect = (GLfloat)curSiz.width()/ curSiz.height();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  // This parameter is the one that controls:
  // HOW LARGE IS THE TRACKBALL ICON ON THE SCREEN.
  float viewRatio = 1.75f;
  float cameraDist = viewRatio / tanf(math::ToRad(fov*.5f));

  if(fov==5)
    cameraDist = 1000; // small hack for orthographic projection where camera distance is rather meaningless...
  nearPlane = cameraDist - 2.f*clipRatioNear;
  farPlane =  cameraDist + 10.f*clipRatioFar;
  if(nearPlane<=cameraDist*.1f) nearPlane=cameraDist*.1f;

  if (!takeSnapTile)
  {
    if(fov==5)	glOrtho( -viewRatio*fAspect, viewRatio*fAspect, -viewRatio, viewRatio, cameraDist - 2.f*clipRatioNear, cameraDist+2.f*clipRatioFar);
    else    		gluPerspective(fov, fAspect, nearPlane, farPlane);
  }
  else	setTiledView(fov, viewRatio, fAspect, nearPlane, farPlane, cameraDist);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0, 0, -cameraDist,0, 0, 0, 0, 1, 0);

}

void GLArea::pasteTile()
{
  glPushAttrib(GL_ENABLE_BIT);
  QImage tileBuffer=grabFrameBuffer(true).mirrored(false,true);

  if (snapBuffer.isNull())
    snapBuffer = QImage(tileBuffer.width() * ss.resolution, tileBuffer.height() * ss.resolution, tileBuffer.format());

  uchar *snapPtr = snapBuffer.bits() + (tileBuffer.bytesPerLine() * tileCol) + ((totalCols * tileRow) * tileBuffer.numBytes());
  uchar *tilePtr = tileBuffer.bits();

  for (int y=0; y < tileBuffer.height(); y++)
  {
    memcpy((void*) snapPtr, (void*) tilePtr, tileBuffer.bytesPerLine());
    snapPtr+=tileBuffer.bytesPerLine() * totalCols;
    tilePtr+=tileBuffer.bytesPerLine();
  }

  tileCol++;

  if (tileCol >= totalCols)
  {
    tileCol=0;
    tileRow++;

    if (tileRow >= totalRows)
    {
      QString outfile=QString("%1/%2%3.png")
        .arg(ss.outdir)
        .arg(ss.basename)
        .arg("");
      //.arg(ss.counter++,2,10,QChar('0'));
      bool ret = (snapBuffer.mirrored(false,true)).save(outfile,"PNG");

      takeSnapTile=false;
      recoverView();

      snapBuffer=QImage();
    }
  }
  update();
  glPopAttrib();
}

void GLArea::recoverView()
{
  int w = width();
  int h = height();

  glViewport(0, 0, (GLint)w, (GLint)h);  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();  

  float r = w/(float)h;
  gluPerspective(60, r, 0.1, 10);
  glMatrixMode(GL_MODELVIEW);  
}

void GLArea::setTiledView(GLdouble fovY, float viewRatio, float fAspect, GLdouble zNear, GLdouble zFar,  float cameraDist)
{
  if(fovY<=5)
  {
    GLdouble fLeft   = -viewRatio*fAspect;
    GLdouble fRight  =  viewRatio*fAspect;
    GLdouble fBottom = -viewRatio;
    GLdouble fTop    =  viewRatio;

    GLdouble tDimX = fabs(fRight-fLeft) / totalCols;
    GLdouble tDimY = fabs(fTop-fBottom) / totalRows;


    glOrtho(fLeft   + tDimX * tileCol, fLeft   + tDimX * (tileCol+1),     /* left, right */
      fBottom + tDimY * tileRow, fBottom + tDimY * (tileRow+1),     /* bottom, top */
      cameraDist - 2.f*clipRatioNear, cameraDist+2.f*clipRatioFar);
  }
  else
  {
    GLdouble fTop    = zNear * tan(math::ToRad(fovY/2.0));
    GLdouble fRight  = fTop * fAspect;
    GLdouble fBottom = -fTop;
    GLdouble fLeft   = -fRight;

    // tile Dimension
    GLdouble tDimX = fabs(fRight-fLeft) / totalCols;
    GLdouble tDimY = fabs(fTop-fBottom) / totalRows;

    glFrustum(fLeft   + tDimX * tileCol, fLeft   + tDimX * (tileCol+1),
      fBottom + tDimY * tileRow, fBottom + tDimY * (tileRow+1), zNear, zFar);
  }
}

void GLArea::saveSnapshot()
{
  is_paintGL_locked = true;

  double SnapResolutionScale = para->getDouble("Snapshot Resolution");
  ss.resolution = SnapResolutionScale;
  snapDrawScal = 1. / SnapResolutionScale;

  totalCols = totalRows = ss.resolution;
  tileRow = tileCol = 0;
  ss.setOutDir(current_snap_path);
  ss.GetSysTime();

  if (para->getBool("SnapShot Each Iteration"))
  {
    int snape_idx = para->getDouble("Snapshot Index");

    ss.basename = QString::number(snape_idx++,10); 
    double snap_id = para->getDouble("Snapshot Index");
    global_paraMgr.setGlobalParameter("Snapshot Index", DoubleValue(snap_id));
    emit needUpdateStatus();

    para->setValue("Snapshot Index", DoubleValue(snape_idx));
  }

  takeSnapTile = true;
  //updateGL();
  if (SnapResolutionScale > 1)
  {
    for (int i = 0; i < 4; i++)
    {
      is_paintGL_locked = false;
      updateGL();
      is_paintGL_locked = true;
    }
  }
  else
  {
    is_paintGL_locked = false;
    updateGL();
    is_paintGL_locked = true;
  }

  is_paintGL_locked = false;
}

void GLArea::runPoisson()
{
  //if (dataMgr.isOriginalEmpty())
  //{
  //	return;
  //}

  runPointCloudAlgorithm(poisson);

  para->setValue("Running Algorithm Name",
    StringValue(poisson.getParameterSet()->getString("Algorithm Name")));

  emit needUpdateStatus();
}

void GLArea::runCamera()
{
  if (dataMgr.isModelEmpty())  return ;

  runPointCloudAlgorithm(camera);

  para->setValue("Running Algorithm Name", 
    StringValue(camera.getParameterSet()->getString("Algorithm Name")));

  emit needUpdateStatus();
}

void GLArea::runNBV()
{
  if (dataMgr.isIsoPointsEmpty() && dataMgr.isSDFVoxelsEmpty()) return;

  runPointCloudAlgorithm(nbv);

  para->setValue("Running Algorithm Name", 
    StringValue(camera.getParameterSet()->getString("Algorithm Name")));
  emit needUpdateStatus();
}

void GLArea::runNormalSmoothing()
{
  if (dataMgr.isSamplesEmpty())
  {
    return;
  }

  for (int i = 0; i < global_paraMgr.norSmooth.getInt("Number Of Iterate"); i++)
  {
    runPointCloudAlgorithm(norSmoother);
  }

  emit needUpdateStatus();
}

void GLArea::outputColor(ostream& out, QColor& color)
{
  out << color.red() << "	" << color.green() << "	" << color.blue() << endl;
}

QColor GLArea::inputColor(istream& in)
{
  QColor color;
  int r, g, b;
  in >> r >> g >> b;
  color.setRgb(r, g, b);
  return color;
}


void GLArea::saveView(QString fileName)
{
  QString file = fileName;
  ofstream outfile(file.toStdString().c_str());

  char viewStr[100];
  trackball.ToAscii(viewStr);
  cout << "saveView" << viewStr << endl;
  outfile << viewStr << endl;

  CMesh* samples = dataMgr.getCurrentSamples();
  //Point3d cut_pos = samples->vert[0].P();


  outfile << global_paraMgr.data.getDouble("CGrid Radius") << endl;

  outfile << -1 << " " << -1 << " " << -1 << endl;	

  //outfile << cut_pos[0] << " " << cut_pos[1] << " " << cut_pos[2] << endl;	
  outfile << global_paraMgr.drawer.getDouble("Sample Dot Size") << endl;
  outfile << global_paraMgr.drawer.getDouble("Sample Draw Width") << endl;
  outfile << global_paraMgr.drawer.getDouble("Normal Line Width") << endl;
  outfile << global_paraMgr.drawer.getDouble("Normal Line Length") << endl;

  outfile << rotate_pos[0] << " " << rotate_pos[1] << " " << rotate_pos[2] << " " << endl;
  outfile << rotate_normal[0] << " " << rotate_normal[1] << " " << rotate_normal[2] << " " << endl;

  outfile << global_paraMgr.drawer.getDouble("Original Dot Size") << endl;

  outfile << global_paraMgr.drawer.getDouble("Skeleton Bone Width") << endl;
  outfile << global_paraMgr.drawer.getDouble("Skeleton Node Size") << endl;
  outfile << global_paraMgr.drawer.getDouble("Skeleton Branch Size") << endl;

  outputColor(outfile, global_paraMgr.drawer.getColor("Sample Point Color"));
  outputColor(outfile, global_paraMgr.drawer.getColor("Original Point Color"));
  outputColor(outfile, global_paraMgr.glarea.getColor("Light Ambient Color"));
  outputColor(outfile, global_paraMgr.glarea.getColor("Light Diffuse Color"));
  outputColor(outfile, global_paraMgr.glarea.getColor("Light Specular Color"));
  outputColor(outfile, global_paraMgr.drawer.getColor("Skeleton Bone Color"));
  outputColor(outfile, global_paraMgr.drawer.getColor("Skeleton Node Color"));
  outputColor(outfile, global_paraMgr.drawer.getColor("Skeleton Branch Color"));
  outputColor(outfile, global_paraMgr.drawer.getColor("Normal Line Color"));


  outfile << global_paraMgr.data.getDouble("CGrid Radius") << endl;
  //outfile << global_paraMgr.data.getDouble("Local Density Radius") << endl;
  //outfile << global_paraMgr.skeleton.getDouble("Snake Search Max Dist Blue") << endl;
  //outfile << global_paraMgr.skeleton.getDouble("Branch Search Max Dist Yellow") << endl;
  //outfile << global_paraMgr.skeleton.getDouble("Branches Merge Max Dist Orange") << endl;

  outfile << -1 << " " << -1 << " " << -1 << " " << -1 << endl;

  outfile << global_paraMgr.data.getDouble("Repulsion Mu") << endl;
  outfile << global_paraMgr.data.getDouble("Repulsion Mu2") << endl;


  trackball_light.ToAscii(viewStr);
  outfile << viewStr << endl;

  outfile << global_paraMgr.glarea.getDouble("Radius Ball Transparency") <<endl;

  outfile << global_paraMgr.drawer.getDouble("ISO Dot Size") << endl;

  outfile << global_paraMgr.glarea.getDouble("Grid ISO Color Scale") << endl;
  outfile << global_paraMgr.glarea.getDouble("ISO Interval Size") << endl;
  outfile << global_paraMgr.glarea.getDouble("Sample Confidence Color Scale") << endl;
  outfile << global_paraMgr.glarea.getDouble("Point ISO Value Shift") << endl;
  outfile << global_paraMgr.data.getDouble("Max Normalize Length") <<endl;
  outfile << dataMgr.original_center_point.X() << " "
    << dataMgr.original_center_point.Y() << " "
    << dataMgr.original_center_point.Z() << " "<<endl;

  outfile << global_paraMgr.glarea.getDouble("Grid ISO Value Shift") << endl;
  outfile << global_paraMgr.glarea.getDouble("Point ISO Color Scale") << endl;
  outfile << global_paraMgr.norSmooth.getDouble("Sharpe Feature Bandwidth Sigma") <<endl;
  outfile << global_paraMgr.norSmooth.getInt("PCA KNN") <<endl;
  outfile << global_paraMgr.camera.getDouble("Camera Resolution") <<endl;

  outfile << global_paraMgr.camera.getDouble("Camera Far Distance") <<endl;
  outfile << global_paraMgr.camera.getDouble("Camera Near Distance") <<endl;

  outfile.close();
}

void  GLArea::saveNBV(QString fileName)
{
  CMesh* nbv_candidates = dataMgr.getNbvCandidates();
  if (nbv_candidates == NULL) cout<<"No NBV!"<<endl;

  Point3f original_center_point = dataMgr.original_center_point;
  double max_normalize_length = global_paraMgr.data.getDouble("Max Normalize Length");

  GlobalFun::deleteIgnore(nbv_candidates);
  CMesh nbv;
  int index = 0;
  for (int i = 0; i < nbv_candidates->vert.size(); ++i)
  {
    CVertex &v = nbv_candidates->vert[i];

    CVertex t = v;
    //t.P() = (t.P() + original_center_point) * max_normalize_length;
    t.P() = (t.P()) * max_normalize_length;

    t.N().Normalize();
    t.m_index = index++;
    nbv.vert.push_back(t);
    nbv.bbox.Add(t.P());
  }
  nbv.vn = nbv.vert.size();

  QString fileName_nbvply = fileName;
  fileName_nbvply.replace(".ply", "_nbv.ply");
  dataMgr.savePly(fileName_nbvply, nbv);

  QString fileName_commands = fileName;
  fileName_commands.replace(".ply", ".txt");
}

void GLArea::loadView(QString fileName)
{

  ifstream infile(fileName.toStdString().c_str());
  double temp;

  string viewStr;
  infile >> viewStr;

  trackball.SetFromAscii(viewStr.c_str());

  double radius;
  infile >> radius;
  global_paraMgr.setGlobalParameter("CGrid Radius", DoubleValue(radius));

  infile >> temp >> temp >> temp;

  double dot_size;
  double quade_width;
  double normal_width;
  double normal_length;

  infile >> dot_size >> quade_width >> normal_width >> normal_length;
  global_paraMgr.drawer.setValue("Sample Dot Size", DoubleValue(dot_size));
  global_paraMgr.drawer.setValue("Sample Draw Width", DoubleValue(quade_width));
  global_paraMgr.drawer.setValue("Normal Line Width", DoubleValue(normal_width));
  global_paraMgr.drawer.setValue("Normal Line Length", DoubleValue(normal_length));

  Point3f r_pos, r_normal;
  infile >> r_pos[0] >> r_pos[1] >> r_pos[2];
  infile >> r_normal[0] >> r_normal[1] >> r_normal[2];

  rotate_pos = r_pos;
  rotate_normal = r_normal;

  //double temp;
  QColor c;
  if (!infile.eof())
  {
    infile >> dot_size;
    global_paraMgr.drawer.setValue("Original Dot Size", DoubleValue(dot_size));

    infile >> temp;
    global_paraMgr.drawer.setValue("Skeleton Bone Width", DoubleValue(temp));

    infile >> temp;
    global_paraMgr.drawer.setValue("Skeleton Node Size", DoubleValue(temp));

    infile >> temp;
    global_paraMgr.drawer.setValue("Skeleton Branch Size", DoubleValue(temp));

    global_paraMgr.drawer.setValue("Sample Point Color", ColorValue(inputColor(infile)));
    global_paraMgr.drawer.setValue("Original Point Color", ColorValue(inputColor(infile)));
    global_paraMgr.glarea.setValue("Light Ambient Color", ColorValue(inputColor(infile)));
    global_paraMgr.glarea.setValue("Light Diffuse Color", ColorValue(inputColor(infile)));
    global_paraMgr.glarea.setValue("Light Specular Color", ColorValue(inputColor(infile)));
    global_paraMgr.drawer.setValue("Skeleton Bone Color", ColorValue(inputColor(infile)));
    global_paraMgr.drawer.setValue("Skeleton Node Color", ColorValue(inputColor(infile)));
    global_paraMgr.drawer.setValue("Skeleton Branch Color", ColorValue(inputColor(infile)));
    global_paraMgr.drawer.setValue("Normal Line Color", ColorValue(inputColor(infile)));
  }

  if (!infile.eof())
  {
    infile >> temp;
    global_paraMgr.setGlobalParameter("CGrid Radius", DoubleValue(temp));

    infile >> temp;
    //global_paraMgr.setGlobalParameter("Local Density Radius", DoubleValue(temp));

  }

  if (!infile.eof())
  {
    infile >> temp;
    //global_paraMgr.skeleton.setValue("Snake Search Max Dist Blue", DoubleValue(temp));

    infile >> temp;
    //global_paraMgr.skeleton.setValue("Branch Search Max Dist Yellow", DoubleValue(temp));

    infile >> temp;
    //global_paraMgr.skeleton.setValue("Branches Merge Max Dist Orange", DoubleValue(temp));
  }
  
  initLight();

  if (!infile.eof())
  {
    double init_radius;
    infile >> init_radius;
    if (init_radius > 0)
    {
      global_paraMgr.setGlobalParameter("Initial Radius", DoubleValue(init_radius));
    }
    else
    {
      global_paraMgr.setGlobalParameter("Initial Radius", DoubleValue(global_paraMgr.data.getDouble("CGrid Radius")));
    }
  }

  infile >> temp;
  global_paraMgr.glarea.setValue("Radius Ball Transparency", DoubleValue(temp));

  infile >> temp;
  global_paraMgr.drawer.setValue("ISO Dot Size", DoubleValue(temp));

  infile >> temp;
  global_paraMgr.glarea.setValue("Grid ISO Color Scale", DoubleValue(temp));

  infile >> temp;
  global_paraMgr.glarea.setValue("ISO Interval Size", DoubleValue(temp));

  infile >> temp;
  global_paraMgr.glarea.setValue("Sample Confidence Color Scale", DoubleValue(temp));

  infile >> temp;
  global_paraMgr.glarea.setValue("Point ISO Value Shift", DoubleValue(temp));

  infile >> temp;
  global_paraMgr.data.setValue("Max Normalize Length", DoubleValue(temp));

  if (temp > 0)
  {
    cout << "temp" << endl;
    global_paraMgr.camera.setValue("Predicted Model Size", DoubleValue(temp/10.));
  }

  infile >> dataMgr.original_center_point[0]
         >> dataMgr.original_center_point[1]
         >> dataMgr.original_center_point[2];

  infile >> temp;
  global_paraMgr.glarea.setValue("Grid ISO Value Shift", DoubleValue(temp));
  infile >> temp;
  global_paraMgr.glarea.setValue("Point ISO Color Scale", DoubleValue(temp));
  infile >> temp;
  global_paraMgr.norSmooth.setValue("Sharpe Feature Bandwidth Sigma", DoubleValue(temp));
  infile >> temp;
  global_paraMgr.norSmooth.setValue("PCA KNN", IntValue(temp));
  infile >> temp;
  global_paraMgr.camera.setValue("Camera Resolution", DoubleValue(temp));


  infile >> temp;
  if (temp > 10.)
  {
    global_paraMgr.camera.setValue("Camera Far Distance", DoubleValue(temp));
  }
  infile >> temp;
  if (temp > 10.)
  {
    global_paraMgr.camera.setValue("Camera Near Distance", DoubleValue(temp));
  }

  infile.close();
  emit needUpdateStatus();
}

void GLArea::wheelEvent(QWheelEvent *e) 
{
  const int WHEEL_STEP = 120;
  double change_rate = 0.05;
  double change = (e->delta() > 0) ? (1 + change_rate) : (1 - change_rate);

  double change_rate2 = 0.015;
  double change2 = (e->delta() > 0) ? (1 + change_rate2) : (1 - change_rate2);

  double size_temp = 0.0;

  if (global_paraMgr.nbv.getBool("Use Confidence Separation")
    && (e->modifiers() & Qt::ControlModifier)
    && (e->modifiers() & Qt::AltModifier)
    && (e->modifiers() & Qt::ShiftModifier)
    && !global_paraMgr.poisson.getBool("Show Slices Mode")
    && !global_paraMgr.glarea.getBool("Show SDF Slices"))
  {
    size_temp = global_paraMgr.nbv.getDouble("Confidence Separation Value");
    size_temp *= change2;
    size_temp = (std::min)((std::max)(size_temp, 1e-10), 0.995);

    if (size_temp > 0.994)
    {
      size_temp = 0.9995;
    }
    global_paraMgr.nbv.setValue("Confidence Separation Value", DoubleValue(size_temp));
    cout << "Confidence Separation Value" << size_temp << endl;
    return;
  }

  if (global_paraMgr.poisson.getBool("Show Slices Mode"))
  {
    //����һ�𰴣��޸�slice�Ĵ�С
    if ((e->modifiers() & Qt::ShiftModifier)
      && (e->modifiers() & Qt::ControlModifier)
      && (e->modifiers() & Qt::AltModifier))
    {
      size_temp = global_paraMgr.poisson.getDouble("Show Slice Percentage");
      size_temp *= change;
      size_temp = (std::min)((std::max)(size_temp, 1e-10), 0.99);

      global_paraMgr.poisson.setValue("Show Slice Percentage", DoubleValue(size_temp));
      cout << "Slice Color Percentage" << size_temp << endl;

      global_paraMgr.poisson.setValue("Run Slice", BoolValue(true));
      runPoisson();
      global_paraMgr.poisson.setValue("Run Slice", BoolValue(false));
    }//����һ�𰴣��ֱ��޸�������ɫ
    else if( (e->modifiers() & Qt::ShiftModifier) && (e->modifiers() & Qt::ControlModifier) )
    {
      if (para->getBool("Show View Grid Slice"))
      {
        size_temp = global_paraMgr.glarea.getDouble("Grid ISO Color Scale");
        size_temp *= change2;
        size_temp = (std::max)(size_temp, 1e-10);

        global_paraMgr.glarea.setValue("Grid ISO Color Scale", DoubleValue(size_temp));
        cout << "Grid ISO Color Scale" << size_temp << endl;
      }
      else
      {
        size_temp = global_paraMgr.glarea.getDouble("Point ISO Color Scale");
        global_paraMgr.glarea.setValue("Point ISO Color Scale", DoubleValue(size_temp * change));
        cout << "Point ISO Color Scale = " << size_temp * change << endl;
      }
    }
    else if ((e->modifiers() & Qt::ShiftModifier) && (e->modifiers() & Qt::AltModifier))
    {
      size_temp = global_paraMgr.glarea.getDouble("Radius Ball Transparency") * change;
      global_paraMgr.glarea.setValue("Radius Ball Transparency", DoubleValue(size_temp));
      cout << "Radius Ball Transparency:  " << size_temp << endl;
      if(size_temp < 0)
      {
        size_temp = 0;
      }
    }
    else if( (e->modifiers() & Qt::AltModifier) && (e->modifiers() & Qt::ControlModifier) )
    {
      if (para->getBool("Show View Grid Slice"))
      {
        if (/*(para->getBool("Show ISO Points") || para->getBool("Show Samples")) &&*/ !para->getBool("Show Normal"))
        {
          size_temp = global_paraMgr.glarea.getDouble("Grid ISO Value Shift");
          if(e->delta() < 0)
          {
            size_temp += 0.02;
          }
          else
          {
            size_temp -= 0.02;
          }
          global_paraMgr.glarea.setValue("Grid ISO Value Shift", DoubleValue(size_temp));
          cout << "Grid ISO Value Shift" << size_temp << endl;
        }
        else
        {
          size_temp = global_paraMgr.drawer.getDouble("Normal Line Length");
          global_paraMgr.drawer.setValue("Normal Line Length", DoubleValue(size_temp * change));
        }
      }
      else
      {
        size_temp = global_paraMgr.glarea.getDouble("Point ISO Value Shift");
        if(e->delta() < 0)
        {
          size_temp += 0.02;
        }
        else
        {
          size_temp -= 0.02;
        }
        global_paraMgr.glarea.setValue("Point ISO Value Shift", DoubleValue(size_temp));
        cout << "Point ISO Value Shift" << size_temp << endl;
      }

    }
    else
    {//�������Ļ����޸Ķ�Ӧ���slice��λ��
      switch(e->modifiers())
      {
      case Qt::ControlModifier:
        {
          size_temp = global_paraMgr.poisson.getDouble("Current X Slice Position");
          size_temp *= change2;
          size_temp = (std::max)(size_temp, 1e-5);
          size_temp = (std::min)(size_temp, 0.999);

          global_paraMgr.poisson.setValue("Current X Slice Position", DoubleValue(size_temp)); 
          cout << "X position: " << size_temp << endl;

          global_paraMgr.poisson.setValue("Run Slice", BoolValue(true));
          runPoisson();
          global_paraMgr.poisson.setValue("Run Slice", BoolValue(false));

          emit needUpdateStatus();
        }
        break;
      case Qt::ShiftModifier:
        {
          size_temp = global_paraMgr.poisson.getDouble("Current Y Slice Position");
          size_temp *= change2;
          size_temp = (std::max)(size_temp, 1e-10);
          size_temp = (std::min)(size_temp, 1.0);
          global_paraMgr.poisson.setValue("Current Y Slice Position", DoubleValue(size_temp)); 
          cout << "Y position: " << size_temp << endl;

          global_paraMgr.poisson.setValue("Run Slice", BoolValue(true));
          runPoisson();
          global_paraMgr.poisson.setValue("Run Slice", BoolValue(false));

          emit needUpdateStatus();
        }
        break;
      case  Qt::AltModifier:
        {
          size_temp = global_paraMgr.poisson.getDouble("Current Z Slice Position");
          size_temp *= change2;
          size_temp = (std::max)(size_temp, 1e-5);
          size_temp = (std::min)(size_temp, 0.999);
          global_paraMgr.poisson.setValue("Current Z Slice Position", DoubleValue(size_temp));      
          cout << "Z position: " << size_temp << endl;

          global_paraMgr.poisson.setValue("Run Slice", BoolValue(true));
          runPoisson();
          global_paraMgr.poisson.setValue("Run Slice", BoolValue(false));

          emit needUpdateStatus();
        }
        break;
      default:
        {
          trackball.MouseWheel( e->delta()/ float(WHEEL_STEP));
        }
        break;
      }

      //global_paraMgr.poisson.setValue("Run Slice", BoolValue(true));
      //runPoisson();
      //global_paraMgr.poisson.setValue("Run Slice", BoolValue(false));

      return;
    }
    return;
  }

  if (global_paraMgr.glarea.getBool("Show SDF Slices"))
  {
    
    double sdf_voxel_size = global_paraMgr.nbv.getDouble("SDF Voxel Size");

    switch(e->modifiers())
    {
    case Qt::ControlModifier:
      {
        double x_pos = global_paraMgr.poisson.getDouble("Current X Slice Position");
        if(e->delta() < 0){
          x_pos -= sdf_voxel_size;
        }else{
          x_pos += sdf_voxel_size;
        }
        global_paraMgr.poisson.setValue("Current X Slice Position", DoubleValue(x_pos)); 
        cout << "X position: " << x_pos << endl;

        if (global_paraMgr.nbv.getBool("Show SDF Slice X"))
        {
          global_paraMgr.nbv.setValue("Run SDF Slice", BoolValue(true));
          runNBV();
          global_paraMgr.nbv.setValue("Run SDF Slice", BoolValue(true));
          emit needUpdateStatus();
        }
      }
      break;
    case Qt::ShiftModifier:
      {
        double y_pos = global_paraMgr.poisson.getDouble("Current Y Slice Position");
        if(e->delta() < 0){
          y_pos -= sdf_voxel_size;
        }else{
          y_pos += sdf_voxel_size;
        }
        global_paraMgr.poisson.setValue("Current Y Slice Position", DoubleValue(y_pos)); 
        cout << "Y position: " << y_pos << endl;
        if (global_paraMgr.nbv.getBool("Show SDF Slice Y"))
        {
          global_paraMgr.nbv.setValue("Run SDF Slice", BoolValue(true));
          runNBV();
          global_paraMgr.nbv.setValue("Run SDF Slice", BoolValue(true));
          emit needUpdateStatus();
        }        
      }
      break;
    case Qt::AltModifier:
      {
        double z_pos = global_paraMgr.poisson.getDouble("Current Z Slice Position");
        if(e->delta() < 0){
          z_pos -= sdf_voxel_size;
        }else{
          z_pos += sdf_voxel_size;
        }
        global_paraMgr.poisson.setValue("Current Z Slice Position", DoubleValue(z_pos)); 
        cout << "Z position: " << z_pos << endl;
        if (global_paraMgr.nbv.getBool("Show SDF Slice Z"))
        {
          global_paraMgr.nbv.setValue("Run SDF Slice", BoolValue(true));
          runNBV();
          global_paraMgr.nbv.setValue("Run SDF Slice", BoolValue(true));
          emit needUpdateStatus();
        }        
      }
      break;
    default:
      {
        trackball.MouseWheel( e->delta()/ float(WHEEL_STEP));
      }
      break;
    }
    return;
  }

  if( (e->modifiers() & Qt::AltModifier) && (e->modifiers() & Qt::ControlModifier) )
  {
    if ((para->getBool("Show ISO Points") || para->getBool("Show Samples")) && !para->getBool("Show Normal"))
    {
      size_temp = global_paraMgr.glarea.getDouble("Point ISO Value Shift");
      if(e->delta() < 0)
      {
        size_temp += 0.02;
      }
      else
      {
        size_temp -= 0.02;
      }
      global_paraMgr.glarea.setValue("Point ISO Value Shift", DoubleValue(size_temp));
      cout << "Point ISO Value Shift" << size_temp << endl;
    }
    else
    {
      size_temp = global_paraMgr.drawer.getDouble("Normal Line Length");
      global_paraMgr.drawer.setValue("Normal Line Length", DoubleValue(size_temp * change));
    }
  }
  else if( (e->modifiers() & Qt::ShiftModifier) && (e->modifiers() & Qt::ControlModifier) )
  {
    if (para->getBool("Show NBV Candidates") && para->getBool("Show NBV Ball"))
    {
      if (e->delta() < 0)
      {
        moveAllCandidates(true);
      }
      else
      {
        moveAllCandidates(false);
      }

    }
    if (global_paraMgr.drawer.getBool("Show Confidence Color"))
    {
      if (para->getBool("Show Samples"))
      {
        size_temp = global_paraMgr.glarea.getDouble("Sample Confidence Color Scale");
        global_paraMgr.glarea.setValue("Sample Confidence Color Scale", DoubleValue(size_temp * change));
        cout << "Sample Confidence Color Scale = " << size_temp * change << endl;
      }

      if (para->getBool("Show ISO Points"))
      {
        size_temp = global_paraMgr.glarea.getDouble("Point ISO Color Scale");
        global_paraMgr.glarea.setValue("Point ISO Color Scale", DoubleValue(size_temp * change));
        cout << "Point ISO Color Scale = " << size_temp * change << endl;
      }
    }

  }
  else if((e->modifiers() & Qt::ShiftModifier) && (e->modifiers() & Qt::AltModifier))
  {
    if (para->getBool("Show NBV Candidates") && para->getBool("Show NBV Ball"))
    {
      nbv_ball_slice *= change;
      cout << "nbv_ball_slice:  " << nbv_ball_slice << endl; 
    }
    else if (para->getBool("Show Normal") && para->getBool("Show NBV Candidates"))
    {
      size_temp = global_paraMgr.drawer.getDouble("Normal Line Width");
      size_temp *= change;
      size_temp = (std::max)(size_temp, 1e-6);
      global_paraMgr.drawer.setValue("Normal Line Width", DoubleValue(size_temp));
      cout << "Normal Line Width = " << size_temp << endl;
    }
    else
    {
      size_temp = global_paraMgr.glarea.getDouble("ISO Interval Size");
      size_temp *= change;
      size_temp = (std::max)(size_temp, 1e-6);
      global_paraMgr.glarea.setValue("ISO Interval Size", DoubleValue(size_temp));
      cout << "ISO scale step = " << size_temp << endl;
    }

  }
  else
  {
    switch(e->modifiers())
    {
    case Qt::ControlModifier:

      if(para->getBool("Show Original") && para->getBool("Show Original Sphere") )
      {
        size_temp = global_paraMgr.drawer.getDouble("Original Draw Width");
        global_paraMgr.drawer.setValue("Original Draw Width", DoubleValue(size_temp * change));
      }
      else if(para->getBool("Show Samples") &&  para->getBool("Show Samples Dot")
        &&para->getBool("Show Original") && para->getBool("Show Original Dot") )
      {
        size_temp = global_paraMgr.drawer.getDouble("Sample Dot Size") * change;
        if(size_temp < 1){
          size_temp = 1;
        }
        global_paraMgr.drawer.setValue("Sample Dot Size", DoubleValue(size_temp));

        size_temp = global_paraMgr.drawer.getDouble("Original Dot Size") * change;
        if(size_temp < 1)
        {
          size_temp = 1;
        }

        global_paraMgr.drawer.setValue("Original Dot Size", DoubleValue(size_temp));

        if (para->getBool("Show ISO Points"))
        {
          size_temp = global_paraMgr.drawer.getDouble("ISO Dot Size") * change;
          size_temp = (std::max)(size_temp, 1.0);
          global_paraMgr.drawer.setValue("ISO Dot Size", DoubleValue(size_temp));
        }
      }
      else if(para->getBool("Show Samples") &&  para->getBool("Show Samples Dot") )
      {
        size_temp = global_paraMgr.drawer.getDouble("Sample Dot Size") * change;
        if(size_temp < 1)
        {
          size_temp = 1;
        }
        global_paraMgr.drawer.setValue("Sample Dot Size", DoubleValue(size_temp));

        if (para->getBool("Show ISO Points"))
        {
          size_temp = global_paraMgr.drawer.getDouble("ISO Dot Size") * change;
          size_temp = (std::max)(size_temp, 1.0);
          global_paraMgr.drawer.setValue("ISO Dot Size", DoubleValue(size_temp));
        }
      }
      else if(para->getBool("Show Samples") &&  para->getBool("Show Samples Quad") )
      {
        size_temp = global_paraMgr.drawer.getDouble("Sample Draw Width") * change;
        if(size_temp < 0)
        {
          size_temp = 0.001;
        }
        //cout << "draw width: " <<size_temp << endl;
        global_paraMgr.drawer.setValue("Sample Draw Width", DoubleValue(size_temp));
      }
      else if(para->getBool("Show Original") && para->getBool("Show Original Dot") )
      {
        size_temp = global_paraMgr.drawer.getDouble("Original Dot Size") * change;
        if(size_temp < 1)
        {
          size_temp = 1;
        }
        global_paraMgr.drawer.setValue("Original Dot Size", DoubleValue(size_temp));
      }
      else  if (para->getBool("Show ISO Points") &&  para->getBool("Show Samples Dot") )
      {
        size_temp = global_paraMgr.drawer.getDouble("ISO Dot Size") * change;
        size_temp = (std::max)(size_temp, 1.0);
        global_paraMgr.drawer.setValue("ISO Dot Size", DoubleValue(size_temp));
      }     
      else
      {
        size_temp = global_paraMgr.drawer.getDouble("Sample Draw Width");
        global_paraMgr.drawer.setValue("Sample Draw Width", DoubleValue(size_temp * change));

        size_temp = global_paraMgr.drawer.getDouble("Original Draw Width");
        global_paraMgr.drawer.setValue("Original Draw Width", DoubleValue(size_temp * change));
      }
      emit needUpdateStatus();
      break;

    case Qt::ShiftModifier:
        size_temp = global_paraMgr.data.getDouble("Down Sample Num");
        global_paraMgr.setGlobalParameter("Down Sample Num", DoubleValue(size_temp * change));
        emit needUpdateStatus();

      break;

    case  Qt::AltModifier:
      size_temp = global_paraMgr.data.getDouble("CGrid Radius");
      global_paraMgr.setGlobalParameter("CGrid Radius", DoubleValue(size_temp * change));
      global_paraMgr.setGlobalParameter("Initial Radius", DoubleValue(size_temp * change));

      initSetting();
      break;

    default:
      trackball.MouseWheel( e->delta()/ float(WHEEL_STEP));
      break;
    }
  }

  updateGL();
}

void GLArea::mouseMoveEvent(QMouseEvent *e)
{
  if (isRightPressed)
  {
    isDragging = true;
  }

  GLint viewport[4];
  glGetIntegerv (GL_VIEWPORT, viewport);

  x2 = e->x();
  y2 = viewport[3] - e->y();

  if (isDefaultTrackBall())
  {
    trackball.MouseMove(e->x(),height()-e->y());
  }
  else 
  {
    trackball_light.MouseMove(e->x(),height()-e->y());
  }

  updateGL();
}

vcg::Trackball::Button QT2VCG(Qt::MouseButton qtbt,  Qt::KeyboardModifiers modifiers)
{
  int vcgbt= vcg::Trackball::BUTTON_NONE;
  if(qtbt & Qt::LeftButton		) vcgbt |= vcg::Trackball::BUTTON_LEFT;
  if(qtbt & Qt::RightButton		) vcgbt |= vcg::Trackball::BUTTON_RIGHT;
  if(qtbt & Qt::MidButton			) vcgbt |= vcg::Trackball::BUTTON_MIDDLE;
  if(modifiers & Qt::ShiftModifier		)	vcgbt |= vcg::Trackball::KEY_SHIFT;
  if(modifiers & Qt::ControlModifier ) vcgbt |= vcg::Trackball::KEY_CTRL;
  if(modifiers & Qt::AltModifier     ) vcgbt |= vcg::Trackball::KEY_ALT;
  return vcg::Trackball::Button(vcgbt);
}

void GLArea::mousePressEvent(QMouseEvent *e)
{
  if ((e->modifiers() & Qt::ShiftModifier) && (e->modifiers() & Qt::ControlModifier) &&
    (e->button()==Qt::LeftButton) )
    activeDefaultTrackball=false;
  else activeDefaultTrackball=true;

  if (isDefaultTrackBall())
  {
    if(e->button() == Qt::LeftButton)
      trackball.MouseDown(e->x(), height() - e->y(), QT2VCG(e->button(), e->modifiers() ) );     

    if(e->button() == Qt::RightButton) 
    {
      GLint viewport[4];
      glGetIntegerv (GL_VIEWPORT, viewport);
      x1 = e->x();
      y1 = viewport[3] - e->y();

      isRightPressed = true;
    }
  }
  else trackball_light.MouseDown(e->x(),height()-e->y(), QT2VCG(e->button(), Qt::NoModifier ) );

  //isDragging = true;
  update();
  updateGL();
}

void GLArea::mouseReleaseEvent(QMouseEvent *e)
{
  isDragging = false;

  if(e->button() == Qt::LeftButton)
    trackball.MouseUp(e->x(),height()-e->y(), QT2VCG(e->button(), e->modifiers() ) );

  if(e->button() == Qt::RightButton ) 
  {
    isRightPressed = false;
    GLint viewport[4];
    glGetIntegerv (GL_VIEWPORT, viewport);

    x2 = e->x();
    y2 = viewport[3] - e->y();

    double x3 = (x1 + x2)/2;
    double y3 = (y1 + y2)/2;

    if((e->modifiers() & Qt::ControlModifier))
    {
      pickPoint(x1,y1, fatherPickList, (x2-x1), (y1-y2), true);
      addPointByPick();
    }
    else if((e->modifiers() & Qt::AltModifier))
    {
      pickPoint(x1,y1, friendPickList, (x2-x1), (y1-y2), true);
      changePointByPick();
    }
    else if((e->modifiers() & Qt::ShiftModifier))
    {
      vector<int> no_use;
      addRBGPick( pickPoint(x1,y1, no_use, (x2-x1), (y1-y2)) );
    }
    else
    {
      pickPoint(x3,y3, pickList, abs(x2-x1), abs(y1-y2), !para->getBool("Multiply Pick Point"));
      friendPickList.clear();
      fatherPickList.clear();
      RGBPickList.assign(3, -1);
      RGB_counter = 0;

      if (!global_paraMgr.drawer.getBool("Use Pick Original") && !dataMgr.isSamplesEmpty() && !pickList.empty()){
        std::cout<<"pick SAMPLE points number: " <<pickList.size() <<std::endl;
      }

      if (global_paraMgr.drawer.getBool("Use Pick Original") && !dataMgr.isOriginalEmpty() && !pickList.empty()){
        std::cout<<"pick ORIGINAL points number: " <<pickList.size() <<std::endl;        
      }
    }
  }

  updateGL();
}

void GLArea::keyReleaseEvent ( QKeyEvent * e )
{
  if(e->key()==Qt::Key_Control) trackball.MouseUp(0,0, QT2VCG(Qt::NoButton, Qt::ControlModifier ) );
  if(e->key()==Qt::Key_Shift) trackball.MouseUp(0,0, QT2VCG(Qt::NoButton, Qt::ShiftModifier ) );
  if(e->key()==Qt::Key_Alt) trackball.MouseUp(0,0, QT2VCG(Qt::NoButton, Qt::AltModifier ) );
}

void  GLArea::removeOutliers()
{
  double outlier_percentage = global_paraMgr.data.getDouble("Outlier Percentage");
  int outlie_num = 20;

  if (global_paraMgr.glarea.getBool("Show Original"))
  {
    GlobalFun::removeOutliers(dataMgr.getCurrentOriginal(), global_paraMgr.data.getDouble("CGrid Radius"), outlier_percentage);
    cout<<"has removed original outliers"<<endl;
  }

  if (global_paraMgr.glarea.getBool("Show Samples"))
  {
    GlobalFun::removeOutliers(dataMgr.getCurrentSamples(), global_paraMgr.data.getDouble("CGrid Radius"), outlier_percentage);
    cout<<"has removed samples outliers"<<endl;
  }

  if (global_paraMgr.glarea.getBool("Show ISO Points"))
  {
    GlobalFun::removeOutliers(dataMgr.getCurrentIsoPoints(), global_paraMgr.data.getDouble("CGrid Radius"), outlier_percentage);
    cout<<"has removed ISO points outliers"<<endl;
  }

  updateUI();
}

void GLArea::savePickPointToIso()
{
  cout<<"save pick point to Iso" <<std::endl;

  CMesh* target;
  if (global_paraMgr.drawer.getBool("Use Pick Original")){
    target = dataMgr.getCurrentOriginal();
  }else{
    target = dataMgr.getCurrentSamples();
  }

  CMesh *iso_points = dataMgr.getCurrentIsoPoints();
  GlobalFun::clearCMesh(*iso_points);

  for(int i = 0; i < pickList.size(); ++i){
    CVertex v = target->vert[pickList[i]];
    v.is_fixed_sample = false;
    v.is_original = false;
    v.is_iso = true;
    v.m_index = i;
    iso_points->vert.push_back(v);
    iso_points->bbox.Add(v.P());
  }
  iso_points->vn = iso_points->vert.size();
  updateUI();
}

void GLArea::removePickPoint()
{
  CMesh* target;
  if (global_paraMgr.drawer.getBool("Use Pick Original")){
    target = dataMgr.getCurrentOriginal();
  }else{
    target = dataMgr.getCurrentSamples();
  }

  CMesh::VertexIterator vi;
  int j = 0;
  for(vi = target->vert.begin(); vi != target->vert.end(); ++vi, ++j)
  {
    vi->m_index = j;
    vi->neighbors.clear();
  }

  if (!para->getBool("Multiply Pick Point"))
  {
    for(int i = 0; i < pickList.size(); i++) 
    {
      if(pickList[i] < 0 || pickList[i] >= target->vert.size())
        continue;

      CVertex &v = target->vert[pickList[i]]; 
      target->vert.erase(target->vert.begin() + v.m_index);
    }
    target->vn = target->vert.size();
  }
  else
  {
    for (int i = 0; i < pickList.size(); i++) {
      target->vert[pickList[i]].is_ignore = true;
    }

    vector<CVertex> save_sample_vert;
    for (int i = 0; i < target->vert.size(); i++) {
      CVertex& v = target->vert[i];
      if (!v.is_ignore)
      {
        save_sample_vert.push_back(v);
      }
    }

    target->vert.clear();
    for (int i = 0; i < save_sample_vert.size(); i++) {
      target->vert.push_back(save_sample_vert[i]);
    }
    target->vn = target->vert.size();
  }

  //update sample index
  for(j = 0, vi = target->vert.begin(); vi != target->vert.end(); ++vi, ++j) {
    vi->m_index = j;
  }

  cleanPickPoints();
}

void GLArea::addPointByPick()
{
  if (dataMgr.isSamplesEmpty())
    return;

  if(pickList.empty() || fatherPickList.empty())
    return;

  CMesh* mesh = dataMgr.getCurrentSamples();
  CVertex newv;

  for(int ii = 0; ii < pickList.size(); ii++) 
  {
    int i = pickList[ii];

    if(i < 0 )
      return;

    CVertex &v = mesh->vert[i];   

    for(int jj = 0; jj < fatherPickList.size(); jj++)
    {
      int j = fatherPickList[jj];

      if(j < 0)
        return;

      CVertex &t = mesh->vert[j];

      if(v.P() == t.P())
      {
        cout << "overlap choose point!!" << endl;
        return;
      }

      newv = v;
      newv.P() = (v.P() + t.P()) / 2.0;  
      newv.m_index = mesh->vert.size();
      mesh->vert.push_back(newv);
    }
  }

  mesh->vn = mesh->vert.size();

  updateGL();
}

void GLArea::cleanPickPoints()
{
  pickList.clear();
  friendPickList.clear();
  fatherPickList.clear();
  RGBPickList.clear();
  glDrawer.cleanPickPoint();
  //glDrawer.weight_color_original.clear();
}

void GLArea::changePointByPick()
{
  if (dataMgr.isSamplesEmpty())
    return;

  CMesh* mesh = dataMgr.getCurrentSamples();
  CVertex newv;

  for(int ii = 0; ii < pickList.size(); ii++) 
  {
    int i = pickList[ii];

    if(i < 0 )
      continue;

    CVertex &v = mesh->vert[i];
    Point3f &p = v.P();     

    for(int jj = 0; jj < friendPickList.size(); jj++)
    {
      int j = friendPickList[jj];

      if(j < 0)
        continue;

      CVertex &t = mesh->vert[j];
      Point3f &q = t.P();
      v.N() = t.N();

      break;

    }
  }

  updateGL();
}


void GLArea::addRBGPick(int pick_index)
{
  vector<Point3f> RGB_normals;

  if(dataMgr.isSamplesEmpty())
    return;

  CMesh* mesh = dataMgr.getCurrentSamples();
  if(pick_index < 0 || pick_index >= mesh->vert.size())
  {
    cout << "pick GRB wrong!" << endl;
    return;
  }

  rotate_normal = mesh->vert[pick_index].N();
  rotate_pos = mesh->vert[pick_index].P();


  RGBPickList[RGB_counter++] = pick_index;

  if(RGB_counter == 3)
  {
    cout << "constructRGBNormals" << endl;

    vector<Point3f> normals;
    for(int i = 0; i < 3; i++)
    {
      normals.push_back(mesh->vert[RGBPickList[i]].N());
    }

    RGB_normals.assign(3, Point3f(0.0, 0.0, 0.0));
    RGB_normals[0] = normals[0];
    RGB_normals[1] = normals[0] ^ normals[2];

    if(RGB_normals[1] * normals[1] < 0)
      RGB_normals[1] = -RGB_normals[1];

    RGB_normals[2] = normals[0]	^ normals[1];
    if(RGB_normals[2] * normals[2] < 0)
      RGB_normals[2] = -RGB_normals[2];


    glDrawer.setRGBNormals(RGB_normals);
    RGB_counter = 0;
  }

}

void GLArea::readRGBNormal(QString fileName)
{
  ifstream infile(fileName.toStdString().c_str());

  vector<Point3f> rgb_normal(3, Point3f(0, 0, 0));
  for(int i = 0; i < rgb_normal.size(); i++)
  {
    infile >> rgb_normal[i].X() >> rgb_normal[i].Y() >> rgb_normal[i].Z();
  }

  glDrawer.setRGBNormals(rgb_normal);

  infile.clear();
}

void GLArea::rotatingAnimation()
{
  if(-rotate_angle > 361)
    return;

  need_rotate = true;
  //rotate_angle -= 1.7f;
  rotate_angle -= 7.f;
  updateGL();


  cout << rotate_angle << endl;

  double total_time = 6500;
  double sleep_time = 45;
  double delta = 360 / (total_time / sleep_time);
  do
  {
    rotate_angle -= delta;
    updateGL();
    Sleep(sleep_time);
  }while(-rotate_angle < 361);

  rotate_angle += 360;
}

void GLArea::figureSnapShot()
{
  if (para->getBool("SnapShot Each Iteration"))
  {

    is_figure_shot = true;
    //quickSaveSkeleton();
    saveSnapshot();
    is_figure_shot = false;
  }

}

void GLArea::drawCandidatesConnectISO()
{
  double width = global_paraMgr.drawer.getDouble("Normal Line Width") ;
  double length = global_paraMgr.drawer.getDouble("Normal Line Length");
  //double half_length = normal_length / 2.0;
  QColor qcolor = global_paraMgr.drawer.getDouble("Original Point Color");
  double near_dist = global_paraMgr.camera.getDouble("Camera Near Distance");
  double max_length = global_paraMgr.data.getDouble("Max Normalize Length");
  near_dist*=10;
  near_dist/=max_length;

  CMesh* candidates = dataMgr.getNbvCandidates();
  for (int i = 0; i < candidates->vert.size(); i++)
  {
    CVertex& v = candidates->vert[i];

    glLineWidth(width* 3); 
    GLColor color(qcolor);

    glColor4f(color.r, color.g, color.b, 1);  
    glColor3f(0, 1, 0);

    Point3f p = v.P(); 
    //Point3f m0 = v.N();
    //Point3f m1 = v.eigen_vector0;
    //Point3f m2 = v.eigen_vector1;

    //int remember_index = v.remember_iso_index;
    Point3f tp = v.P() + v.N() *  near_dist;
    // Z
    glBegin(GL_LINES);	
    glVertex3d(p[0], p[1], p[2]);
    glVertex3f(tp[0], tp[1], tp[2]);
    glEnd(); 
  }

  glLineWidth(width);
}


void GLArea::moveAllCandidates(bool is_forward)
{
  CMesh* nbv_candidates = dataMgr.getNbvCandidates();
  double step = 0.01;
  for (int i = 0; i < nbv_candidates->vert.size(); i++)
  {
    CVertex& v = nbv_candidates->vert[i];
    if (is_forward)
    {
      v.P() += v.N() * step;
    }
    else
    {
      v.P() -= v.N() * step;
    }
  }
}