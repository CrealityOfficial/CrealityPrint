cmake_minimum_required(VERSION 3.13)
project(libslic3r)

set(CMAKE_CXX_STANDARD 17)
string(TIMESTAMP COMPILE_TIME %Y%m%d-%H%M%S)
set(SLIC3R_BUILD_TIME ${COMPILE_TIME})

#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libslic3r_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/libslic3r_version.h @ONLY)
set(OPTIMIZE_SOURCE 1)

__enable_bigobj()

set(OpenVDBUtils_SOURCES "")
if (TARGET OpenVDB::openvdb)
    set(OpenVDBUtils_SOURCES OpenVDBUtils.cpp OpenVDBUtils.hpp)
endif()

set(lisbslic3r_sources
    ArcFitter.cpp
    ArcFitter.hpp
    AABBTreeIndirect.hpp
    AABBTreeLines.hpp
    Algorithm/PathSorting.hpp
    Algorithm/RegionExpansion.hpp
    Algorithm/RegionExpansion.cpp
    AnyPtr.hpp
    BoundingBox.cpp
    BoundingBox.hpp
    BridgeDetector.cpp
    BridgeDetector.hpp
    Brim.cpp
    Brim.hpp
    BuildVolume.cpp
    BuildVolume.hpp
	ColorSpaceConvert.hpp
	ColorSpaceConvert.cpp
    Circle.cpp
    Circle.hpp
    clipper.cpp
    clipper.hpp
    ClipperUtils.cpp
    ClipperUtils.hpp
    Clipper2Utils.cpp
    Clipper2Utils.hpp
    ClipperZUtils.hpp
    Color.cpp
    Color.hpp
    Config.cpp
    Config.hpp
    CurveAnalyzer.cpp
    CurveAnalyzer.hpp
    EdgeGrid.cpp
    EdgeGrid.hpp
    ElephantFootCompensation.cpp
    ElephantFootCompensation.hpp
    Emboss.cpp
    Emboss.hpp
    EmbossShape.hpp
    enum_bitmask.hpp
    ExPolygon.cpp
    ExPolygon.hpp
    ExPolygonSerialize.hpp
    ExPolygonsIndex.cpp
    ExPolygonsIndex.hpp
    Extruder.cpp
    Extruder.hpp
    ExtrusionEntity.cpp
    ExtrusionEntity.hpp
    ExtrusionEntityCollection.cpp
    ExtrusionEntityCollection.hpp
    ExtrusionSimulator.cpp
    ExtrusionSimulator.hpp
    FileParserError.hpp
    Fill/Fill.cpp
    Fill/Fill.hpp
    Fill/Fill3DHoneycomb.cpp
    Fill/Fill3DHoneycomb.hpp
    Fill/FillAdaptive.cpp
    Fill/FillAdaptive.hpp
    Fill/FillBase.cpp
    Fill/FillBase.hpp
    Fill/FillConcentric.cpp
    Fill/FillConcentric.hpp
    Fill/FillConcentricInternal.cpp
    Fill/FillConcentricInternal.hpp
    Fill/FillHoneycomb.cpp
    Fill/FillHoneycomb.hpp
	Fill/FillCross.cpp
    Fill/FillCross.hpp
    Fill/FillGyroid.cpp
    Fill/FillGyroid.hpp
    Fill/FillPlanePath.cpp
    Fill/FillPlanePath.hpp
    Fill/FillLine.cpp
    Fill/FillLine.hpp
    Fill/FillLightning.cpp
    Fill/FillLightning.hpp
    Fill/Lightning/DistanceField.cpp
    Fill/Lightning/DistanceField.hpp
    Fill/Lightning/Generator.cpp
    Fill/Lightning/Generator.hpp
    Fill/Lightning/Layer.cpp
    Fill/Lightning/Layer.hpp
    Fill/Lightning/TreeNode.cpp
    Fill/Lightning/TreeNode.hpp
	Fill/Cross/SierpinskiFillProvider.h
    Fill/Cross/SierpinskiFillProvider.cpp
    Fill/Cross/SierpinskiFill.h
    Fill/Cross/SierpinskiFill.cpp
    Fill/Cross/ImageBasedDensityProvider.h
    Fill/Cross/ImageBasedDensityProvider.cpp
    Fill/FillRectilinear.cpp
    Fill/FillRectilinear.hpp
	Fill/FillQuarter.cpp
    Fill/FillQuarter.hpp
	Fill/FillTetrahedral.cpp
    Fill/FillTetrahedral.hpp
	Fill/Quarter/NoZigZagConnectorProcessor.cpp
    Fill/Quarter/NoZigZagConnectorProcessor.h
	Fill/Quarter/ZigzagConnectorProcessor.cpp
    Fill/Quarter/ZigzagConnectorProcessor.h
    Flow.cpp
    Flow.hpp
    FlushVolCalc.cpp
    FlushVolCalc.hpp
    format.hpp
    Format/3mf.cpp
    Format/3mf.hpp
    Format/bbs_3mf.cpp
    Format/bbs_3mf.hpp
    GCode/ThumbnailData.cpp
    GCode/ThumbnailData.hpp
    GCode/CoolingBuffer.cpp
    GCode/CoolingBuffer.hpp
	GCode/FanMover.cpp
    GCode/FanMover.hpp
    GCode/PostProcessor.cpp
    GCode/PostProcessor.hpp
    GCode/PressureEqualizer.cpp
    GCode/PressureEqualizer.hpp
    GCode/PrintExtents.cpp
    GCode/PrintExtents.hpp
    GCode/RetractWhenCrossingPerimeters.cpp
    GCode/RetractWhenCrossingPerimeters.hpp
    GCode/SpiralVase.cpp
    GCode/SpiralVase.hpp
    GCode/SeamPlacer.cpp
    GCode/SeamPlacer.hpp
    GCode/ToolOrdering.cpp
    GCode/ToolOrdering.hpp
    GCode/WipeTower.cpp
    GCode/WipeTower.hpp
    GCode/WipeTower2.cpp
    GCode/WipeTower2.hpp
    GCode/GCodeProcessor.cpp
    GCode/GCodeProcessor.hpp
    GCode/AvoidCrossingPerimeters.cpp
    GCode/AvoidCrossingPerimeters.hpp
    GCode/ExtrusionProcessor.hpp
    GCode/ConflictChecker.cpp
    GCode/ConflictChecker.hpp
	GCode/smoothspeedandacc.h
	GCode/smoothspeedandacc.cpp
	GCode/SmallAreaInfillFlowCompensator.hpp
	GCode/SmallAreaInfillFlowCompensator.cpp
    GCode.cpp
    GCode.hpp
    GCodeReader.cpp
    GCodeReader.hpp
    GCodeWriter.cpp
    GCodeWriter.hpp
    Geometry.cpp
    Geometry.hpp
    Geometry/Bicubic.hpp
    Geometry/Circle.cpp
    Geometry/Circle.hpp
    Geometry/ConvexHull.cpp
    Geometry/ConvexHull.hpp
    Geometry/Curves.hpp
    Geometry/MedialAxis.cpp
    Geometry/MedialAxis.hpp
    Geometry/Voronoi.hpp
    Geometry/VoronoiOffset.cpp
    Geometry/VoronoiOffset.hpp
    Geometry/VoronoiUtilsCgal.cpp
    Geometry/VoronoiUtilsCgal.hpp
    Geometry/VoronoiVisualUtils.hpp
    Int128.hpp
    KDTreeIndirect.hpp
    Layer.cpp
    Layer.hpp
    LayerRegion.cpp
    libslic3r.h
    Line.cpp
    Line.hpp
    LocalesUtils.cpp
    LocalesUtils.hpp
    Model.cpp
    Model.hpp
    MultiMaterialSegmentation.cpp
    MultiMaterialSegmentation.hpp
    CustomGCode.cpp
    CustomGCode.hpp
    Orient.hpp
    Orient.cpp
    MultiPoint.cpp
    MultiPoint.hpp
    MutablePriorityQueue.hpp
    NSVGUtils.cpp
    NSVGUtils.hpp
    ObjectID.cpp
    ObjectID.hpp
    PerimeterGenerator.cpp
    PerimeterGenerator.hpp
    PlaceholderParser.cpp
    PlaceholderParser.hpp
    Point.cpp
    Point.hpp
    Polygon.cpp
    Polygon.hpp
    MutablePolygon.cpp
    MutablePolygon.hpp
    Polyline.cpp
    Polyline.hpp
    Preset.cpp
    Preset.hpp
    PresetBundle.cpp
    PresetBundle.hpp
    ProjectTask.cpp
    ProjectTask.hpp
    PrincipalComponents2D.hpp
    PrincipalComponents2D.cpp
    AppConfig.cpp
    AppConfig.hpp
    Print.cpp
    Print.hpp
    PrintApply.cpp
    PrintBase.cpp
    PrintBase.hpp
    PrintConfig.cpp
    PrintConfig.hpp
    PrintObject.cpp
    PrintObjectSlice.cpp
    PrintRegion.cpp
    Semver.cpp
    ShortEdgeCollapse.cpp
    ShortEdgeCollapse.hpp
    ShortestPath.cpp
    ShortestPath.hpp
    Slicing.cpp
    Slicing.hpp
    SlicesToTriangleMesh.hpp
    SlicesToTriangleMesh.cpp
    SlicingAdaptive.cpp
    SlicingAdaptive.hpp
    SupportMaterial.cpp
    SupportMaterial.hpp
    PrincipalComponents2D.cpp
    PrincipalComponents2D.hpp
    SupportSpotsGenerator.cpp
    SupportSpotsGenerator.hpp
    
	TreeSupport.hpp 
    TreeSupport.cpp
	Support/SupportCommon.cpp
	Support/SupportCommon.hpp
	Support/SupportDebug.cpp
	Support/SupportDebug.hpp
	Support/SupportLayer.hpp
	Support/SupportParameters.cpp
	Support/SupportParameters.hpp
	Support/OrganicSupport.cpp
	Support/OrganicSupport.hpp
	Support/TreeSupport.cpp
	Support/TreeSupport.hpp
	Support/TreeSupportCommon.cpp
	Support/TreeSupportCommon.hpp
	Support/TreeModelVolumes.cpp
	Support/TreeModelVolumes.hpp
			
    MinimumSpanningTree.hpp
    MinimumSpanningTree.cpp
    Surface.cpp
    Surface.hpp
    SurfaceCollection.cpp
    SurfaceCollection.hpp
    SurfaceMesh.hpp
    SVG.cpp
    SVG.hpp
    Technologies.hpp
    Tesselate.cpp
    Tesselate.hpp
    TextConfiguration.hpp
    TriangleMesh.cpp
    TriangleMesh.hpp
    TriangleMeshSlicer.cpp
    TriangleMeshSlicer.hpp
    MeshSplitImpl.hpp
    TriangulateWall.hpp
    TriangulateWall.cpp
    utils.cpp
    Utils.hpp
    Time.cpp
    Time.hpp
    Timer.cpp
    Timer.hpp
    Thread.cpp
    Thread.hpp
    TriangleSelector.cpp
    TriangleSelector.hpp
    TriangleSetSampling.cpp
    TriangleSetSampling.hpp
    MTUtils.hpp
    VariableWidth.cpp
    VariableWidth.hpp
	NormalUtils.cpp
	NormalUtils.hpp
    MinAreaBoundingBox.hpp
    MinAreaBoundingBox.cpp
    miniz_extension.hpp
    miniz_extension.cpp
    MarchingSquares.hpp
    Execution/Execution.hpp
    Execution/ExecutionSeq.hpp
    Execution/ExecutionTBB.hpp
    Optimize/Optimizer.hpp
    Optimize/NLoptOptimizer.hpp
    Optimize/BruteforceOptimizer.hpp
    Arachne/BeadingStrategy/BeadingStrategy.hpp
    Arachne/BeadingStrategy/BeadingStrategy.cpp
    Arachne/BeadingStrategy/BeadingStrategyFactory.hpp
    Arachne/BeadingStrategy/BeadingStrategyFactory.cpp
    Arachne/BeadingStrategy/DistributedBeadingStrategy.hpp
    Arachne/BeadingStrategy/DistributedBeadingStrategy.cpp
    Arachne/BeadingStrategy/LimitedBeadingStrategy.hpp
    Arachne/BeadingStrategy/LimitedBeadingStrategy.cpp
    Arachne/BeadingStrategy/OuterWallInsetBeadingStrategy.hpp
    Arachne/BeadingStrategy/OuterWallInsetBeadingStrategy.cpp
    Arachne/BeadingStrategy/RedistributeBeadingStrategy.hpp
    Arachne/BeadingStrategy/RedistributeBeadingStrategy.cpp
    Arachne/BeadingStrategy/WideningBeadingStrategy.hpp
    Arachne/BeadingStrategy/WideningBeadingStrategy.cpp
    Arachne/utils/ExtrusionJunction.hpp
    Arachne/utils/ExtrusionJunction.cpp
    Arachne/utils/ExtrusionLine.hpp
    Arachne/utils/ExtrusionLine.cpp
    Arachne/utils/HalfEdge.hpp
    Arachne/utils/HalfEdgeGraph.hpp
    Arachne/utils/HalfEdgeNode.hpp
    Arachne/utils/SparseGrid.hpp
    Arachne/utils/SparsePointGrid.hpp
    Arachne/utils/SparseLineGrid.hpp
    Arachne/utils/SquareGrid.hpp
    Arachne/utils/SquareGrid.cpp
    Arachne/utils/PolygonsPointIndex.hpp
    Arachne/utils/PolygonsSegmentIndex.hpp
    Arachne/utils/PolylineStitcher.hpp
    Arachne/utils/PolylineStitcher.cpp
    Arachne/utils/VoronoiUtils.hpp
    Arachne/utils/VoronoiUtils.cpp
    Arachne/SkeletalTrapezoidation.hpp
    Arachne/SkeletalTrapezoidation.cpp
    Arachne/SkeletalTrapezoidationEdge.hpp
    Arachne/SkeletalTrapezoidationGraph.hpp
    Arachne/SkeletalTrapezoidationGraph.cpp
    Arachne/SkeletalTrapezoidationJoint.hpp
    Arachne/WallToolPaths.hpp
    Arachne/WallToolPaths.cpp
    calib.hpp
    calib.cpp
    GCode/Thumbnails.cpp
    GCode/Thumbnails.hpp	
	NanoSVGImpl.cpp
	Platform.cpp
	Platform.hpp
	CutUtils.cpp
	CutUtils.hpp
	Debugger.hpp
	Debugger.cpp
)

set(EXTRA_SOURCES 	
	../semver/semver.h
	../semver/semver.c
	../clipper/clipper_z.hpp
	../clipper/clipper_z.cpp
	../admesh/connect.cpp
    ../admesh/normals.cpp
    ../admesh/shared.cpp
    ../admesh/stl.h
    ../admesh/stl_io.cpp
    ../admesh/stlinit.cpp
    ../admesh/util.cpp
	miniz.h
	miniz.c
	../qoi/qoi.h
	../qoi/qoilib.c
	)

list(APPEND lisbslic3r_sources 
	#"${CMAKE_CURRENT_BINARY_DIR}/libslic3r_version.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/libslic3r_version.h"
	)
if (APPLE)
    list(APPEND lisbslic3r_sources
			MacUtils.mm
            Format/ModelIO.hpp
            Format/ModelIO.mm
        )
endif ()

list(APPEND lisbslic3r_sources
    	IntersectionPoints.hpp
		IntersectionPoints.cpp
    	Triangulation.hpp
		Triangulation.cpp
	)
	
__cc_find(boost_static)
__cc_find(eigen)
__cc_find(admesh)
__cc_find(clipper2)
__cc_find(png_static)
__cc_find(openssl)
__cc_find(nlopt_static)
__cc_find(expat)

__assert_target(clipper2)
__assert_target(eigen)
__assert_target(admesh)
__assert_target(nlopt_static)
__assert_target(expat)

set(libslic3r_dir ${CMAKE_CURRENT_SOURCE_DIR}/../)
set(ILIBS)
set(INTERFACES ${libslic3r_dir} ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../extra/
	)
set(INCS ${INTERFACES})
set(DEFS)

if(WIN32)
    list(APPEND ILIBS Psapi.lib)
endif()

list(APPEND ILIBS eigen clipper2 admesh boost_filesystem boost_nowide boost_thread imgui
			 boost_regex png_static ssl crypto expat nlopt_static qhullstatic glu-libtess)
list(APPEND DEFS BOOST_ALL_NO_LIB TBB_USE_CAPTURED_EXCEPTION=0
	_CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES NOMINMAX UNICODE)

if(USE_BENCH_DEBUG)
	list(APPEND DEFS BENCH_DEBUG) 
endif()

if(TARGET tbb_inner)
	list(APPEND DEFS USE_TBB)
	list(APPEND ILIBS tbb_inner tbbmalloc)
endif()

__cc_find(cgal)
__assert_target(cgal)
list(APPEND ILIBS cgal)
	
set(INTERFACE_DEFS )
if(OPTIMIZE_SOURCE)
	list(APPEND DEFS MINIMAL_ENGINE)
	list(APPEND INTERFACE_DEFS MINIMAL_ENGINE)
else()
	list(APPEND lisbslic3r_sources
			AABBMesh.hpp
			AABBMesh.cpp
			QuadricEdgeCollapse.cpp
			QuadricEdgeCollapse.hpp
			CutSurface.hpp
			CutSurface.cpp
			#MeshBoolean.hpp
			#MeshBoolean.cpp
			Format/AMF.cpp
			Format/AMF.hpp
			Format/OBJ.cpp
			Format/OBJ.hpp
			Format/objparser.cpp
			Format/objparser.hpp
			Format/STEP.cpp
			Format/STEP.hpp
			Format/STL.cpp
			Format/STL.hpp
			Format/SL1.hpp
			Format/SL1.cpp
			Format/svg.hpp
			Format/svg.cpp
			
			Measure.hpp
			Measure.cpp
			MeasureUtils.hpp
			PolygonTrimmer.cpp
			PolygonTrimmer.hpp
			FaceDetector.cpp
			FaceDetector.hpp
			Arrange.hpp
			Arrange.cpp
			ModelArrange.hpp
			ModelArrange.cpp
	
			SLAPrint.cpp
			SLAPrintSteps.cpp
			SLAPrintSteps.hpp
			SLAPrint.hpp
			SLA/Pad.hpp
			SLA/Pad.cpp
			SLA/SupportTreeBuilder.hpp
			SLA/SupportTreeMesher.hpp
			SLA/SupportTreeMesher.cpp
			SLA/SupportTreeBuildsteps.hpp
			SLA/SupportTreeBuildsteps.cpp
			SLA/SupportTreeBuilder.cpp
			SLA/Concurrency.hpp
			SLA/SupportTree.hpp
			SLA/SupportTree.cpp
			#SLA/SupportTreeIGL.cpp
			SLA/Rotfinder.hpp
			SLA/Rotfinder.cpp
			SLA/BoostAdapter.hpp
			SLA/SpatIndex.hpp
			SLA/SpatIndex.cpp
			SLA/RasterBase.hpp
			SLA/RasterBase.cpp
			SLA/AGGRaster.hpp
			SLA/RasterToPolygons.hpp
			SLA/RasterToPolygons.cpp
			SLA/ConcaveHull.hpp
			SLA/ConcaveHull.cpp
			#SLA/Hollowing.hpp
			#SLA/Hollowing.cpp
			SLA/JobController.hpp
			SLA/SupportPoint.hpp
			SLA/SupportPointGenerator.hpp
			SLA/SupportPointGenerator.cpp
			SLA/IndexedMesh.hpp
			SLA/IndexedMesh.cpp
			SLA/Clustering.hpp
			SLA/Clustering.cpp
			SLA/ReprojectPointsOnMesh.hpp
			
			Shape/TextShape.hpp
			Shape/TextShape.cpp
	
			Zipper.hpp
			Zipper.cpp
			PNGReadWrite.hpp
			PNGReadWrite.cpp
			GCodeSender.cpp
			GCodeSender.hpp
			TryCatchSignal.hpp
			TryCatchSignal.cpp	
			BlacklistedLibraryCheck.cpp
			BlacklistedLibraryCheck.hpp
		)
	
	__cc_find(opencascade_static)
	set(OLIBS TKernel
			TKMath
			TKG2d
			TKG3d
			TKGeomBase
			TKBRep
			TKGeomAlgo
			TKTopAlgo
			TKPrim
			TKBO
			TKShHealing
			TKBool
			TKHLR
			TKMesh
			TKService
			TKV3d
			TKCDF
			TKLCAF
			TKCAF
			TKBinL
			TKXmlL
			TKBin
			TKXml
			TKVCAF
			TKXSBase
			TKSTEPBase
			TKSTEPAttr
			TKSTEP209
			TKSTEP
			TKIGES
			TKXCAF
			TKXDE
			TKXDEIGES
			TKXDESTEP
			TKSTL
			TKVRML
			TKRWMesh
			TKXmlXCAF
			TKBinXCAF     
			)
	list(REVERSE OLIBS)  # for linux build
	list(APPEND ILIBS ${OLIBS})
endif()

__add_real_target(libslic3r lib 
	SOURCE ${lisbslic3r_sources} ${OpenVDBUtils_SOURCES} ${EXTRA_SOURCES}
	ILIB ${ILIBS}
	INTERFACE ${INTERFACES}
	INC ${INCS}
	DEF ${DEFS}
	INTERFACE_DEF ${INTERFACE_DEFS}
	FOLDER slice
	OPENMP
	)
	
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${lisbslic3r_sources})
#encoding_check(libslic3r)


