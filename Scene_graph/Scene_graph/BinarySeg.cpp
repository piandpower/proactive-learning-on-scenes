#include "BinarySeg.h"


CBinarySeg::CBinarySeg()
{
	thresholdClose0 = 0.005;  //判断patch是否相连
	thresholdClose1 = 0.05;	  //计算局部法向量

	paraSmallK = 1;
	paraSmallS = 0.02;
	paraLargeK = 20;
	paraLargeS = 0.1;

	paraConvexK = 0.1;
	paraConvexT = 0.9;
	paraConcave = 1.0;

	paraGeometry = 0.4;
	paraAppearence = 0.3;

	paraMinPatchInObject = 5;
	paraMaxCutEnergy = 18.328;

	paraAlpha = 0.0;

	seedPatch = 0;
}

CBinarySeg::~CBinarySeg(void)
{

}

void CBinarySeg::AddTable(MyPointCloud_RGB_NORMAL &table)
{
	tablePoint = table;

	for(int i=0;i<tablePoint.mypoints.size();i++)
	{
		tableCen.x += tablePoint.mypoints[i].x;
		tableCen.y += tablePoint.mypoints[i].y;
		tableCen.z += tablePoint.mypoints[i].z;
	}
	tableCen.x /= tablePoint.mypoints.size();
	tableCen.y /= tablePoint.mypoints.size();
	tableCen.z /= tablePoint.mypoints.size();
}

void CBinarySeg::AddClusterPoints(vector<MyPointCloud_RGB_NORMAL> &points)
{
	for(int i=0;i<points.size();i++)
	{
		vecPatchPoint.push_back(points[i]);
	}
	clusterPatchNum.push_back(points.size());
}

void CBinarySeg::AddPatchNormal(vector<Normal> &normal)
{
	vecPatcNormal = normal;
}

void CBinarySeg::MainStep()
{
	clock_t time[10];

	ofstream outFilet("Output\\Time.txt",ios::out|ios::app);

	time[0] = clock();
	PointCloudPreprocess();

	time[1] = clock();
	ComputeSmoothValue();

	time[2] = clock();
	vecvecObjectPool.clear();
	for(int i =0; i <vecPatchPoint.size(); i ++)
	{

		ofstream outFile0("Output\\cutEnergy.txt",ios::out|ios::app);
		seedPatch = i;
		int flagStop = true;
		paraLargeS = 0.01;
		outFile0 <<  "index:" << i <<  endl;
		while(paraLargeS < 0.4 && flagStop)
		{
			vector<int> vecObjectHypo;
			double cutEnergy;
			vecObjectHypo.clear();
			ComputeDataValue();
			GraphCutSolve(vecObjectHypo,cutEnergy);
			if(vecObjectHypo.size() > paraMinPatchInObject && cutEnergy < paraMaxCutEnergy)
			{
				vecvecObjectPool.push_back(vecObjectHypo);
				flagStop = false;
				outFile0 <<  "cutEnergy:" <<cutEnergy << "paraLargeS:" <<paraLargeS << "  foreground size:" << vecObjectHypo.size() <<  endl;
			}	
			paraLargeS += (double)0.01;
		}
		outFile0 <<  "  " <<  endl;

		if(flagStop)
		{
			vector<int> vecObjectHypo;
			vecvecObjectPool.push_back(vecObjectHypo);
		}
		outFile0.close();
	}

	time[3] = clock();

	double duration0 = (double)(time[1] - time[0]) / CLOCKS_PER_SEC;
	double duration1 = (double)(time[2] - time[1]) / CLOCKS_PER_SEC;
	double duration2 = (double)(time[3] - time[2]) / CLOCKS_PER_SEC;
	outFilet <<  "step1:" << duration0 <<  endl;
	outFilet <<  "step2:" << duration1 <<  endl;
	outFilet <<  "step2:" << duration2 <<  endl;
	outFilet <<  "  " <<  endl;
	outFilet.close();


	//output
// 	ofstream outFile1("Output\\ObjectPool1229.txt");
// 	outFile1 <<  "thresholdClose0:" <<thresholdClose0
// 		<<  "  thresholdClose1:" <<thresholdClose1
// 		<<  "  paraSmallS:" <<paraSmallS
// 		<<  "  paraLargeK:" <<paraLargeK
// 		<<  "  paraLargeS:" <<paraLargeS
// 		<<  "  paraConvexK:" <<paraConvexK
// 		<<  "  paraConvexT:" <<paraConvexT
// 		<<  "  paraConcave:" <<paraConcave
// 		<<  "  paraGeometry:" <<paraGeometry
// 		<<  "  paraAppearence:" <<paraAppearence
// 		<<  "  paraMinPatchInObject:" <<paraMinPatchInObject
// 		<<  "  paraMaxCutEnergy:" <<paraMaxCutEnergy
// 		<<  endl;
// 	for(int i=0;i<vecvecObjectPool.size();i++)
// 	{
// 		for(int j=0;j<vecvecObjectPool[i].size();j++)
// 		{
// 			outFile1 << vecvecObjectPool[i][j] <<  "  ";
// 		}
// 		outFile1 << "  " << endl;
// 	}
// 	outFile1.close();

	ofstream outFilep("Output\\PatchPoint-table0.txt");
	ofstream outFiles("Output\\ClusterSize-table0.txt");
	ofstream outFilen("Output\\PatchNormal-table0.txt");
	ofstream outFiletc("Output\\TableCloud-table0.txt");

	//output
	for(int i=0;i<vecPatchPoint.size();i++)
	{
		for(int j=0;j<vecPatchPoint[i].mypoints.size();j++)
		{
			outFilep<< vecPatchPoint[i].mypoints[j].x << "  " <<
			vecPatchPoint[i].mypoints[j].y << "  " <<
			vecPatchPoint[i].mypoints[j].z << "  " <<
			vecPatchPoint[i].mypoints[j].normal_x << "  " <<
			vecPatchPoint[i].mypoints[j].normal_y << "  " <<
			vecPatchPoint[i].mypoints[j].normal_z << "  " <<
			vecPatchPoint[i].mypoints[j].r << "  " <<
			vecPatchPoint[i].mypoints[j].g << "  " <<
			vecPatchPoint[i].mypoints[j].b << "  " ;
		}
		outFilep <<  endl;
	}
	outFilep.close();

	//cluster size
	for(int i=0;i<clusterPatchNum.size();i++)
	{
		outFiles<< clusterPatchNum[i] << "  ";
	}
	outFiles <<  endl;
	outFiles.close();
	
	//normal
	for(int i=0;i<vecPatcNormal.size();i++)
	{
		outFilen<< vecPatcNormal[i].normal_x << "  " <<
				   vecPatcNormal[i].normal_y << "  " <<
				   vecPatcNormal[i].normal_z <<  endl;
	}
	outFilen.close();

	//table cloud
	for(int i=0;i<tablePoint.mypoints.size();i++)
	{
		outFiletc<< tablePoint.mypoints[i].x << "  "<< tablePoint.mypoints[i].y << "  "<< tablePoint.mypoints[i].z <<  endl;
	}
	outFiletc.close();
}

void CBinarySeg::PointCloudPreprocess()
{
	//bounding box, color average, center position
	xMin = yMin = zMin = LARGE_NUM;
	xMax = yMax = zMax = SMALL_NUM;
	for(int i = 0;i <vecPatchPoint.size();i++)
	{
		ColorType color;
		MyPoint point;
		vector<int> vecColorDetial;
		vecColorDetial.resize(21);
		for(int j = 0;j < vecColorDetial.size();j++)
		{
			vecColorDetial[j] = 0;
		}

		color.mRed = color.mGreen = color.mBlue = 0;
		point.x = point.y = point.z =0;

		for(int j = 0;j < vecPatchPoint[i].mypoints.size();j++)
		{
			color.mRed += vecPatchPoint[i].mypoints[j].r;
			color.mGreen += vecPatchPoint[i].mypoints[j].g;
			color.mBlue += vecPatchPoint[i].mypoints[j].b;

			vecColorDetial[(int)(vecPatchPoint[i].mypoints[j].r / 40) ] += 1;
			vecColorDetial[(int)(vecPatchPoint[i].mypoints[j].g / 40)  + 7] += 1;
			vecColorDetial[(int)(vecPatchPoint[i].mypoints[j].b / 40)  + 14] += 1;

			point.x += vecPatchPoint[i].mypoints[j].x;
			point.y += vecPatchPoint[i].mypoints[j].y;
			point.z += vecPatchPoint[i].mypoints[j].z;
			if(xMax < vecPatchPoint[i].mypoints[j].x) xMax = vecPatchPoint[i].mypoints[j].x;
			if(yMax < vecPatchPoint[i].mypoints[j].y) yMax = vecPatchPoint[i].mypoints[j].y;
			if(zMax < vecPatchPoint[i].mypoints[j].z) zMax = vecPatchPoint[i].mypoints[j].z;
			if(xMin > vecPatchPoint[i].mypoints[j].x) xMin = vecPatchPoint[i].mypoints[j].x;
			if(yMin > vecPatchPoint[i].mypoints[j].y) yMin = vecPatchPoint[i].mypoints[j].y;
			if(zMin > vecPatchPoint[i].mypoints[j].z) zMin = vecPatchPoint[i].mypoints[j].z;
		}
		if(vecPatchPoint[i].mypoints.size() > 0)
		{
			color.mRed /= vecPatchPoint[i].mypoints.size();
			color.mGreen /= vecPatchPoint[i].mypoints.size();
			color.mBlue /= vecPatchPoint[i].mypoints.size();
			point.x/= vecPatchPoint[i].mypoints.size();
			point.y /= vecPatchPoint[i].mypoints.size();
			point.z /= vecPatchPoint[i].mypoints.size();
		}

		vecPatchColor.push_back(color);
		vecvecPatchColorDetial.push_back(vecColorDetial);
		vecPatchCenPoint.push_back(point);
	}

	boundingBoxSize = sqrt((xMax-xMin) * (xMax-xMin) + (yMax-yMin) * (yMax-yMin) + (zMax-zMin) * (zMax-zMin));

	//distance between patches
	vecvecPatchMinDis.resize(vecPatchPoint.size());
	vecvecPatchCenDis.resize(vecPatchPoint.size());

	for(int i = 0;i <vecPatchPoint.size();i++)
	{
		vecvecPatchMinDis[i].resize(vecPatchPoint.size(),LARGE_NUM);
		vecvecPatchCenDis[i].resize(vecPatchPoint.size(),LARGE_NUM);
	}

	// 	vecKDTree.resize(vecPatchPoint.size());
	// 	for(int i = 0;i < vecKDTree.size(); i++)
	// 	{
	// 		vecKDTree[i] = kd_create( 3 );
	// 	}
	// 	for(int i = 0;i < vecKDTree.size(); i++)
	// 	{
	// 		for(int j = 0;j < vecPatchPoint[i].mypoints.size();j++)
	// 		{
	// 			kd_insert3( vecKDTree[i], vecPatchPoint[i].mypoints[j].x, vecPatchPoint[i].mypoints[j].y, vecPatchPoint[i].mypoints[j].z, "d" ) ;
	// 		}
	// 	}

	int countPatch = 0;
	int patchBegin,patchEnd;
	for(int i = 0;i <clusterPatchNum.size();i++)
	{
		patchBegin = countPatch;
		patchEnd = countPatch + clusterPatchNum[i];
		GetAdjacency(patchBegin,patchEnd);
		countPatch += clusterPatchNum[i];
	}

	// 	for(int i = 0;i < vecKDTree.size(); i++)
	// 	{
	// 		kd_free( vecKDTree[i] );
	// 	}

	//handle table
	vecIfConnectTable.clear();
	ConnectTableNum = 0;
	for(int i = 0;i <vecPatchPoint.size();i++)
 	{
 		vecIfConnectTable.push_back(IfConnectTable(vecPatchPoint[i].mypoints));
		if(vecIfConnectTable[vecIfConnectTable.size() - 1])
			ConnectTableNum++;
 	}

	//output
	// 	ofstream outFile1("Output\\color.txt");
	// 	for(int i = 0;i <vecvecPatchColorDetial.size();i++)
	// 	{
	// 		outFile1 << "patch "<< i << endl;
	// 		for(int j=0;j<vecvecPatchColorDetial[i].size();j++)
	// 		{
	// 			outFile1 << "data " << vecvecPatchColorDetial[i][j] << endl;
	// 		}
	// 		outFile1 << " " << endl;
	// 		outFile1 << " " << endl;
	// 	}
	// 	outFile1.close();

	//output
	// 	ofstream outFile0("Output\\basicinfo.txt");
	// 	for(int i = 0;i <clusterPatchNum.size();i++)
	// 	{
	// 		outFile0 << "clusterPatchNum: " << clusterPatchNum[i] << " " ;
	// 	}
	// 	outFile0 << "   " << endl;
	// 	
	// 	outFile0 << "vecPatchPoint: " << vecPatchPoint.size() << " " ;
	// 	outFile0 << "   " << endl;
	// 
	// 	outFile0 << "bounding box   xMax: " << xMax << " xMin: " << xMin 
	// 						  << " yMax: " << yMax << " yMin: " << yMin
	// 						  << " zMax: " << zMax << " zMin: " << zMin << endl;
	// // 	for(int i = 0;i <vecPatchColor.size();i++)
	// // 	{
	// // 		outFile0 << "color: " << vecPatchColor[i].mBlue << " " << vecPatchColor[i].mRed << " " << vecPatchColor[i].mGreen << endl;
	// // 	}
	// // 	outFile0 << "   " << endl;
	// 
	// 	for(int i = 0;i <vecPatchPoint.size();i++)
	// 	{
	// 		outFile0 << "patch size: " << vecPatchPoint[i].mypoints.size() << endl;
	// 	}
	// 	outFile0 << "   " << endl;
	// 
	// 	for(int i = 0;i <vecPatchCenPoint.size();i++)
	// 	{
	// 		outFile0 << "center point: " << vecPatchCenPoint[i].x << " " << vecPatchCenPoint[i].y << " " << vecPatchCenPoint[i].z << endl;
	// 	}
	// 	outFile0 << "   " << endl;
	// 
	// 	for(int i = 0;i <vecPatcNormal.size();i++)
	// 	{
	// 		outFile0 << "average normal: " << vecPatcNormal[i].normal_x << " "<< vecPatcNormal[i].normal_y << " " <<vecPatcNormal[i].normal_z << endl;
	// 	}
	// 	outFile0 << "   " << endl;
	// 	outFile0.close();
	// 
	// 	//output
	// 	ofstream outFile2("Output\\nearbyinfo.txt");
	// // 	for(int i = 0;i <vecIfConnectTable.size();i++)
	// // 	{
	// // 		outFile2 << "connecttable " << vecIfConnectTable[i] <<  " "  ;
	// // 	}
	// // 	outFile2 << "   " << endl;
	// 
	// 	for(int i = 0;i <vecpairPatchConnection.size();i++)
	// 	{
	// 		outFile2 << "patch " << vecpairPatchConnection[i].first << " "<< vecpairPatchConnection[i].second << " "  << endl;
	// 	}
	// 	outFile2 << "   " << endl;
	// 
	// // 	for(int i = 0;i <vecvecNearbyPoint.size();i++)
	// // 	{
	// // 		for(int j = 0;j <vecvecNearbyPoint[i].size();j++)
	// // 		{
	// // 			if(vecvecNearbyPoint[i][j].nearbyPoint.size()>0)
	// // 				for(int k = 0;k < vecvecNearbyPoint[i][j].nearbyPoint.size();k++)
	// // 					outFile2 << "nearby " << vecvecNearbyPoint[i][j].nearbyPoint[k].indexFirst << " "<< vecvecNearbyPoint[i][j].nearbyPoint[k].indexSecond << " patch " <<
	// // 					vecvecNearbyPoint[i][j].nearbyPoint[k].patchFirst << " "<< vecvecNearbyPoint[i][j].nearbyPoint[k].patchSecond << " " << endl;
	// // 		}
	// // 	}
	// 	outFile2 << "   " << endl;
	// 	outFile2.close();
	// 
	// // 	ofstream outFile3("Output\\nearbynormal.txt");
	// // 	for(int i = 0;i <vecvecPatctNearbyNormal.size();i++)
	// // 	{
	// // 		for(int j = 0;j <vecvecPatctNearbyNormal[i].size();j++)
	// // 		{
	// // 			if(vecvecNearbyPoint[i][j].nearbyPoint.size())
	// // 				outFile3 << "nearbynormal  i " << i << "  j  " << j << "   num  " << 
	// // 										   vecvecNearbyPoint[i][j].nearbyPoint.size() <<"  value  "<<
	// // 										   vecvecPatctNearbyNormal[i][j].normal0.normal_x << " "<< 
	// // 				                           vecvecPatctNearbyNormal[i][j].normal0.normal_y << " "<< 
	// // 										   vecvecPatctNearbyNormal[i][j].normal0.normal_z << " "<< 
	// // 										   vecvecPatctNearbyNormal[i][j].normal1.normal_x << " "<< 
	// // 										   vecvecPatctNearbyNormal[i][j].normal1.normal_y << " "<< 
	// // 										   vecvecPatctNearbyNormal[i][j].normal1.normal_z << endl;
	// // 		}
	// // 	}
	// // 	outFile3 << "   " << endl;
	// // 	outFile3.close();

}

void CBinarySeg::ComputeSmoothValue()
{
	for(int i = 0;i <vecpairPatchConnection.size();i++)
	{
		vecSmoothValue.push_back(GetBinarySmoothValue(vecpairPatchConnection[i].first,vecpairPatchConnection[i].second));
	}

	NomalizeAppearence();
	NomalizeSmooth();
}


void CBinarySeg::ComputeDataValue()
{
	vecDataValue.clear();
	for(int i = 0;i <vecPatchPoint.size();i++)
	{
		vecDataValue.push_back(GetBinaryDataValue(vecvecPatchCenDis[seedPatch][i]));
	}

	//output
	// 	ofstream outFile1("Output\\datasmooth.txt");
	// 	for(int i = 0;i <vecPatchColor.size();i++)
	// 	{
	// 		outFile1 << "data  " << vecDataValue[i] << endl;
	// 	}
	// 	for(int i = 0;i <vecpairPatchConnection.size();i++)
	// 	{
	// 		outFile1 << "GeometryValue  " << vecGeometryValue[i] << endl;
	// 		outFile1 << "AppearenceValue  " << vecAppearenceValue[i] << endl;
	// 		outFile1 << "smooth  " << vecSmoothValue[i] << endl;
	// 	}
	// 	outFile1.close();
}

void CBinarySeg::GraphCutSolve(vector<int>& vecObjectHypo, double &cutEnergy)
{
	typedef Graph<double,double,double> GraphType;
	GraphType *g = new GraphType(/*estimated # of nodes*/ vecPatchPoint.size() + 1, 
		/*estimated # of edges*/ vecpairPatchConnection.size() + ConnectTableNum); 

	g -> add_node(vecPatchPoint.size() + 1); 

	for(int i = 0;i <vecDataValue.size();i++)
	{
		if(i == seedPatch)
			g -> add_tweights(i, LARGE_NUM, 0);
		else if(i != seedPatch)
			g -> add_tweights(i, paraAlpha, vecDataValue[i]);
	}
	g -> add_tweights(vecDataValue.size(), 0, LARGE_NUM);			//table data

	for(int i = 0;i <vecSmoothValue.size();i++)
	{
		g -> add_edge(vecpairPatchConnection[i].first, vecpairPatchConnection[i].second, vecSmoothValue[i], vecSmoothValue[i]);
	}
	for(int i = 0;i <vecIfConnectTable.size();i++)
	{
		if(vecIfConnectTable[i])
			g -> add_edge(i, vecDataValue.size(), 0, 0);			//table smooth
	}

	m_flow = g -> maxflow();
	cutEnergy = m_flow;

	int countSOURCE,countSINK;
	countSINK = countSOURCE =0;

	vecFore.clear();
	vecBack.clear();

	for(int i=0;i<vecPatchPoint.size();i++)
	{
		if (g->what_segment(i) == GraphType::SOURCE)
		{
			countSOURCE++;
			vecFore.push_back(i);
			vecObjectHypo.push_back(i);
		}
		else
		{
			countSINK++;
			vecBack.push_back(i);
		}
	}

	delete g; 

}

void CBinarySeg::GetAdjacency(int patchBegin,int patchEnd)
{
	pair<int,int> pairPatchConnection;
	for(int i = patchBegin;i < patchEnd;i++)
	{
		for(int j = patchBegin;j < patchEnd;j++)
		{
			if(i < j)
			{
				bool  stable;   // if connect stable

				vecvecPatchMinDis[i][j] = vecvecPatchMinDis[j][i] = GetMinDisBetPatch(i,j,stable);
				vecvecPatchCenDis[i][j] = vecvecPatchCenDis[j][i] = GetCenDisBetPatch(i,j);

				if(vecvecPatchMinDis[i][j] < thresholdClose0 && stable)
				{
					pairPatchConnection.first = i;
					pairPatchConnection.second = j;
					vecpairPatchConnection.push_back(pairPatchConnection);

					pairPatchConnection.first = j;
					pairPatchConnection.second = i;
					vecpairPatchConnection.push_back(pairPatchConnection);
				} 
			}
		}
	}

	//output
	// 	ofstream outFile0("Output\\Yanzheng.txt",ios::app);
	// 	outFile0 << " " << vecvecPatchMinDis[11][12] << endl;
	// 	outFile0 << " " << vecvecPatchMinDis[111][112] << endl;
	// 	outFile0 << " " << vecvecPatchMinDis[211][212] << endl;
	// 	outFile0 << " " << vecvecPatchMinDis[311][312] << endl;
	// 	outFile0.close();
}

double CBinarySeg::GetMinDisBetPatch(int m,int n,bool &stable)
{
	//////////////////////////////////kdtree
	// 	double minDis = LARGE_NUM;
	// 	vector<bool> vecPointClose0(vecPatchPoint[m].mypoints.size(),false);
	// 	vector<bool> vecPointClose1(vecPatchPoint[n].mypoints.size(),false);
	// 
	// 	
	// 	for(int i = 0;i < vecPatchPoint[n].mypoints.size();i++)
	// 	{
	// 		double pt[3];
	// 		pt[0] = vecPatchPoint[n].mypoints[i].x;
	// 		pt[1] = vecPatchPoint[n].mypoints[i].y;
	// 		pt[2] = vecPatchPoint[n].mypoints[i].z;
	// 
	// 		struct kdres *presults0;
	// 		presults0 = kd_nearest3( vecKDTree[m], pt[0], pt[1], pt[2]);
	// 
	// 		double dis;
	// 		dis = sqrt((presults0->riter->item->pos[0] - pt[0]) * (presults0->riter->item->pos[0] - pt[0]) +
	// 			  (presults0->riter->item->pos[1] - pt[1]) * (presults0->riter->item->pos[1] - pt[1]) +
	// 		      (presults0->riter->item->pos[2] - pt[2]) * (presults0->riter->item->pos[2] - pt[2]));
	// 
	// 		if(dis < minDis)
	// 			minDis = dis;
	// 
	// 		if(dis < thresholdClose0 * 3)
	// 			vecPointClose1[i] = true;
	// 
	// 		kd_res_free( presults0 );
	// 	}
	// 
	// 	for(int i = 0;i < vecPatchPoint[m].mypoints.size();i++)
	// 	{
	// 		double pt[3];
	// 		pt[0] = vecPatchPoint[m].mypoints[i].x;
	// 		pt[1] = vecPatchPoint[m].mypoints[i].y;
	// 		pt[2] = vecPatchPoint[m].mypoints[i].z;
	// 
	// 		struct kdres *presults1;
	// 		presults1 = kd_nearest3( vecKDTree[n], pt[0], pt[1], pt[2]);
	// 
	// 		double dis;
	// 		dis = sqrt((presults1->riter->item->pos[0] - pt[0]) * (presults1->riter->item->pos[0] - pt[0]) +
	// 			  (presults1->riter->item->pos[1] - pt[1]) * (presults1->riter->item->pos[1] - pt[1]) +
	// 			  (presults1->riter->item->pos[2] - pt[2]) * (presults1->riter->item->pos[2] - pt[2]));
	// 
	// 		if(dis < minDis)
	// 			minDis = dis;
	// 		
	// 		if(dis < thresholdClose0 * 3)
	// 			vecPointClose0[i] = true;
	// 
	// 		kd_res_free( presults1 );
	// 	}
	// 
	// 
	// 	int count0=0;
	// 	int count1=0;
	// 	for(int i = 0; i < vecPointClose0.size();i++)
	// 	{
	// 		if(vecPointClose0[i] == true)
	// 			count0++;
	// 	}
	// 	for(int i = 0; i < vecPointClose1.size();i++)
	// 	{
	// 		if(vecPointClose1[i] == true)
	// 			count1++;
	// 	}
	// 	if(count0 > 5 && count1 >5)
	// 		stable = true;
	// 	else
	// 		stable = false;
	// 
	// 	return minDis;


	//////////////////////////////////蛮力法
	double minDis = LARGE_NUM;
	vector<bool> vecPointClose0(vecPatchPoint[m].mypoints.size(),false);
	vector<bool> vecPointClose1(vecPatchPoint[n].mypoints.size(),false);

	for(int i = 0;i < vecPatchPoint[m].mypoints.size();i++)
	{
		for(int j = 0;j < vecPatchPoint[n].mypoints.size();j++)
		{
			double dis = sqrt((vecPatchPoint[m].mypoints[i].x-vecPatchPoint[n].mypoints[j].x) * (vecPatchPoint[m].mypoints[i].x-vecPatchPoint[n].mypoints[j].x)
				+ (vecPatchPoint[m].mypoints[i].y-vecPatchPoint[n].mypoints[j].y) * (vecPatchPoint[m].mypoints[i].y-vecPatchPoint[n].mypoints[j].y)
				+ (vecPatchPoint[m].mypoints[i].z-vecPatchPoint[n].mypoints[j].z) * (vecPatchPoint[m].mypoints[i].z-vecPatchPoint[n].mypoints[j].z));
			if(minDis > dis)
			{
				minDis = dis;
			}

			if(dis < thresholdClose0 * 3)
			{
				vecPointClose0[i] = true;
				vecPointClose1[j] = true;
			}
		}
	}

	int count0=0;
	int count1=0;
	for(int i = 0; i < vecPointClose0.size();i++)
	{
		if(vecPointClose0[i] == true)
			count0++;
	}
	for(int i = 0; i < vecPointClose1.size();i++)
	{
		if(vecPointClose1[i] == true)
			count1++;
	}

	if(count0 > 5 && count1 >5)
		stable = true;
	else
		stable = false;

	return minDis;
}

double CBinarySeg::GetCenDisBetPatch(int m,int n)
{
	double dis =  sqrt((vecPatchCenPoint[m].x-vecPatchCenPoint[n].x) * (vecPatchCenPoint[m].x-vecPatchCenPoint[n].x)
		+ (vecPatchCenPoint[m].y-vecPatchCenPoint[n].y) * (vecPatchCenPoint[m].y-vecPatchCenPoint[n].y)
		+ (vecPatchCenPoint[m].z-vecPatchCenPoint[n].z) * (vecPatchCenPoint[m].z-vecPatchCenPoint[n].z));
	return dis;
}

double CBinarySeg::GetBinaryDataValue(double d)
{
	if(d == LARGE_NUM || d == 0)  
	{
		return LARGE_NUM;
	}
	else
	{
		double penaltyValue, panaltySmallValue, penaltyLargeValue;
		penaltyValue = 0;

		panaltySmallValue =  paraSmallK * (d - 0.813342* paraSmallS);
		if(panaltySmallValue < 0)
			panaltySmallValue =  0;

		penaltyLargeValue =  paraLargeK * (d - 0.813342* paraLargeS);
		if(penaltyLargeValue < 0)
			penaltyLargeValue =  0;

		penaltyValue = panaltySmallValue + penaltyLargeValue;
		return penaltyValue;
	}

}

double CBinarySeg::GetBinarySmoothValue(int m,int n)
{
	double smoothValue,geometryValue,appearenceValue;

	//normal
	MyPoint cenM,cenN;
	Normal norM,norN,norMN,norNM;
	double nomalizeValue;

	cenM = vecPatchCenPoint[m];
	cenN = vecPatchCenPoint[n];

	norM = vecPatcNormal[m];
	norN = vecPatcNormal[n];

	norMN.normal_x = cenN.x - cenM.x;
	norMN.normal_y = cenN.y - cenM.y;
	norMN.normal_z = cenN.z - cenM.z;
	nomalizeValue = sqrt(norMN.normal_x * norMN.normal_x + norMN.normal_y * norMN.normal_y + norMN.normal_z * norMN.normal_z);
	norMN.normal_x /= nomalizeValue;
	norMN.normal_y /= nomalizeValue;
	norMN.normal_z /= nomalizeValue;

	bool convexFlag;
	double convexValue;   //convex if > 0
	convexValue = norMN.normal_x * norN.normal_x +  norMN.normal_y * norN.normal_y + norMN.normal_z * norN.normal_z;

	if(convexValue >= 0)	
		convexFlag = true;
	else	
		convexFlag = false;

	double cosValue;
	cosValue = (norM.normal_x * norN.normal_x + norM.normal_y * norN.normal_y + norM.normal_z * norN.normal_z);
	if(convexFlag)	
		geometryValue = paraConvexK * cosValue + paraConvexT;
	else	
		geometryValue = paraConcave * cosValue;
	if(geometryValue < 0)	geometryValue = 0;

	//color
	appearenceValue = 0;
	for(int i = 0;i < 21;i++)
	{
		double Mi,Ni;
		Mi = vecvecPatchColorDetial[m][i];
		Ni = vecvecPatchColorDetial[n][i];

		if(Mi != 0 || Ni != 0)
			appearenceValue += (Mi - Ni) * (Mi - Ni) / (Mi + Ni);
	}

	smoothValue = paraGeometry * geometryValue + paraAppearence * appearenceValue;

	vecGeometryValue.push_back(geometryValue);
	vecAppearenceValue.push_back(appearenceValue);

	return smoothValue;
}  

bool CBinarySeg::IfConnectTable(vector<MyPt_RGB_NORMAL> &points)
{
 	for(int i=0;i<points.size();i++)
 	{
 		for(int j=0;j<tablePoint.mypoints.size();j++)
 		{
 			double dis = sqrt((points[i].x - tablePoint.mypoints[j].x) * (points[i].x - tablePoint.mypoints[j].x) +
 							  (points[i].y - tablePoint.mypoints[j].y) * (points[i].y - tablePoint.mypoints[j].y) +
 							  (points[i].z - tablePoint.mypoints[j].z) * (points[i].z - tablePoint.mypoints[j].z));
 			if(dis < thresholdClose0)
 			{
 				return true;
 			}
 		}
 	}
 	return false;
}

void CBinarySeg::NomalizeData()
{
	double maxSV = SMALL_NUM;
	double minSV = LARGE_NUM;

	for(int i = 0;i <vecDataValue.size();i++)
	{
		if(maxSV < vecDataValue[i] && vecDataValue[i] != LARGE_NUM)
			maxSV = vecDataValue[i];
		if(minSV > vecDataValue[i] && vecDataValue[i] != LARGE_NUM)
			minSV = vecDataValue[i];
	}
	for(int i = 0;i <vecDataValue.size();i++)
	{
		if( vecDataValue[i] != LARGE_NUM)
			vecDataValue[i] =  (vecDataValue[i] - minSV)/(maxSV - minSV)/2 + 1;
	}
}

void CBinarySeg::NomalizeAppearence()
{
	double maxSV = SMALL_NUM;
	for(int i = 0;i <vecpairPatchConnection.size();i++)
	{
		if(maxSV < vecAppearenceValue[i])
			maxSV = vecAppearenceValue[i];
	}
	for(int i = 0;i <vecpairPatchConnection.size();i++)
	{
		vecAppearenceValue[i] = vecAppearenceValue[i] / maxSV;
		vecAppearenceValue[i] = 1 - vecAppearenceValue[i];
	}

	for(int i = 0;i <vecpairPatchConnection.size();i++)
	{
		vecGeometryValue[i] = paraGeometry * vecGeometryValue[i];
		vecAppearenceValue[i] = paraAppearence * vecAppearenceValue[i];
		vecSmoothValue[i] = vecGeometryValue[i] + vecAppearenceValue[i];
	}
}

void CBinarySeg::NomalizeSmooth()
{
	double maxSV = SMALL_NUM;
	double minSV = LARGE_NUM;
	double para = 0.3;
	for(int i = 0;i <vecSmoothValue.size();i++)
	{
		if(maxSV < vecSmoothValue[i])
			maxSV = vecSmoothValue[i];
		if(minSV > vecSmoothValue[i])
			minSV = vecSmoothValue[i];
	}
	for(int i = 0;i <vecSmoothValue.size();i++)
	{
		vecSmoothValue[i] =  1 * (vecSmoothValue[i] - minSV)/(maxSV - minSV);
		vecSmoothValue[i] = pow(2.7183,- (1 - vecSmoothValue[i]) * (1 - vecSmoothValue[i]) / para /para);
	}

}