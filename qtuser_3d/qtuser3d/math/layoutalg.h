#ifndef _NULLSPACE_CLayoutAlg_1589765331924_H
#define _NULLSPACE_CLayoutAlg_1589765331924_H
#include "qtuser3d/qtuser3dexport.h"
#include <QtGui/QVector3D>
#include <QtGui/QVector2D>
#include <QtCore/QVector>
#include <QRect>
#include <vector> 

namespace qtuser_3d
{
	//for every line, how long the every unit is empty
	struct SEmptyLine
	{
		int iXStart;
		int iLength;
		SEmptyLine()
		{
			iXStart = 0;
			iLength = 0;
		}
		~SEmptyLine()
		{
			iXStart = 0;
			iLength = 0;
		}
	};

	struct S3DPrtRectI
	{
		S3DPrtRectI()
		{
			iXMin = 0;
			iYMin = 0;
			iXMax = 0;
			iYMax = 0;
		}
		~S3DPrtRectI()
		{
			iXMin = 0;
			iYMin = 0;
			iXMax = 0;
			iYMax = 0;
		}
#if 1
		float iXMin;
		float iYMin;
		float iXMax;
		float iYMax;
#else

		int iXMin;
		int iYMin;
		int iXMax;
		int iYMax;
#endif
	};


	struct S3DPrtRectF
	{
		float fXMin;
		float fYMin;
		float fXMax;
		float fYMax;
		S3DPrtRectF()
		{
			fXMin = 0;
			fYMin = 0;
			fXMax = 0;
			fYMax = 0;
		}
		~S3DPrtRectF()
		{
			fXMin = 0;
			fYMin = 0;
			fXMax = 0;
			fYMax = 0;
		}

	};



	struct S3DPrtPointI
	{
		int iX;
		int iY;

		S3DPrtPointI()
		{
			iX = 0;
			iY = 0;
		}
		~S3DPrtPointI()
		{
			iX = 0;
			iY = 0;
		}
	};

	struct S3DPrtPointF
	{
		float fX;
		float fY;

		S3DPrtPointF()
		{
			fX = 0;
			fY = 0;
		}
		~S3DPrtPointF()
		{
			fX = 0;
			fY = 0;
		}
	};

	struct STwoSegmentsProject
	{
		STwoSegmentsProject()
		{
			fSlopeProject = 0;
			iYAxis = -1;
		}
		~STwoSegmentsProject()
		{
			fSlopeProject = 0;
			iYAxis = -1;
		}
		//the project straight line slope 
		//0: slope is 0 (parallel with x axis)
		//1: slope is infinite(parallel with y axis)
		//2: other value
		int iYAxis;
		float fSlopeProject;
		QVector3D qv3SrcMin;
		QVector3D qv3SrcMax;
		QVector3D qv3InsertMin;
		QVector3D qv3InsertMax;
	};

	struct SModelPolygon
	{
		SModelPolygon()
		{
			bOptimise = false;
		}
		~SModelPolygon()
		{
			bOptimise = false;
		}
		QVector<QVector3D> ptPolygon;
		bool bOptimise;
		S3DPrtRectF rcGrobalDst;
		QVector3D ptGrobalCenter;//the grobal rectangle center
		QVector<STwoSegmentsProject> vtPolygonProject;
	};

	class QTUSER_3D_API CLayoutAlg
	{
	public:
		CLayoutAlg();
		virtual ~CLayoutAlg();

	public:
		/*******************************************************************
		describe:
		when you get one point, you want to check this point is inside the polygon or not. you need to know the principle. If this point is inside the polygon and this polygon is convex hull, any straight line through this point, it will cross the edge of this polygon.
		and you will got two points that they are on the edge of this polygon. at the same time, this point is at the middle of these two points. for using this principle, I use the line that is a special and it will reduce the complex that calculate out the two points value. at this time, I use the line that is parelle with x axis.
		so the formula of this line is y=point.y. when I use this line formula, this line will cross the edge line of the polygon. you will get two points, you just compare their x axis value, then you can check the point is at the middle of these two points or not.

		parameter:
		ptPlatform: the platform rectangle global position
		plgGroup: all the model polygons global position
		plgInsert: the model polygon global position will be installed
		ptInsert: the global position will be installed and this position is the box center on platform

		return:
		0: get the valid destination point
		1, this polygon is large than this platform
		2, this polygon is less than this platform, but it can't find the valid position to layout this model
		author:Gavin
		date:june/sixteenth day/2020
		******************************************************************/
		int InsertOnePolygon(const S3DPrtRectF& ptPlatform, const QVector<SModelPolygon>& plgGroup, SModelPolygon plgInsert, S3DPrtPointF& ptInsert, int iModelGap);

		/*******************************************************************
	describe:
	at first, if these two model polygon position are not optimized, bOptimise value is false, at this time, in this function, I will optimize all the points of this polygon. you can check the optimize algorithm function "OrderVetex" and "DeleteDumplicateVetex"
	we will project all the polygon edge vetex on one straight line and this straight line is y=k*x. all these straight lines are perpendiculat with the straight line segment of the polygon. after we get all these perpendiculat straight line. we project all the vertexes on these straight lines. one the straight line, all one polygon vetex are projected on this straight line and it will be one straight line segment. for another polygon, all its vetexes will be projected on
	this straight line and they will make up one straight line segment too. if these two segment line meet at one point. it means that these two polygon must meet at some place.

	parameter:
	vtSrcPolygon: the destination global polygon positon on the platform(use the member variable ptPolygon of vtSrcPolygon)
	vtDstpolygon: the destination global polygon positon on the platform(use the member variable ptPolygon of vtSrcPolygon)
	iModelGap: the model distance threshold, if the distance is less than this value, it will return true. or else , it will return false


	return:
	true: these two polygons collide
	false: these two polygons not collide

	author:yaoyaping
	date:june/eighteenth day/2020
	******************************************************************/
		bool CheckTwoPolygonCollideByProtectedWay(SModelPolygon vtSrcPolygon, SModelPolygon vtDstpolygon, int iModelGap);


		/*******************************************************************
	describe:
	we will project all the polygon edge vetex on one straight line and this straight line is y=k*x. all these straight lines are perpendiculat with the straight line segment of the polygon. after we get all these perpendiculat straight line. we project all the vertexes on these straight lines. one the straight line, all one polygon vetex are projected on this straight line and it will be one straight line segment. for another polygon, all its vetexes will be projected on
	this straight line and they will make up one straight line segment too. if these two segment line meet at one point. it means that these two polygon must meet at some place.
	parameter:
	rcGrobalDst: the destination global box rectangle on the platform
	rcGrobalSrc: the destination global box rectangle on the platform
	iModelGap: the model distance threshold, if the distance is less than this value, it will return true. or else , it will return false


	return:
	true: these two polygons collide
	false: these two polygons not collide

	author:yaoyaping
	date:june/eighteenth day/2020
	******************************************************************/
		bool CheckTwoPolygonCollideByBoxWay(const S3DPrtRectF& rcGrobalDst, const S3DPrtRectF& rcGrobalSrc, int iModelGap);


	protected:

		//these three functions are used to calculate the destination position on the outside of the platform
	private:
		/*******************************************************************
describe:
layout the model ontside of the platform. from the bottom right of the platform and the direction is the anticlockwise. the step is one. for checking the model collide with the group models or not, the collide algorithm is checking the circumscribe rectangle meet or not.
how to draw the circumscribe rectangle. the top and bottom side line of the rectangle should be parallel with the x axis. and the left and right side line should be parallel with y axis. the top side line is parallel with the x axis, so this line formula is y = b. for the value of b, it is the largest value of the polygon. 
for the bottom line of this circumscribe rectangle,it is parallel with x axis, so its formula is y=b and this b value is most small value of this polygon. for the left and right side line, they are parallel with y axis, so their formula is x = a. for this a value, they are the most small and big value of the convex polygon x asix value.
depends on this principle, we check these models circumscribe rectangles. if all these cricumscribe rectangle meet together. we think these two models meet together. or eles, they are separate. however, use the circumscribe rectangle, it is not accurate to check the two models meet or not. if these two model circumscribe rectangles meet together. we can not think these two 
models meet. however, this algorithm is very simple and it just need a little time. for the outside of the platform, the space is very large, I need we can use this algorithm and we can get the destination position of this model very fast.

parameter:
plgGroup: all the model polygons
plgInsert: this model polygon will be inserted
ptDst:	the destination position

return:
0: get the valid destination point in platform
1, this polygon is large than this platform

author:Gavin
date:june/twenty-ninth day/2020
******************************************************************/
		int GetDstPointOutPlatform(const QVector<SModelPolygon>& plgGroup, SModelPolygon plgInsert, S3DPrtPointF& ptDst);
		bool IsDstPointValidOutPlatform(S3DPrtPointF ptIDst, const QVector<SModelPolygon>& plgGroup, const SModelPolygon& plgInsert);

		/*******************************************************************
describe:
check the destination postion is in the circumscribe rectangle or not
parameter:
plgGroup: all the model polygons
ptDst:	the destination position

return:
true: this destination position is in the group rectangle
false, this destination position is not in the group rectangle

author:Gavin
date:june/twenty-ninth day/2020
******************************************************************/
		bool IsInsidePoligonGroupByBox(const QVector<SModelPolygon>& plgGroup, S3DPrtPointF ptDst);


		//these functions are used to improve the speed of calculating the destination postion inside of platform
	private:
		/*******************************************************************
describe:
find a valid positon in the platform

parameter:
plgGroup: all the model polygons
plgInsert: this model polygon will be inserted
ptDst:	the destination position

return:
0: get the valid destination point in platform
1, this polygon is large than this platform

author:Gavin
date:june/sixteenth day/2020
******************************************************************/
		int InsertModelInPlatform(const QVector<SModelPolygon>& plgGroup, SModelPolygon plgInsert, S3DPrtPointF& ptDst);

		/*******************************************************************
describe:
calculate all the line segment that one model convex polygon is projected on the y=kx. and this straight line y=kx is parallel with one side of the convex polygon.

parameter:
plgGroup: all the model polygons
plgInsert: this model polygon will be inserted

return:
true: get all the segment of the model convex polygon successfully

author:Gavin
date:june/sixteenth day/2020
******************************************************************/
		bool InitializeAllProjectSegment(QVector<SModelPolygon>& plgGroup, SModelPolygon& plgInsert);

		/*******************************************************************
describe:
calculate one line segment that one model convex polygon is projected on the y=kx. and this straight line y=kx is parallel with the side of the convex polygon. the side of the convex polygon is from "qPntStart" to "qPntEnd". 

parameter:
vtProjectSegment: save the segment that the convex polygon is project on the y=kx.
vtSrcPolygon: all the points that one model convex polygon and all these points are optimized.
vtInsertPolygon: all the points that one model convex polygon and all these points are optimized.
qPntStart: the start point of one side of the convex polygon
qPntEnd: the end point of one side of the convex polygon

return:
true: get all the segment of the model convex polygon successfully

author:Gavin
date:june/sixteenth day/2020
******************************************************************/
		bool InitializeProjectPoint(STwoSegmentsProject& vtProjectSegment, const QVector<QVector3D>& vtSrcPolygon, const QVector<QVector3D>& vtInsertPolygon, const QVector3D& qPntStart, const QVector3D& qPntEnd);

		/*******************************************************************
describe:
all these four functions is to calculate one line segment that one model convex polygon is projected on the y=kx. and this straight line y=kx is parallel with the side of the convex polygon. the side of the convex polygon is from "qPntStart" to "qPntEnd".

parameter:
vtProjectSegment: save the segment that the convex polygon is project on the y=kx.
vtSrcPolygon: all the points that one model convex polygon and all these points are optimized.
vtInsertPolygon: all the points that one model convex polygon and all these points are optimized.

return:
true: get all the segment of the model convex polygon successfully

author:Gavin
date:june/sixteenth day/2020
******************************************************************/
		bool GetProjectSegmentOnXAxis(STwoSegmentsProject& vtProjectSegment, const QVector<QVector3D>& vtSrcPolygon, const QVector<QVector3D>& vtInsertPolygon);
		bool GetProjectSegmentOnYAxis(STwoSegmentsProject& vtProjectSegment, const QVector<QVector3D>& vtSrcPolygon, const QVector<QVector3D>& vtInsertPolygon);
		bool GetProjectSegmentOnLess1(STwoSegmentsProject& vtProjectSegment, const QVector<QVector3D>& vtSrcPolygon, const QVector<QVector3D>& vtInsertPolygon, float fKSlope);
		bool GetProjectSegmentOnLarge1(STwoSegmentsProject& vtProjectSegment, const QVector<QVector3D>& vtSrcPolygon, const QVector<QVector3D>& vtInsertPolygon, float fKSlope);

		/*******************************************************************
describe:
in the previous functions, all the project line segments are saved in the list. when you check the destination position is valid or not, you just need to move all the project line segment along the vector. using this way, it will reduce a lot of the time.

parameter:
ptIDst: the destination positon
vtSrcPolygon: all the points that one model convex polygon and all these points are optimized.
vtInsertPolygon: all the points that one model convex polygon and all these points are optimized.

return:
true: get all the segment of the model convex polygon successfully

author:Gavin
date:june/sixteenth day/2020
******************************************************************/
		bool IsDstPointValidNewWay(S3DPrtPointF ptIDst, QVector<SModelPolygon>& plgGroup, const SModelPolygon& plgInsert);


		/*******************************************************************
describe:
calculate the vector and move all the center point, circumscribe rectangle along this vector.

parameter:
ptIDst: the destination positon
dstPolygon: all the points that one model convex polygon and all these points are optimized.
plgInsert: this polygon will be inserted

return:
QVector3D: return the vector that these insert porject line segment that is need to moved

author:Gavin
date:june/sixteenth day/2020
******************************************************************/
		QVector3D CalculateNewPointNewWay(QVector3D ptDst, SModelPolygon& dstPolygon, const SModelPolygon& plgInsert);


		/*******************************************************************
describe:
after move all the insert mode project line segment, check they meet with the source project line segments or not

parameter:
vtTranslate: the destination positon to the first positon of the inset model
plgGroup: all the points that one model convex polygon and all these points are optimized.
iModelGap: these models gap distance

return:
true: the insert polygon project line segment meet with one source mode polygon
false: they not meet

author:Gavin
date:june/sixteenth day/2020
******************************************************************/
		bool CheckTwoPolygonCollideByProtectedNewWay(QVector3D vtTranslate, QVector<SModelPolygon>& plgGroup, int iModelGap);


		/**********************************below functions are used to calculate one point is in the polygon or not*******************************************************************************************/
	private:
		/*******************************************************************
		describe:
		check this point is in the polygon group or not and return the polygon index

		parameter:
		plgGroup: the polygon group
		ptDst: check this point

		return:
		-1: this point is not in one polygon of this polygon group
		other value: this point is in one polygon of this polygon group and this value is the index of the polygon in this polygon group.

		author:Gavin
		date:june/ninteenth day/2020
		******************************************************************/
		int IsInsidePoligonGroup(const QVector<SModelPolygon>& plgGroup, S3DPrtPointF ptDst);


		/*******************************************************************
		describe:
		I check this point is in the polygon or not. the principle is that the point is in the polygon. in this case, we use one straight line go through this point and we will find this line will meet with the polygon edge. and this straight line will get even number points to meet with this edge. this straight line slope can be any number.
		at this time, I use the slope to be zero and it will be more easy to calculate. all these polygons are the convex hull. so any straight lines meet with the polygon edge and the meet points count is two. if we check this point is in this polygon or not. we need to check this point is between these two meet points or not. if this point is
		between these two meet points, this point is in polygon, or else it is not.

		parameter:
		plgGroup: the polygon
		ptDst: the point need to be checked

		return:
		false: this point is not in one polygon of this polygon group
		true: this point is in one polygon of this polygon group.


		author:Gavin
		date:june/eighteenth day/2020
		******************************************************************/
		bool IsInsidePoligon(const SModelPolygon& vtPolygon, S3DPrtPointF ptDst);

		/*******************************************************************
		describe:
		when you get one point, you want to check this point is inside the polygon or not. you need to know the principle. If this point is inside the polygon and this polygon is convex hull, any straight line through this point, it will cross the edge of this polygon.
		and you will got two points that they are on the edge of this polygon. at the same time, this point is at the middle of these two points. for using this principle, I use the line that is a special and it will reduce the complex that calculate out the two points value. at this time, I use the line that is parelle with x axis.
		so the formula of this line is y=point.y. when I use this line formula, this line will cross the edge line of the polygon. you will get two points, you just compare their x axis value, then you can check the point is at the middle of these two points or not.

		parameter:
		qPntStart: the straight line segment start endpoint
		qPntEnd: the stragiht line segment end
		iY: draw one straight line y = iY
		iXCross: this is the y=iY straight line meet with the polygon edge straight line segment(qPntStart, qPntEnd).


		return:
		true: the straight line y=iY meet with the straight line segment(qPntStart, qPntEnd).
		false: the straight line y=iY doesn't meet with the straight line segment(qPntStart, qPntEnd).
		author:Gavin
		date:june/sixteenth day/2020
		******************************************************************/
		bool GetCrossY(const QVector3D& qPntStart, const QVector3D& qPntEnd, int iY, int& iXCross);
		/*------------------------------------------------------------------end----------------------------------------------------------------*/


	private:

		bool IsDstPointValid(S3DPrtPointF ptIDst, const QVector<SModelPolygon>& plgGroup, const SModelPolygon& srcPolygon);


		/*******************************************************************
		describe:
		move all the polygon edge points into the new position

		parameter:

		return:

		author:Gavin
		date:june/sixteenth day/2020
		******************************************************************/
		void CalculateNewPoint(QVector3D ptDst, SModelPolygon& dstPolygon, const SModelPolygon& srcPolygon);

		/*******************************************************************
	describe:
	call "OrderVetex" and "DeleteDumplicateVetex" to reduce the vetexes.

	parameter:
	modelInfor: reduce and order these polygons edge vetexes.
	return:

	author:yaoyaping
	date:june/seventeen day/2020
	******************************************************************/
		S3DPrtRectF GetPlatform();


		//-----------------------------------------------collect all the polygon vetexes--------------------------------------------------------------------------------------------
	protected:
		/*******************************************************************
		describe:
		call "OrderVetex" and "DeleteDumplicateVetex" to reduce the vetexes.

		parameter:
		modelInfor: reduce and order these polygons edge vetexes.
		return:

		author:yaoyaping
		date:june/seventeen day/2020
		******************************************************************/
		void CollectVetex(SModelPolygon& modelInfor);


		/*******************************************************************
		describe:
		order all the vertexes anticlockwise.at first, get the first vetex, order all the vetexes depends on the x axis value from small to large. after this, get the first item, this item is the vetex that the x axis value is the most small.
		on the second step, we compare all other vetexes with the first vetex y axis value. all the up vetex put into one vector and the below ones are put into the another one vector. we will put all the below vetexes vector into a collect vector from the index 0 to the last one.
		after this, we will put the up vetexes vector into this collect vector one by one from the last one to the first one. after all these steps, we will get all the vetexs that are the anticlockwise. basically, this polygon is the convex hull.

		parameter:

		return:

		author:yaoyaping
		date:june/seventeen day/2020
		******************************************************************/
		void OrderVetex(QVector<QVector3D>& polygons);

		/*******************************************************************
		describe:
		I compare one vetex to all the other vetexes. if these two vetex are in a unit pixel. we think these two vetex are the same position. I will delete one vetex and than save one. using this one to compare with all the other vetex like this way.

		parameter:

		return:

		author:yaoyaping
		date:june/seventeen day/2020
		******************************************************************/
		void DeleteDumplicateVetex(QVector<QVector3D>& polygons);

		//-----------------------------------------------check two model polygons meet or not--------------------------------------------------------------------------------------------
	protected:

		/*******************************************************************
	describe:
	we will project all the polygon edge vetex on one straight line and this straight line is y=k*x. all these straight lines are perpendiculat with the straight line segment of the polygon. after we get all these perpendiculat straight line. we project all the vertexes on these straight lines. one the straight line, all one polygon vetex are projected on this straight line and it will be one straight line segment. for another polygon, all its vetexes will be projected on
	this straight line and they will make up one straight line segment too. if these two segment line meet at one point. it means that these two polygon must meet at some place.
	parameter:

	return:
	0: these two polygons meet

	author:yaoyaping
	date:june/eighteenth day/2020
	******************************************************************/
		bool CheckTwoPolygonCollideOnLine(const QVector<QVector3D>& vtSrcPolygon, const QVector<QVector3D>& vtPointProjected, const QVector3D& qPntStart, const QVector3D& qPntEnd, float& fDistance);


		/*******************************************************************
describe:
for the mesh fixes, there are ten functions in the function list of the ultimaker cura. I researched these function in this afternoon.

parameter:

return:
0: these two polygons meet

author:yaoyaping
date:june/eighteenth day/2020
******************************************************************/
		bool CheckTwoPolygonCollideOnXAxis(const QVector<QVector3D>& vtSrcPolygon, const QVector<QVector3D>& vtPointProjected, float fKSlope, float& fDistance);
		bool CheckTwoPolygonCollideOnYAxis(const QVector<QVector3D>& vtSrcPolygon, const QVector<QVector3D>& vtPointProjected, float fKSlope, float& fDistance);
		bool CheckTwoPolygonCollideOnLess1(const QVector<QVector3D>& vtSrcPolygon, const QVector<QVector3D>& vtPointProjected, float fKSlope, float& fDistance);
		bool CheckTwoPolygonCollideOnLarge1(const QVector<QVector3D>& vtSrcPolygon, const QVector<QVector3D>& vtPointProjected, float fKSlope, float& fDistance);

		QVector3D GetCrossPoint(const QVector3D& vtSrcPolygon, float fKSlope, float fKProjectLineSlope);

		//---------------------------------------------------------------end check two polygons collide--------------------------------------------------------------------------------------------


	private:
		S3DPrtRectF  m_rcPlatform;
		S3DPrtPointF m_ptPlatformCenter;
		int m_iUnit;

		int m_iModelGap;
	};

}
#endif // _NULLSPACE_CLayoutAlg_1589765331924_H
