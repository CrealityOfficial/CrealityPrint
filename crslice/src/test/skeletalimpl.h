#include "crslice/test/test_skeletal.h"
#include "../conv.h"

#include "tools/BoostInterface.h"

#include "skeletal/BeadingStrategyFactory.h"
#include "skeletal/SkeletalTrapezoidation.h"
#include "skeletal/VariableLineUtils.h"
#include "tools/Cache.h"
#include "utils/VoronoiUtils.h"
#include "utils/VoronoiUtilsCgal.h"
#include "utils/linearAlg2D.h"
#include "tools/SVG.h"

using namespace cura52;
namespace crslice
{
    typedef ClipperLib::IntPoint Point;
    typedef cura52::PolygonsSegmentIndex Segment;
    typedef boost::polygon::voronoi_cell<double> cell_type;
    typedef boost::polygon::voronoi_vertex<double> vertex_type;
    typedef boost::polygon::voronoi_edge<double> edge_type;
    typedef boost::polygon::segment_traits<Segment>::point_type point_type;
    typedef boost::polygon::voronoi_diagram<double> voronoi_type;

    using graph_t = SkeletalTrapezoidationGraph;
    using edge_t = STHalfEdge;
    using node_t = STHalfEdgeNode;
    using Beading = BeadingStrategy::Beading;
    using BeadingPropagation = SkeletalTrapezoidationJoint::BeadingPropagation;
    using TransitionMiddle = SkeletalTrapezoidationEdge::TransitionMiddle;
    using TransitionEnd = SkeletalTrapezoidationEdge::TransitionEnd;

    enum class EdgeDiscretizeType
    {
        edt_edge,
        edt_parabola,
        edt_straightedge
    };

    struct DiscretizeEdge
    {
        EdgeDiscretizeType type;
        edge_type* edge;
    };

    struct DiscretizeCell
    {
        Point start_source_point;
        Point end_source_point;
        edge_type* starting_vonoroi_edge = nullptr;
        edge_type* ending_vonoroi_edge = nullptr;
    };

	class SkeletalCheckImpl
	{
	public:
		SkeletalCheckImpl() {}
		~SkeletalCheckImpl() {}

        void setInput(const SerailCrSkeletal& skeletal);
        bool isValid();

        void detectMissingVoronoiVertex(CrMissingVertex& vertex);

        void skeletalTrapezoidation(CrPolygons& innerPoly, std::vector<CrVariableLines>& out,
            SkeletalDetail* detail = nullptr);
        void transferEdges(CrDiscretizeEdges& discretizeEdges);
        bool transferCell(int index, CrDiscretizeCell& discretizeCell);
        bool transferInvalidCell(int index, CrDiscretizeCell& discretizeCell);
        void transferGraph();

        void generateBoostVoronoiTxt(const std::string& fileName);
        void generateTransferEdgeSVG(const std::string& fileName);
        void generateNoPlanarVertexSVG(const std::string& fileName);
    private:
        void setParam(const CrSkeletalParam& param);
        void clear();
        void reset(const cura52::Polygons& poly);
        void modify();

        void classifyEdge();
        void checkNoPlanarVertex();

        int edgeIndex(edge_type* edge);
        void transfer();

        void insertEdge(const edge_type* edge, std::vector<trimesh::vec3>& points);
    public:
        cura52::Polygons input;

        coord_t allowed_distance = 0;
        AngleRadians transitioning_angle = 0.0;
        coord_t discretization_step_size = 0;
        double scaled_spacing_wall_0 = 0;
        double scaled_spacing_wall_X = 0;
        coord_t wall_transition_length = 0;
        double min_even_wall_line_width = 0;
        double wall_line_width_0 = 0;
        Ratio wall_split_middle_threshold = 0;
        double min_odd_wall_line_width = 0;
        double wall_line_width_x = 0;
        Ratio wall_add_middle_threshold = 0;
        int wall_distribution_count = 0;
        size_t max_bead_count = 0;
        coord_t transition_filter_dist = 0;
        coord_t allowed_filter_deviation = 0;
        bool print_thin_walls = true;
        coord_t min_feature_size = 0;
        coord_t min_bead_width = 0;
        coord_t wall_0_inset = 0;

        int wall_inset_count = 1;
        coord_t stitch_distance = 0;
        coord_t max_resolution = 0;
        coord_t max_deviation = 0;
        coord_t max_area_deviation = 0;

        AABB box;
        std::vector<Segment> segments;
        std::vector<Point> points;
        voronoi_type graph;

        std::unordered_map<Point, Point, PointHash> vertex_mapping;
        Polygons                                    polys_copy;
        bool degenerated_voronoi_diagram = false;
        double fix_angle = M_PI / 6.0;

        std::vector<DiscretizeEdge> discretize_edges;
        std::vector<DiscretizeCell> discretize_cells;
        std::vector<const vertex_type*> noplanar_vertexes;
        std::vector<const cell_type*> invalid_cells;

        graph_t HE;
	};
}
