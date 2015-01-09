#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent, Qt::WFlags flags)
: QMainWindow(parent, flags)
{
	cout << "MainWindow constructed" << endl;
	ui.setupUi(this);
	area = new GLArea(this);
	setCentralWidget(area);

	init();
	initWidgets();
	createActionGroups();
	iniStatusBar();
	initConnect();

  area->loadDefaultModel();
  //to avoid unclear UI at start
  lightOnOff(false);
  lightOnOff(true);
  lightOnOff(false);
}

MainWindow::~MainWindow()
{
	if(area) delete area;
	area = NULL;
}

void MainWindow::initWidgets()
{
  ui.actionShow_Model->setChecked(global_paraMgr.glarea.getBool("Show Model"));
	ui.actionShow_Samples->setChecked(global_paraMgr.glarea.getBool("Show Samples"));
	ui.actionShow_Original->setChecked(global_paraMgr.glarea.getBool("Show Original"));
	ui.actionShow_Normals->setChecked(global_paraMgr.glarea.getBool("Show Normal"));
	ui.actionShow_Neighborhood_Ball->setChecked(global_paraMgr.glarea.getBool("Show Radius"));
	ui.actionShow_All_Raidus->setChecked(global_paraMgr.glarea.getBool("Show All Radius"));	
	ui.actionCull_Points->setChecked(global_paraMgr.drawer.getBool("Need Cull Points"));
	ui.actionShow_Individual_Color->setChecked(global_paraMgr.drawer.getBool("Show Individual Color"));
    ui.actionShow_Confidence_Color->setChecked(global_paraMgr.drawer.getBool("Show Confidence Color"));

	ui.actionShow_Sample_Quads->setChecked(paras->glarea.getBool("Show Samples Quad"));
	ui.actionShow_Sample_Dot->setChecked(paras->glarea.getBool("Show Samples Dot"));
	ui.actionShow_Sample_Circle->setChecked(paras->glarea.getBool("Show Samples Circle"));
	ui.actionShow_Sample_Sphere->setChecked(paras->glarea.getBool("Show Samples Sphere"));
	
	ui.actionShow_Original_Quad->setChecked(paras->glarea.getBool("Show Original Quad"));
	ui.actionShow_Original_Dot->setChecked(paras->glarea.getBool("Show Original Dot"));
	ui.actionShow_Original_Circle->setChecked(paras->glarea.getBool("Show Original Circle"));
	ui.actionShow_Original_Sphere->setChecked(paras->glarea.getBool("Show Original Sphere"));
	
	ui.actionShow_Normal_Color->setChecked(paras->drawer.getBool("Use Color From Normal"));
	ui.actionSnap_Each_Iteration->setChecked(paras->glarea.getBool("SnapShot Each Iteration"));
	ui.actionNo_Snap_Radius->setChecked(paras->glarea.getBool("No Snap Radius"));
  ui.actionShow_Skeleton->setChecked(paras->glarea.getBool("Show Skeleton"));
  ui.actionShow_colorful_branches->setChecked(paras->drawer.getBool("Use Differ Branch Color"));
  ui.actionShow_Box->setChecked(paras->glarea.getBool("Show Bounding Box"));
	
  ui.actionShow_ISO->setChecked(paras->glarea.getBool("Show ISO Points"));
  ui.actionUse_ISO_Interval->setChecked(paras->glarea.getBool("Use ISO Interval"));
  ui.actionShow_View_Grids->setChecked(paras->glarea.getBool("Show View Grids"));
  ui.actionShow_NBV_Candidates->setChecked(paras->glarea.getBool("Show NBV Candidates"));
  ui.actionShow_Scan_Candidates->setChecked(paras->glarea.getBool("Show Scan Candidates"));
  ui.actionShow_Scan_History->setChecked(paras->glarea.getBool("Show Scan History"));
  ui.actionShow_Current_Scanned_Mesh->setChecked(paras->glarea.getBool("Show Scanned Mesh"));
  ui.actionShow_Poisson_Surface->setChecked(paras->glarea.getBool("Show Poisson Surface"));
  ui.actionShow_NBV_Label->setChecked(paras->glarea.getBool("Show NBV Label"));
  ui.actionShow_NBV_Ball->setChecked(paras->glarea.getBool("Show NBV Ball"));
  //sdf related
  ui.actionShow_SDF_Voxels->setChecked(paras->glarea.getBool("Show SDF Voxels"));
  ui.actionShow_SDF_Voxels->setChecked(paras->glarea.getBool("Show SDF Slices"));
  ui.actionShow_GraphCut_Related->setChecked(paras->glarea.getBool("Show GraphCut Related"));
}

void MainWindow::initConnect()
{
	if (!connect(area,SIGNAL(needUpdateStatus()),this,SLOT(updateStatusBar())))
	{
		cout << "can not connect signal" << endl;
	}
  connect(ui.actionEvaluation, SIGNAL(triggered()), this, SLOT(evaluation()));
  connect(ui.actionCompute_Normal_For_Poisson_Surface, SIGNAL(triggered()), this, SLOT(computeNormalForPoissonSurface()));
  connect(ui.actionConvert_ply_to_obj, SIGNAL(triggered()), this, SLOT(convertPlyToObj()));
  connect(ui.actionSave_Parameter, SIGNAL(triggered()), this, SLOT(savePara()));
	connect(ui.actionImport_Ply, SIGNAL(triggered()), this, SLOT(openFile()));
	connect(ui.actionSave_Ply, SIGNAL(triggered()), this, SLOT(saveFile()));
  connect(ui.actionRemove_Outlier, SIGNAL(triggered()), this, SLOT(removeOutliers()));
	connect(ui.actionDownSample, SIGNAL(triggered()), this, SLOT(downSample()));
  connect(ui.actionTransformToGroundAxis, SIGNAL(triggered()), this, SLOT(transformToGroundAxis()));
	connect(ui.actionSubSample, SIGNAL(triggered()), this, SLOT(subSample()));
	connect(ui.actionNormalize, SIGNAL(triggered()), this, SLOT(normalizeData()));
	connect(ui.actionClear_Data, SIGNAL(triggered()), this, SLOT(clearData()));
	connect(ui.actionImport_Image, SIGNAL(triggered()), this, SLOT(openImage()));
	connect(ui.actionSave_View, SIGNAL(triggered()), this, SLOT(saveView()));
  connect(ui.actionSave_NBV, SIGNAL(triggered()), this, SLOT(saveNBV()));
  connect(ui.actionSave_NBV_Grids, SIGNAL(triggered()), this, SLOT(saveViewGridsForVoreen()));
	connect(ui.actionSave_Skel, SIGNAL(triggered()), this, SLOT(saveSkel()));
  connect(ui.actionQianSample, SIGNAL(triggered()), this, SLOT(getQianSample()));

	connect(ui.actionSnapShot, SIGNAL(triggered()), this, SLOT(saveSnapshot()));
	connect(ui.actionRun_PCA, SIGNAL(triggered()), this, SLOT(runPCA_Normal()));
	connect(ui.actionReorientate, SIGNAL(triggered()), this, SLOT(reorientateNormal()));
  connect(ui.actionPoisson, SIGNAL(triggered()), this, SLOT(showPoissonParaDlg()));
  connect(ui.actionCamera, SIGNAL(triggered()), this, SLOT(showCameraParaDlg()));

	connect(ui.actionInitial_Sampling_2, SIGNAL(triggered()), this, SLOT(initialSampling()));
	connect(ui.actionAuto_Play, SIGNAL(triggered()), this, SLOT(autoPlaySkeleton()));
	connect(ui.actionStop, SIGNAL(triggered()), this, SLOT(setStop()));
	connect(ui.actionStep, SIGNAL(triggered()), this, SLOT(stepPlaySkeleton()));
	connect(ui.actionJump, SIGNAL(triggered()), this, SLOT(jumpPlaySkeleton()));
	connect(ui.actionRecompute_Quad,SIGNAL(triggered()),this,SLOT(recomputeQuad()));
	
	connect(ui.actionLight_On_Off, SIGNAL(toggled(bool)), this, SLOT(lightOnOff(bool)));
	connect(ui.actionShow_Individual_Color, SIGNAL(toggled(bool)), this, SLOT(showIndividualColor(bool)));
	connect(ui.actionShow_Samples,SIGNAL(toggled(bool)),this,SLOT(showSamples(bool)));
  connect(ui.actionShow_Model, SIGNAL(toggled(bool)), this, SLOT(showModel(bool)));
	connect(ui.actionShow_Original,SIGNAL(toggled(bool)),this,SLOT(showOriginal(bool)));
	connect(ui.actionShow_Normals,SIGNAL(toggled(bool)),this,SLOT(showNormals(bool)));
	connect(ui.actionShow_Neighborhood_Ball,SIGNAL(toggled(bool)),this,SLOT(showNeighborhoodBall(bool)));
	connect(ui.actionShow_All_Raidus,SIGNAL(toggled(bool)),this,SLOT(showAllNeighborhoodBall(bool)));
	connect(ui.actionCull_Points,SIGNAL(toggled(bool)),this,SLOT(cullPoints(bool)));
	connect(ui.actionShow_Normal_Color,SIGNAL(toggled(bool)),this,SLOT(showNormalColor(bool)));
	connect(ui.actionSnap_Each_Iteration,SIGNAL(toggled(bool)),this,SLOT(setSnapshotEachIteration(bool)));
	connect(ui.actionNo_Snap_Radius,SIGNAL(toggled(bool)),this,SLOT(setNoSnapshotWithRadius(bool)));
  connect(ui.actionShow_colorful_branches,SIGNAL(toggled(bool)),this,SLOT(showColorfulBranches(bool)));
  connect(ui.actionShow_Box,SIGNAL(toggled(bool)),this,SLOT(showBox(bool)));  
  connect(ui.actionShow_Confidence_Color, SIGNAL(toggled(bool)), this, SLOT(showConfidenceColor(bool)));
  connect(ui.actionShow_NBV_Label, SIGNAL(toggled(bool)), this, SLOT(showNBVLables(bool)));
  connect(ui.actionShow_NBV_Ball, SIGNAL(toggled(bool)), this, SLOT(showNBVBall(bool)));
  
  connect(ui.actionShow_ISO,SIGNAL(toggled(bool)),this,SLOT(showIsoPoints(bool)));
  connect(ui.actionUse_ISO_Interval,SIGNAL(toggled(bool)),this,SLOT(useIsoInterval(bool)));

  connect(ui.actionShow_View_Grids, SIGNAL(toggled(bool)), this, SLOT(showViewGrids(bool)));
  connect(ui.actionShow_NBV_Candidates, SIGNAL(toggled(bool)), this, SLOT(showNBVCandidates(bool)));
  connect(ui.actionShow_Scan_Candidates, SIGNAL(toggled(bool)), this, SLOT(showScanCandidates(bool)));
  connect(ui.actionShow_Scan_History, SIGNAL(toggled(bool)), this, SLOT(showScanHistory(bool)));
  connect(ui.actionShow_Current_Scanned_Mesh, SIGNAL(toggled(bool)), this, SLOT(showScannedMesh(bool)));
  connect(ui.actionShow_Poisson_Surface, SIGNAL(toggled(bool)), this, SLOT(showPoissonSurface(bool)));  
	connect(sample_draw_type,SIGNAL(triggered(QAction *)),this,SLOT(setSmapleType(QAction *)));
	connect(original_draw_type,SIGNAL(triggered(QAction *)),this,SLOT(setOriginalType(QAction *)));

	connect(ui.actionSample_Color,SIGNAL(triggered()),this,SLOT(sampleColor()));
	connect(ui.actionOriginal_Color,SIGNAL(triggered()),this,SLOT(originalColor()));
	connect(ui.actionBackground_Color,SIGNAL(triggered()),this,SLOT(backGroundColor()));
	connect(ui.actionAmbient_Color,SIGNAL(triggered()),this,SLOT(ambientColor()));
	connect(ui.actionDiffuse_Color,SIGNAL(triggered()),this,SLOT(diffuseColor()));
	connect(ui.actionSpecular_Color,SIGNAL(triggered()),this,SLOT(specularColor()));
	connect(ui.actionNormal_Color,SIGNAL(triggered()),this,SLOT(normalColor()));
	connect(ui.actionFeature_Color,SIGNAL(triggered()),this,SLOT(featureColor()));

	connect(ui.actionErase_Pick,SIGNAL(triggered()),this,SLOT(removePickPoints()));
  connect(ui.actionSave_Field_Points,SIGNAL(triggered()),this,SLOT(saveFieldPoints()));
	
  connect(ui.actionSwitch_Sample_Original,SIGNAL(triggered()),this,SLOT(switchSampleOriginal()));
  connect(ui.actionSwitch_Sample_with_ISO,SIGNAL(triggered()),this,SLOT(switchSampleISO()));
  connect(ui.actionSwitch_Sample_NBV,SIGNAL(triggered()),this,SLOT(switchSampleNBV()));
  connect(ui.actionAdd_Sample_To_Original,SIGNAL(triggered()),this,SLOT(addSamplesToOriginal()));
  connect(ui.actionRemove_Ignore,SIGNAL(triggered()),this,SLOT(deleteIgnore()));
  connect(ui.actionRecover_Ignore,SIGNAL(triggered()),this,SLOT(recoverIgnore()));
  connect(ui.actionSwitch_History_NBV,SIGNAL(triggered()),this,SLOT(switchHistoryNBV()));
  connect(ui.actionAdd_NBV_To_History,SIGNAL(triggered()),this,SLOT(addNBVtoHistory()));
  connect(ui.actionSICP_With_Normal, SIGNAL(triggered()), this, SLOT(SICPWithNormal()));

  //connect(ui.actionPoisson_test,SIGNAL(triggered()),this,SLOT(poissonTest()));
  //connect(ui.actionPoisson_test_all,SIGNAL(triggered()),this,SLOT(poissonTestAll()));

  //sdf related
  connect(ui.actionShow_SDF_Voxels, SIGNAL(toggled(bool)), this, SLOT(showSDFVoxels(bool)));
  connect(ui.actionShow_SDF_Slices, SIGNAL(toggled(bool)), this, SLOT(showSDFSlices(bool)));
  connect(ui.actionShow_GraphCut_Related, SIGNAL(toggled(bool)), this, SLOT(showGraphCutRelated(bool)));

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this->area, SLOT(update()));
	timer->start(100);
}

void MainWindow::iniStatusBar()
{
	QStatusBar *status_bar = statusBar();

	original_size_label = new QLabel;
	original_size_label->setMinimumSize(200,30);
	original_size_label->setIndent(2);
	original_size_label->setFrameShape(QFrame::NoFrame);
	original_size_label->setFrameShadow(QFrame::Plain);

	downSample_num_label = new QLabel;
	downSample_num_label->setMinimumSize(200,30);
	downSample_num_label->setFrameShape(QFrame::NoFrame);
	downSample_num_label->setFrameShadow(QFrame::Plain);

	radius_label = new QLabel;
	radius_label->setMinimumSize(200,30);
	radius_label->setFrameShape(QFrame::NoFrame);
	radius_label->setFrameShadow(QFrame::Plain);
	
	sample_size_lable = new QLabel;
	sample_size_lable->setMinimumSize(200,30);
	sample_size_lable->setFrameShape(QFrame::NoFrame);
	sample_size_lable->setFrameShadow(QFrame::Plain);

  iso_size_lable = new QLabel;
  iso_size_lable->setMinimumSize(200,30);
  iso_size_lable->setFrameShape(QFrame::NoFrame);
  iso_size_lable->setFrameShadow(QFrame::Plain);

	error_label = new QLabel;
	error_label->setMinimumSize(200,30);
	error_label->setFrameShape(QFrame::NoFrame);
	error_label->setFrameShadow(QFrame::Plain);

  iteration_label = new QLabel;
  iteration_label->setMinimumSize(200,30);
  iteration_label->setFrameShape(QFrame::NoFrame);
  iteration_label->setFrameShadow(QFrame::Plain);

	updateStatusBar();

	status_bar->addWidget(downSample_num_label);
  status_bar->addWidget(iteration_label);
	status_bar->addWidget(error_label);
	status_bar->addWidget(radius_label);
	status_bar->addWidget(original_size_label);
	status_bar->addWidget(sample_size_lable);
  status_bar->addWidget(iso_size_lable);
}

void MainWindow::updateStatusBar()
{
	QString title = strTitle +  " -- " + area->dataMgr.cur_file_name
    ;
	setWindowTitle(title);

	int o_size = 0;
	if(!area->dataMgr.isOriginalEmpty())
		o_size = area->dataMgr.getCurrentOriginal()->vert.size();
	QString strOriginal = "Original points: " + QString::number(o_size); 
	original_size_label->setText(strOriginal);

	int s_size = 0;
	if(!area->dataMgr.isSamplesEmpty())
		s_size = area->dataMgr.getCurrentSamples()->vert.size();

	QString str = "Sample points: " + QString::number(s_size);
	sample_size_lable->setText(str);

  int i_size = 0;
  if(!area->dataMgr.isIsoPointsEmpty())
    i_size = area->dataMgr.getCurrentIsoPoints()->vert.size();
  QString str2 = "ISO points: " + QString::number(i_size);
  iso_size_lable->setText(str2);

	double nDown = global_paraMgr.data.getDouble("Down Sample Num");
	QString strSub = "Down: " + QString::number(nDown);
	downSample_num_label->setText(strSub);

	double radius  = global_paraMgr.data.getDouble("CGrid Radius");
	QString strRadius = "Radius: " + QString::number(radius);
	radius_label->setText(strRadius);

	update();
	repaint();
}


void MainWindow::createActionGroups()
{
	sample_draw_type = new QActionGroup(this);
	ui.actionShow_Sample_Quads->setActionGroup(sample_draw_type);
	ui.actionShow_Sample_Dot->setActionGroup(sample_draw_type);
	ui.actionShow_Sample_Circle->setActionGroup(sample_draw_type);
	ui.actionShow_Sample_Sphere->setActionGroup(sample_draw_type);

	original_draw_type = new QActionGroup(this);
	ui.actionShow_Original_Quad->setActionGroup(original_draw_type);
	ui.actionShow_Original_Dot->setActionGroup(original_draw_type);
	ui.actionShow_Original_Circle->setActionGroup(original_draw_type);
	ui.actionShow_Original_Sphere->setActionGroup(original_draw_type);

	QString str = strTitle + " -- Welcome!";
	setWindowTitle(str);
}

void MainWindow::init()
{
	strTitle = "Quality-Driven-Poisson-Guided-Autoscanning";
  paraDlg_Poisson = NULL;
  paraDlg_Camera = NULL;
	paras = &global_paraMgr;
}

void MainWindow::showPoissonParaDlg()
{
  if(paraDlg_Poisson != 0)
  {
    paraDlg_Poisson->close();
    delete paraDlg_Poisson;
  }

  paraDlg_Poisson = new StdParaDlg(paras, area, this);
  paraDlg_Poisson->setAllowedAreas(Qt::LeftDockWidgetArea
    | Qt::RightDockWidgetArea);
  addDockWidget(Qt::LeftDockWidgetArea,paraDlg_Poisson);
  paraDlg_Poisson->setFloating(false);
  paraDlg_Poisson->hide();
  paraDlg_Poisson->showPoissonParaDlg();
}

void MainWindow::showCameraParaDlg()
{
  if (paraDlg_Camera != NULL)
  {
    paraDlg_Camera->close();
    delete paraDlg_Camera;
  }

  paraDlg_Camera = new StdParaDlg(paras, area, this);
  paraDlg_Camera->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
  addDockWidget(Qt::RightDockWidgetArea, paraDlg_Camera);
  paraDlg_Camera->setFloating(false);
  paraDlg_Camera->hide();
  paraDlg_Camera->showCameraParaDlg();
}

void MainWindow::autoPlaySkeleton()
{

}

void MainWindow::stepPlaySkeleton()
{

}

void MainWindow::jumpPlaySkeleton()
{

}

void MainWindow::initialSampling()
{

}

void MainWindow::setStop()
{

}

void MainWindow::openFile()
{
	QString file = QFileDialog::getOpenFileName(this, "Select a ply file", "", "*.ply");
	if(!file.size()) return;

  area->dataMgr.loadPlyToModel(file);
	area->dataMgr.loadPlyToSample(file);
	area->initAfterOpenFile();
	area->updateGL();
}

void MainWindow::openImage()
{
	QString file = QFileDialog::getOpenFileName(this, "Select a ply file", "", "");
	if(!file.size()) return;

	area->dataMgr.loadImage(file);
	area->initAfterOpenFile();
	area->updateGL();
}

void MainWindow::saveFile()
{
	QString file = QFileDialog::getSaveFileName(this, "Save samples as", "", "*.ply *.sfl");
	if(!file.size()) return;

	area->cleanPickPoints();
  
  if (global_paraMgr.glarea.getBool("Show Model")
    &&!area->dataMgr.isModelEmpty())
  {
    QString file_model(file);
    file_model.replace(".ply", "_model.ply");
    area->dataMgr.savePly(file_model, *area->dataMgr.getCurrentModel());
  }

  if (global_paraMgr.glarea.getBool("Show ISO Points")
    && !area->dataMgr.isIsoPointsEmpty())
  {
    QString file_iso(file);
    file_iso.replace(".ply", "_iso.ply");
    area->dataMgr.savePly(file_iso, *area->dataMgr.getCurrentIsoPoints());
  }

	if (global_paraMgr.glarea.getBool("Show Original"))
	{
		if (global_paraMgr.glarea.getBool("Show Samples"))
		{
			area->dataMgr.eraseRemovedSamples();
			area->dataMgr.savePly(file, *area->dataMgr.getCurrentSamples());
		}

		if (file.endsWith("ply") && !area->dataMgr.isOriginalEmpty())
		{
			file.replace(".ply", "_original.ply");
			area->dataMgr.savePly(file, *area->dataMgr.getCurrentOriginal());
    }
		return;
	}

  if (global_paraMgr.glarea.getBool("Show Samples"))
  {
    if (file.endsWith("ply") && !area->dataMgr.isSamplesEmpty())
    {
      area->dataMgr.savePly(file, *area->dataMgr.getCurrentSamples());
    }
  }

  if (global_paraMgr.glarea.getBool("Show Poisson Surface"))
  {
    if (file.endsWith("ply") && !area->dataMgr.isPoissonSurfaceEmpty())
    {
      area->dataMgr.savePly(file, *area->dataMgr.getCurrentPoissonSurface());
    }
  }
}

void MainWindow::removeOutliers()
{
  if (global_paraMgr.glarea.getBool("Show NBV Ball") && global_paraMgr.glarea.getBool("Show NBV Candidates"))
  {
    area->removeBadCandidates();
  }
  else
  {
    area->removeOutliers();
  }
  area->initView();
  area->updateGL();
}

void MainWindow::downSample()
{
  if (global_paraMgr.glarea.getBool("GLarea Busying"))
  {
    global_paraMgr.glarea.setValue("Algorithm Stop", BoolValue(true));
    global_paraMgr.glarea.setValue("GLarea Busying", BoolValue(false));
    return;
  }

	area->dataMgr.downSamplesByNum();
	area->initSetting();
	area->updateGL();
}

void MainWindow::transformToGroundAxis()
{
  //TODO
  area->dataMgr.transformToGroundAxis();
}

void MainWindow::getQianSample()
{
 /* CMesh* samples = area->dataMgr.getCurrentSamples();
  CMesh* original = area->dataMgr.getCurrentOriginal();

  if (!original->vert.empty())
  {
    original->vert.clear();
  }
  for (int i = 0; i < samples->vert.size(); i++)
  {
    CVertex v = samples->vert[i];
    v.is_original = true;
    original->vert.push_back(v);
  }
  original->vn = samples->vn;*/

  area->dataMgr.downSamplesByNum(false);
  area->initAfterOpenFile();
  //area->initSetting();
  area->updateGL();
}

void MainWindow::subSample()
{
	area->dataMgr.subSamples();
	area->initSetting();
	area->updateGL();
}

void MainWindow::normalizeData()
{
	area->dataMgr.normalizeAllMesh();
	area->initView();
	area->updateGL();
}

void MainWindow::clearData()
{
	area->dataMgr.clearData();
  area->initView();
	area->updateUI();
	area->updateGL();
}

void MainWindow::saveSnapshot()
{
	area->saveSnapshot();
}

void MainWindow::saveView()
{
	QString file = QFileDialog::getSaveFileName(this, "Select a ply file", "", "*.View");
	if(!file.size()) return;
	area->saveView(file);
}

void MainWindow::saveNBV()
{
  QString file = QFileDialog::getSaveFileName(this, "Save NBV as...", "", "*.ply");
  if (!file.size()) return;

  area->saveNBV(file);
}

void MainWindow::saveSkel()
{
	QString file = QFileDialog::getSaveFileName(this, "Save samples as", "", "*.skel");
	if(!file.size()) return;

	file.replace(".skel", ".View");
	area->saveView(file);

	area->updateGL();
}

void MainWindow::saveFieldPoints()
{
  global_paraMgr.poisson.setValue("Run Normalize Field Confidence", BoolValue(true));  
  area->runPoisson();
  global_paraMgr.poisson.setValue("Run Normalize Field Confidence", BoolValue(false));  


  QString file = QFileDialog::getSaveFileName(this, "Save filed as", "", "*.raw");
  if(!file.size()) return;

  area->dataMgr.saveFieldPoints(file);
  //area->dataMgr.saveSkeletonAsSkel(file);
}

void MainWindow::savePara()
{
  area->dataMgr.saveParameters("parameter.para");
}


void MainWindow::saveViewGridsForVoreen()
{
  QString file = QFileDialog::getSaveFileName(this, "Save View Grids as", "", "*.raw");
  if (!file.size()) return;

  area->dataMgr.saveViewGrids(file);
}

void MainWindow::setSnapshotEachIteration(bool _val)
{
	paras->glarea.setValue("SnapShot Each Iteration",BoolValue(_val));
}

void MainWindow::setNoSnapshotWithRadius(bool _val)
{
	paras->glarea.setValue("No Snap Radius", BoolValue(_val));
}

void MainWindow::showColorfulBranches(bool _val)
{
  //if (area->paraMgr.glarea.getBool("GLarea Busying"))
  //{
  //  return;
  //}

  if (_val)
  {
    area->glDrawer.generateRandomColorList();
  }
  paras->drawer.setValue("Use Differ Branch Color", BoolValue(_val));
  area->updateGL();
}

void MainWindow::showBox(bool _val)
{
  paras->glarea.setValue("Show Bounding Box", BoolValue(_val));
  area->updateGL();
}


void MainWindow::showIsoPoints(bool _val)
{
  paras->glarea.setValue("Show ISO Points", BoolValue(_val));
  area->updateGL();
}

void MainWindow::showViewGrids(bool _val)
{
  paras->glarea.setValue("Show View Grids", BoolValue(_val));

  if (!area->dataMgr.isFieldPointsEmpty())
  {
    CMesh* field_points = area->dataMgr.getCurrentFieldPoints();
    for (int i = 0; i < 200; i++)
    {
      cout << "eigen confidence:  "<< field_points->vert[i].eigen_confidence << endl;
    }
  }

  area->updateGL();
}

void MainWindow::showNBVCandidates(bool _val)
{
  paras->glarea.setValue("Show NBV Candidates", BoolValue(_val));
  area->updateGL();
}

void MainWindow::showScanCandidates(bool _val)
{
  paras->glarea.setValue("Show Scan Candidates", BoolValue(_val));
  area->updateGL();
}

void MainWindow::showScanHistory(bool _val)
{
  paras->glarea.setValue("Show Scan History", BoolValue(_val));
  area->updateGL();
}

void MainWindow::showSDFVoxels(bool _val)
{
  paras->glarea.setValue("Show SDF Voxels", BoolValue(_val));
  area->updateGL();
}

void MainWindow::showSDFSlices(bool _val)
{
  paras->glarea.setValue("Show SDF Slices", BoolValue(_val));
  area->updateGL();
}

void MainWindow::showGraphCutRelated(bool _val)
{
  paras->glarea.setValue("Show GraphCut Related", BoolValue(_val));
  //jerrysyf
  area->updateGL();
}

void MainWindow::showScannedMesh(bool _val)
{
  paras->glarea.setValue("Show Scanned Mesh", BoolValue(_val));
  area->updateGL();
}

void MainWindow::showPoissonSurface(bool _val)
{
  paras->glarea.setValue("Show Poisson Surface", BoolValue(_val));
  area->updateGL();
}

void MainWindow::useIsoInterval(bool _val)
{
  paras->glarea.setValue("Use ISO Interval", BoolValue(_val));
  area->updateGL();
}

void MainWindow::lightOnOff(bool _val)
{
	paras->glarea.setValue("Light On or Off",BoolValue(_val));
	if (_val)
	{
		glEnable(GL_LIGHTING);
	}
	else
	{
		glDisable(GL_LIGHTING);
	}
	area->updateGL();
}

void MainWindow::showModel(bool _val)
{
  global_paraMgr.glarea.setValue("Show Model", BoolValue(_val));
  area->updateGL();
}

void MainWindow::showOriginal(bool _val)
{
	global_paraMgr.glarea.setValue("Show Original", BoolValue(_val));
	area->updateGL();
}

void MainWindow::showSamples(bool _val)
{
	global_paraMgr.glarea.setValue("Show Samples", BoolValue(_val));
	area->updateGL();
}

void MainWindow::showNormals(bool _val)
{
	global_paraMgr.glarea.setValue("Show Normal", BoolValue(_val));
	area->updateGL();
}

void MainWindow::cullPoints(bool _val)
{
	global_paraMgr.drawer.setValue("Need Cull Points", BoolValue(_val));
	area->updateGL();
}

void MainWindow::showNormalColor(bool _val)
{
	cout << "show normal" << endl;
	global_paraMgr.drawer.setValue("Use Color From Normal", BoolValue(_val));
	area->updateGL();
}


void MainWindow::showNeighborhoodBall(bool _val)
{
	global_paraMgr.glarea.setValue("Show Radius", BoolValue(_val));
	area->updateGL();
}

void MainWindow::showAllNeighborhoodBall(bool _val)
{
	global_paraMgr.glarea.setValue("Show All Radius", BoolValue(_val));
	area->updateGL();
}

void MainWindow::showIndividualColor(bool _val)
{
	global_paraMgr.drawer.setValue("Show Individual Color", BoolValue(_val));
	area->updateGL();
}

void MainWindow::showConfidenceColor(bool _val)
{
  global_paraMgr.drawer.setValue("Show Confidence Color", BoolValue(_val));
  area->updateGL();
}

void MainWindow::showNBVLables(bool _val)
{
  global_paraMgr.glarea.setValue("Show NBV Label", BoolValue(_val));
  area->updateGL();
}

void MainWindow::showNBVBall(bool _val)
{
  global_paraMgr.glarea.setValue("Show NBV Ball", BoolValue(_val));
  area->updateGL();
}


void MainWindow::setSmapleType(QAction * action)
{
	if(action == ui.actionShow_Sample_Quads)
	{
		paras->glarea.setValue("Show Samples Quad",BoolValue(true));
		paras->glarea.setValue("Show Samples Dot",BoolValue(false));
		paras->glarea.setValue("Show Samples Circle",BoolValue(false));
		paras->glarea.setValue("Show Samples Sphere",BoolValue(false));
	}
	else if (action == ui.actionShow_Sample_Dot)
	{
		paras->glarea.setValue("Show Samples Quad",BoolValue(false));
		paras->glarea.setValue("Show Samples Dot",BoolValue(true));
		paras->glarea.setValue("Show Samples Circle",BoolValue(false));
		paras->glarea.setValue("Show Samples Sphere",BoolValue(false));
	}
	else if(action == ui.actionShow_Sample_Circle)
	{
		paras->glarea.setValue("Show Samples Quad",BoolValue(false));
		paras->glarea.setValue("Show Samples Dot",BoolValue(false));
		paras->glarea.setValue("Show Samples Circle",BoolValue(true));
		paras->glarea.setValue("Show Samples Sphere",BoolValue(false));
	}
	else if (action == ui.actionShow_Sample_Sphere)
	{
		paras->glarea.setValue("Show Samples Quad",BoolValue(false));
		paras->glarea.setValue("Show Samples Dot",BoolValue(false));
		paras->glarea.setValue("Show Samples Circle",BoolValue(false));
		paras->glarea.setValue("Show Samples Sphere",BoolValue(true));
	}
	area->updateGL();
}

void MainWindow::setOriginalType(QAction * action)
{
	if(action == ui.actionShow_Original_Quad)
	{
		paras->glarea.setValue("Show Original Quad",BoolValue(true));
		paras->glarea.setValue("Show Original Dot",BoolValue(false));
		paras->glarea.setValue("Show Original Circle",BoolValue(false));
		paras->glarea.setValue("Show Original Sphere",BoolValue(false));
	}
	else if (action == ui.actionShow_Original_Dot)
	{
		paras->glarea.setValue("Show Original Quad",BoolValue(false));
		paras->glarea.setValue("Show Original Dot",BoolValue(true));
		paras->glarea.setValue("Show Original Circle",BoolValue(false));
		paras->glarea.setValue("Show Original Sphere",BoolValue(false));
	}
	else if(action == ui.actionShow_Original_Circle)
	{
		paras->glarea.setValue("Show Original Quad",BoolValue(false));
		paras->glarea.setValue("Show Original Dot",BoolValue(false));
		paras->glarea.setValue("Show Original Circle",BoolValue(true));
		paras->glarea.setValue("Show Original Sphere",BoolValue(false));
	}
	else if (action == ui.actionShow_Original_Sphere)
	{
		paras->glarea.setValue("Show Original Quad",BoolValue(false));
		paras->glarea.setValue("Show Original Dot",BoolValue(false));
		paras->glarea.setValue("Show Original Circle",BoolValue(false));
		paras->glarea.setValue("Show Original Sphere",BoolValue(true));
	}
	area->updateGL();
}

void MainWindow::runPCA_Normal()
{
	int knn = global_paraMgr.norSmooth.getInt("PCA KNN");
	CMesh* samples = area->dataMgr.getCurrentSamples();
	vcg::NormalExtrapolation<vector<CVertex> >::ExtrapolateNormals(samples->vert.begin(), samples->vert.end(), knn, -1);
}

void MainWindow::reorientateNormal()
{
	if (area->dataMgr.isSamplesEmpty())
		return;

	CMesh* samples = area->dataMgr.getCurrentSamples();
	for (int i = 0; i < samples->vert.size(); i++)
		samples->vert[i].N() *= -1;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	event->accept();
}

void MainWindow::dropEvent ( QDropEvent * event )
{
	const QMimeData * data = event->mimeData();

	if (data->hasUrls())
	{
		QList< QUrl > url_list = data->urls();
		for (int i=0, size=url_list.size(); i<size; i++)
		{
			QString path = url_list.at(i).toLocalFile();
			area->openByDrop(path);
			cout << "open file: "<< path.toStdString() << endl;
		}
	}
}

void MainWindow::sampleColor()
{
	area->changeColor("Sample Point Color");
	area->updateGL();
}

void MainWindow::originalColor()
{
	area->changeColor("Original Point Color");
	area->updateGL();
}

void MainWindow::backGroundColor()
{
	area->changeColor("Background Color");
	area->updateGL();
}

void MainWindow::ambientColor()
{
	area->changeColor("Light Ambient Color");
	area->updateGL();
}

void MainWindow::diffuseColor()
{
	area->changeColor("Light Diffuse Color");
	area->updateGL();
}

void MainWindow::specularColor()
{
	area->changeColor("Light Specular Color");
	area->updateGL();
}

void MainWindow::normalColor()
{
	area->changeColor("Normal Line Color");
	area->updateGL();
}

void MainWindow::featureColor()
{
	area->changeColor("Feature Color");
	area->updateGL();
}

void MainWindow::recomputeQuad()
{
	//cout << "recompute quad" << endl;
	//if (area->dataMgr.isSamplesEmpty())
	//{
	//	return;
	//}

	area->dataMgr.recomputeQuad();
	area->updateGL();
}


void MainWindow::removePickPoints()
{
	area->removePickPoint();
	area->updateUI();
	area->updateGL();
}


//void MainWindow::poissonTest()
//{
//  cout << "poisson test" << endl;
//
//  area->poissonTest();
//  area->updateUI();
//  area->updateGL();
//}
//
//void MainWindow::poissonTestAll()
//{
//  global_paraMgr.glarea.setValue("All Octree Nodes", BoolValue(true));
//  area->poissonTest();
//  global_paraMgr.glarea.setValue("All Octree Nodes", BoolValue(false));
//
//  area->updateUI();
//  area->updateGL();
//}

void MainWindow::switchSampleOriginal()
{
  area->cleanPickPoints();
  area->dataMgr.switchSampleToOriginal();
  area->updateUI();
}

void MainWindow::switchSampleISO()
{
  area->cleanPickPoints();
  area->dataMgr.switchSampleToISO();
  area->updateUI();
}

void MainWindow::switchSampleNBV()
{
  area->cleanPickPoints();
  area->dataMgr.switchSampleToNBV();
  area->updateUI();
}

void MainWindow::addSamplesToOriginal()
{
  CMesh* samples = area->dataMgr.getCurrentSamples();
  CMesh* original = area->dataMgr.getCurrentOriginal();

  samples->face.clear();
  original->face.clear();

  int idx = original->vert.back().m_index + 1;

  for (int i = 0; i < samples->vert.size(); i++)
  {
    CVertex t = samples->vert[i];
    t.is_original = true;
    t.m_index = idx++;

    original->vert.push_back(t);
    original->bbox.Add(t.P());
  }
  original->vn = original->vert.size();
}

void MainWindow::deleteIgnore()
{
  CMesh* mesh;
  if (global_paraMgr.glarea.getBool("Show ISO Points")
    && !area->dataMgr.isIsoPointsEmpty())
  {
    mesh = area->dataMgr.getCurrentIsoPoints();
  }
  else if (global_paraMgr.glarea.getBool("Show Original")
    && !area->dataMgr.isOriginalEmpty())
  {
    mesh = area->dataMgr.getCurrentOriginal();
  }
  else
  {
    mesh = area->dataMgr.getCurrentSamples();
  }

  GlobalFun::deleteIgnore(mesh);
}

void MainWindow::recoverIgnore()
{
  CMesh* mesh;
  if (global_paraMgr.glarea.getBool("Show ISO Points")
    && !area->dataMgr.isIsoPointsEmpty())
  {
    mesh = area->dataMgr.getCurrentIsoPoints();
  }
  else if (global_paraMgr.glarea.getBool("Show Original")
    && !area->dataMgr.isOriginalEmpty())
  {
    mesh = area->dataMgr.getCurrentOriginal();
  }
  else
  {
    mesh = area->dataMgr.getCurrentSamples();
  }

  GlobalFun::recoverIgnore(mesh);
  cout << "recover!!" << endl;
}

void MainWindow::convertPlyToObj()
{
  QString file_location = QFileDialog::getExistingDirectory(this, "choose a directory...", "",QFileDialog::ShowDirsOnly);
  if (!file_location.size()) 
    return;

  QDir dir(file_location);
  if (!dir.exists()) 
    return;

  dir.setFilter(QDir::Files);
  dir.setSorting(QDir::Name);
  QFileInfoList list = dir.entryInfoList();

  for (int i = 0; i < list.size(); ++i)
  {
    QFileInfo fileInfo = list.at(i);
    QString f_name = fileInfo.fileName();

    if (!f_name.endsWith(".ply"))
      continue;

    f_name = file_location + "\\" + f_name;
    QString out = f_name;
    out.replace(".ply", ".obj");
    CMesh ply;
    int mask = tri::io::Mask::IOM_VERTCOORD + tri::io::Mask::IOM_VERTNORMAL;
    tri::io::ImporterPLY<CMesh>::Open(ply, f_name.toAscii().data(), mask);
    tri::io::ExporterOBJ<CMesh>::Save(ply, out.toAscii().data(), mask);
  }
}

void MainWindow::computeNormalForPoissonSurface()
{
  QString file_location = QFileDialog::getExistingDirectory(this, "choose a directory...", "",QFileDialog::ShowDirsOnly);
  if (!file_location.size()) 
    return;

  QDir dir(file_location);
  if (!dir.exists()) 
    return;

  dir.setFilter(QDir::Files);
  dir.setSorting(QDir::Name);
  
  QFileInfoList list = dir.entryInfoList();

  for (int i = 0; i < list.size(); ++i)
  {
    QFileInfo fileInfo = list.at(i);
    QString f_name = fileInfo.fileName();

    if (!f_name.endsWith(".ply"))
      continue;

    QString in_ply = file_location + "\\" + f_name;
    QString out_ply = file_location + "\\add_surface_normal_" + f_name;
    CMesh ply_mesh;
    int mask = tri::io::Mask::IOM_ALL;
    tri::io::ImporterPLY<CMesh>::Open(ply_mesh, in_ply.toAscii().data(), mask);
    //tri::UpdateNormals<CMesh>::PerFace(ply);
    //tri::io::ExporterPLY<CMesh>::Save(ply_mesh, out_ply.toAscii().data(), mask);
  }
}

void MainWindow::evaluation()
{
  evaluationForDifferentModels();

 /*QString file_name = QFileDialog::getOpenFileName(this, "choose a file to evaluate...", "", "*.ply");
  if (file_name.size() == 0)
  {
    cout<<"can't open file" <<endl;
    return;
  }

  area->dataMgr.loadPlyToISO(file_name);
  CMesh *target = area->dataMgr.getCurrentIsoPoints();
  CMesh *model = area->dataMgr.getCurrentModel();

  GlobalFun::computeAnnNeigbhors(model->vert, target->vert, 1, false, "runEvaluation");

  for (int i = 0; i < target->vert.size(); ++i)
  {
    CVertex &v = target->vert[i];
    if (!v.neighbors.empty())
    {
      CVertex &nearest = model->vert[target->vert[i].neighbors[0]];
      double dist = GlobalFun::computeEulerDist(v.P(), nearest.P());
      v.eigen_confidence = dist;
      Point3f c = GlobalFun::scalar2color(dist);
      v.C().SetRGB(255 * c[0], 255 * c[1], 255 * c[2]);
    }
  }
  GlobalFun::normalizeConfidence(target->vert, 0.f);*/
}

void MainWindow::evaluationForDifferentModels()
{
  QString file_name = QFileDialog::getOpenFileName(this, "choose a file to evaluate...", "", "*.ply");
  if (file_name.size() == 0)
  {
  cout<<"can't open file" <<endl;
  return;
  }
  area->dataMgr.loadPlyToPoisson(file_name);
  CMesh *target = area->dataMgr.getCurrentPoissonSurface();
  //CMesh *target = area->dataMgr.getCurrentOriginal();
  CMesh *model = area->dataMgr.getCurrentModel();

  GlobalFun::computeAnnNeigbhors(target->vert, model->vert, 1, false, "runEvaluation");
  //GlobalFun::computeAnnNeigbhors(model->vert, target->vert, 1, false, "runEvaluation");

  double dist_sum = 0.0;
  double dist_max = 0.0;
  ofstream out;
  out.open("dist.txt");

  for (int i = 0; i < model->vert.size(); ++i)
  {
    CVertex &v = model->vert[i];
    if (!v.neighbors.empty())
    {
      CVertex &nearest = target->vert[v.neighbors[0]];
      double dist = GlobalFun::computeEulerDist(v.P(), nearest.P());
      out<<dist <<std::endl;
      v.eigen_confidence = dist;
      if (dist > dist_max) dist_max = dist;

      dist_sum += dist;
    }
  }
  out.close();
  std::cout<<"mean: "<<dist_sum / model->vert.size() <<std::endl;
  std::cout<<"max: "<<dist_max <<std::endl;
}

void MainWindow::switchHistoryNBV()
{
  vector<ScanCandidate> temp_history;;
  vector<ScanCandidate>* history = area->dataMgr.getScanHistory();

  CMesh* candidates = area->dataMgr.getNbvCandidates();

  for (int i = 0; i < candidates->vert.size(); i++)
  {
    ScanCandidate s =  make_pair(candidates->vert[i].P(), candidates->vert[i].N());
    temp_history.push_back(s);
  }

  candidates->vert.clear();
  for (int i = 0; i < history->size(); i++)
  {
    ScanCandidate s = history->at(i);
    CVertex new_v;
    new_v.m_index = i;
    new_v.is_view_grid = true;
    new_v.P() = s.first;
    new_v.N() = s.second;

    candidates->vert.push_back(new_v);
  }
  candidates->vn = candidates->vert.size();

  history->clear();
  for (int i = 0; i < temp_history.size(); i++)
  {
    history->push_back(temp_history[i]);
  }
}

void MainWindow::addNBVtoHistory()
{

  CMesh* candidates = area->dataMgr.getNbvCandidates();
  vector<ScanCandidate>* history = area->dataMgr.getScanHistory();

  for (int i = 0; i < candidates->vert.size(); i++)
  {
    ScanCandidate s =  make_pair(candidates->vert[i].P(), candidates->vert[i].N());
    history->push_back(s);
  }
}

void MainWindow::SICPWithNormal()
{
  std::cout<<"SICP with normal." <<std::endl;
  QString still_mesh_file = QFileDialog::getOpenFileName(this, "Choose a static mesh", "", "*.ply");
  if(!still_mesh_file.size()) 
    return;
  area->dataMgr.loadPlyToOriginal(still_mesh_file);

  QString moving_mesh_file = QFileDialog::getOpenFileName(this, "Choose a moving mesh", "", "*.ply");
  if(!moving_mesh_file.size()) 
    return;
  area->dataMgr.loadPlyToSample(moving_mesh_file);

  CMesh *moving_mesh = area->dataMgr.getCurrentOriginal();
  CMesh *still_mesh = area->dataMgr.getCurrentSamples();
  std::cout<<"original point num: " <<moving_mesh->vert.size() <<std::endl;
  std::cout<<"sample point num: " <<still_mesh->vert.size() <<std::endl;

  double error = -1.0f;
  GlobalFun::computeICPNoNormal(moving_mesh, still_mesh, error);

  std::cout<<"original point num: " <<moving_mesh->vert.size() <<std::endl;
  std::cout<<"sample point num: " <<still_mesh->vert.size() <<std::endl;
  std::cout<<"ICP Error: " <<error <<std::endl;
}