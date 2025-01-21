#include "Orient.hpp"
#include "Geometry.hpp"
#include <numeric>
#include <ClipperUtils.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/log/trivial.hpp>
#include <tbb/parallel_for.h>

#if defined(_MSC_VER) && defined(__clang__)
#define BOOST_NO_CXX17_HDR_STRING_VIEW
#endif

#include <boost/multiprecision/integer.hpp>
#include <boost/rational.hpp>
// for minimun support
#include "libslic3r/AABBMesh.hpp"

#include <chrono>
//#include "omp.h"
#include <thread>

#undef MAX3
#define MAX3(a,b,c) std::max(std::max(a,b),c)

#undef MEDIAN
#define MEDIAN3(a,b,c) std::max(std::min(a,b), std::min(std::max(a,b),c))
#ifndef SQ
#define SQ(x) ((x)*(x))
#endif

#define SUPPORT_WIDTH 3
#define SUPPORT_WIDTH_MIN_VOLUME 0.28
#define SAMPLE_AREA 5

namespace Slic3r {

namespace orientation {

    struct CostItems {
        float overhang;
        float bottom;
        float bottom_hull;
        float contour;
        float area_laf;  // area_of_low_angle_faces
        float area_projected; // area of projected 2D profile
        float volume;
        float area_total;  // total area of all faces
        float radius;    // radius of bounding box
        float height_to_bottom_hull_ratio;  // affects stability, the lower the better
        float unprintability;
        CostItems(CostItems const & other) = default;
        CostItems() { memset(this, 0, sizeof(*this)); }
        static std::string field_names() {
            return "                                      overhang, bottom, bothull, contour, A_laf, A_prj, unprintability";
        }
        std::string field_values() {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1);
            ss << overhang << ",\t" << bottom << ",\t" << bottom_hull << ",\t" << contour << ",\t" << area_laf << ",\t" << area_projected << ",\t" << unprintability;
            return ss.str();
        }
    };

    bool static compare_vec3f(const Vec3f& a, const Vec3f& b)
    {
        if (a.x() != b.x())
            return a.x() > b.x();
        if (a.y() != b.y())
            return a.y() > b.y();
        return a.z() > b.z();
    }

// A class encapsulating the libnest2d Nester class and extending it with other
// management and spatial index structures for acceleration.
class AutoOrienter {
public:
    int face_count_hull;
    OrientMesh *orient_mesh = NULL;
    TriangleMesh* mesh;
    TriangleMesh mesh_convex_hull;
    Eigen::MatrixXf normals, normals_quantize, normals_hull, normals_hull_quantize;
    Eigen::VectorXf areas, areas_hull;
    Eigen::VectorXf is_apperance; // whether a facet is outer apperance
    Eigen::MatrixXf z_projected;
    Eigen::VectorXf z_max, z_max_hull;  // max of projected z
    Eigen::VectorXf z_median;  // median of projected z
    Eigen::VectorXf z_mean;  // mean of projected z

    struct meshProjectParam {
        //Eigen::MatrixXf normals;
        Eigen::MatrixXf               z_projected;
        Eigen::VectorXf               z_max, z_max_hull; // max of projected z
        Eigen::VectorXf               z_median;          // median of projected z
        Eigen::VectorXf               z_mean;            // mean of projected z
        //std::vector<Vec3f>            face_normals;
    };
    std::vector<Vec3f> face_normals;
    std::vector<Vec3f> face_normals_hull;
    OrientParams params;
    std::unique_ptr<AABBMesh>     m_emesh;

    std::vector< Vec3f> orientations;  // Vec3f == stl_normal
    std::function<void(unsigned)> progressind = {}; // default empty indicator function
    std::function<bool(void)>     stopcond    = {}; // default empty indicator function

public:
    AutoOrienter(OrientMesh* orient_mesh_,
                 const OrientParams           &params_,
                 std::function<void(unsigned)> progressind_,
                 std::function<bool(void)>     stopcond_)
    {
        orient_mesh = orient_mesh_;
        mesh = &orient_mesh->mesh;
        params = params_;
        progressind = progressind_;
        if (params.orient_type == orientation::EOrientType::MinVolume) {
            stopcond = stopcond_;
        }

        params.ASCENT = cos(PI - (orient_mesh->overhang_angle + 1) * PI / 180); // use per-object overhang angle
        if (params.orient_type != orientation::EOrientType::MinArea) {
            m_emesh = std::make_unique<AABBMesh>(*mesh, false);
        }
        // BOOST_LOG_TRIVIAL(info) << orient_mesh->name << ", angle=" << orient_mesh->overhang_angle << ", params.ASCENT=" << params.ASCENT;
        // std::cout << orient_mesh->name << ", angle=" << orient_mesh->overhang_angle << ", params.ASCENT=" << params.ASCENT;

        preprocess();
    }

    AutoOrienter(TriangleMesh* mesh_)
    {
        mesh = mesh_;
        preprocess();
    }

    struct VecHash {
        size_t operator()(const Vec3f& n1) const {
            return std::hash<coord_t>()(int(n1(0)*100+100)) + std::hash<coord_t>()(int(n1(1)*100+100)) * 101 + std::hash<coord_t>()(int(n1(2)*100+100)) * 10221;
        }
    };

    Vec3f quantize_vec3f(const Vec3f n1) {
        return Vec3f(floor(n1(0) * 1000) / 1000, floor(n1(1) * 1000) / 1000, floor(n1(2) * 1000) / 1000);
    }

    Vec3d process()
    {
        orientations = {{0, 0, -1}};                                                        // original orientation
        //orientations = {{0, 0, -1}, {0, 0, 1}, {0, -1, 0}, {0, 1, 0},  {-1, 0, 0} , {1, 0, 0}}; // original orientation

        area_cumulation_accurate(face_normals, normals_quantize, areas, 10);

        area_cumulation_accurate(face_normals_hull, normals_hull_quantize, areas_hull, 14);

        if (params.orient_type == orientation::EOrientType::MinArea)
        {
            add_supplements();
        } else if (params.orient_type == orientation::EOrientType::MinVolume) {
            orientations.push_back({0, 0, 1});
        }

        if(progressind)
            progressind(20);

        remove_duplicates();

        if (progressind)
            progressind(30);

        std::unordered_map<Vec3f, CostItems, VecHash> results;
        BOOST_LOG_TRIVIAL(info) << CostItems::field_names();
        std::cout << CostItems::field_names() << std::endl;

        if (params.orient_type == orientation::EOrientType::MinTime) {
            std::vector<std::pair<float,int>> total_height;
            total_height.resize(orientations.size());

            std::vector<std::thread> orient_threads;


            auto get_cost_time = [&](Vec3f orient, int i) {
                meshProjectParam mesh_param;
                project_vertices(-orient, mesh_param);
                float aa = getCostTime(orient,0.2f,mesh_param);          
                total_height[i] = std::make_pair(aa, i);
            };


//#pragma omp parallel 
            for (int i = 0; i < orientations.size();i++) {          
                //meshProjectParam mesh_param;
                //project_vertices(-orientations[i], mesh_param);
                //float aa = getCostTime(orientations[i],0.2f,mesh_param);
                ////float aa = getAreaWall(orientation);
                ////total_height.push_back(std::make_pair(aa,i));
                //total_height[i] = std::make_pair(aa, i);


                orient_threads.emplace_back(get_cost_time,orientations[i],i);
            }

            for (auto& t : orient_threads)
            {
                t.join();
            }

            std::sort(total_height.begin(),total_height.end(),[](const std::pair<float,int>& p1, const std::pair<float,int>& p2) {return p1.first < p2.first; });
            int ori_frist = total_height[0].second;
            Vec3f result_  = orientations[ori_frist];
            if (progressind)
                progressind(60);
            return result_.cast<double>();
        }
        else if (params.orient_type == orientation::EOrientType::MinArea) {
            for (int i = 0; i < orientations.size();i++) {
                auto orientation = -orientations[i];

                project_vertices(orientation);

                auto cost_items = get_features(orientation, false);

                float unprintability = target_function(cost_items, orientation::EOrientType::MinArea);

                results[orientation] = cost_items;

                BOOST_LOG_TRIVIAL(info) << std::fixed << std::setprecision(4) << "orientation:" << orientation.transpose() << ", cost:" << std::fixed << std::setprecision(4) << cost_items.field_values();
                std::cout << std::fixed << std::setprecision(4) << "orientation:" << orientation.transpose() << ", cost:" << std::fixed << std::setprecision(4) << cost_items.field_values() << std::endl;
            }
        }
        else {
            std::mutex resultsMutex;
            tbb::parallel_for(tbb::blocked_range<int>(0, orientations.size()), [&](const tbb::blocked_range<int>& range) {
                for (int i = range.begin(); i != range.end(); ++i) {
                //for (int i = 0; i < orientations.size(); ++i) {
                    if (stopcond && stopcond()) {
                        return;
                    }
                    auto orientation = -orientations[i];

                    meshProjectParam mesh_param;
                    project_vertices(orientation, mesh_param);

                    auto cost_items = get_features(orientation, mesh_param, params.orient_type);

                    float unprintability = target_function(cost_items, params.orient_type);
                    if (stopcond && stopcond()) {
                        return;
                    }

                    resultsMutex.lock();
                    results[orientation] = cost_items;
                    resultsMutex.unlock();

                    BOOST_LOG_TRIVIAL(info) << std::fixed << std::setprecision(4) << "orientation:" << orientation.transpose()
                                            << ", cost:" << std::fixed << std::setprecision(4) << cost_items.field_values();
                    std::cout << std::fixed << std::setprecision(4) << "orientation:" << orientation.transpose() << ", cost:" << std::fixed
                              << std::setprecision(4) << cost_items.field_values() << std::endl;
                        }
                    });
                //}
        }

        if (progressind)
            progressind(60);

        typedef std::pair<Vec3f, CostItems> PAIR;
        std::vector<PAIR> results_vector(results.begin(), results.end());
        sort(results_vector.begin(), results_vector.end(), [](const PAIR& p1, const PAIR& p2) {return p1.second.unprintability < p2.second.unprintability; });

        if (progressind)
            progressind(80);

        if (results_vector.empty()) {
            return {0, 0, 1};
        }

        //To avoid flipping, we need to verify if there are orientations with same unprintability.
        Vec3f n1 = {0, 0, 1};
        auto best_orientation = results_vector[0].first;

        for (int i = 1; i< results_vector.size()-1; i++) {
            if (abs(results_vector[i].second.unprintability - results_vector[0].second.unprintability) < EPSILON && abs(results_vector[0].first.dot(n1)-1) > EPSILON) {
                if (abs(results_vector[i].first.dot(n1)-1) < EPSILON*EPSILON) { 
                    best_orientation = n1;
                    break; 
                }
            }
            else {
                break;
            }

        }

        BOOST_LOG_TRIVIAL(info) << std::fixed << std::setprecision(6) << "best:" << best_orientation.transpose() << ", costs:" << results_vector[0].second.field_values();
        std::cout << std::fixed << std::setprecision(6) << "best:" << best_orientation.transpose() << ", costs:" << results_vector[0].second.field_values() << std::endl;

        return -best_orientation.cast<double>();
    }

    struct Edge
    {
        Vec3f v1, v2;
        Vec3f normal;
        int   faceIdx;
        Edge(const Vec3f& a, const Vec3f& b, const Vec3f& n, int idx)
            : v1(a), v2(b), normal(n), faceIdx(idx)
        {
            if (compare_vec3f(v1, v2))
                std::swap(v1, v2);
        }
        bool operator==(const Edge& other) const { return v1 == other.v1 && v2 == other.v2; }
    };

    struct EdgeHash
    {
        std::size_t operator()(const Edge& edge) const
        {
            return std::hash<float>()(edge.v1.x()) ^ std::hash<float>()(edge.v1.y()) ^ std::hash<float>()(edge.v1.z()) ^
                   std::hash<float>()(edge.v2.x()) ^ std::hash<float>()(edge.v2.y()) ^ std::hash<float>()(edge.v2.z());
        }
    };

    void export_edges_to_obj(const std::vector<std::pair<std::pair<Vec3f, Vec3f>, float>> &edges, const std::string& filename)
    {
        std::ofstream obj_file(filename);
        if (!obj_file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }

        int vertex_index = 1;
        for (const auto& edge : edges) {
            obj_file << "v " << edge.first.first.x() << " " << edge.first.first.y() << " " << edge.first.first.z() << "\n";
            obj_file << "v " << edge.first.second.x() << " " << edge.first.second.y() << " " << edge.first.second.z() << "\n";
        }

        for (size_t i = 0; i < edges.size(); ++i) {
            obj_file << "l " << vertex_index << " " << vertex_index + 1 << "\n";
            vertex_index += 2;
        }

        obj_file.close();
    }
    // Calculate the angle (cosine value) between two vectors
    double calculateAngleBetweenVectors(const Vec3f& a, const Eigen::Vector3f& b)
    {
        // Calculate the dot product
        double dotProduct = a.dot(b);
        // Calculate the magnitudes of the vectors
        double magnitudeA = a.norm();
        double magnitudeB = b.norm();
        // Calculate the angle (cosine value)
        double cosine = dotProduct / (magnitudeA * magnitudeB);
        return cosine;
    }

    // Calculate the angle (cosine value) between a line segment and a plane
    double calculateAngleBetweenLineAndPlane(const Eigen::Vector3f& lineStart,
                                             const Eigen::Vector3f& lineEnd,
                                             const Eigen::Vector3f& planeNormal)
    {
        // Calculate the direction vector of the line segment with fixed endpoint order
        Eigen::Vector3f lineDirection = lineEnd - lineStart;
        // Calculate the angle (cosine value) between the line segment direction vector and the plane normal vector
        double cosine = calculateAngleBetweenVectors(lineDirection, planeNormal);
        // Return the cosine value
        return cosine;
    }

    double calculateAngleBetweenLineAndPlane(const Vec3f& u, const Vec3f& normal)
    {
        double sinTheta = u.normalized().dot(normal.normalized());
        return asin(sinTheta) * 180.0 / M_PI;
    }

    // Calculate the projection of point C onto the line segment AB
    Vec3f calculate_projection(const Vec3f& A, const Vec3f& B, const Vec3f& C)
    {
        // Calculate the vector AB
        Vec3f AB = B - A;
        // Calculate the vector AC
        Vec3f AC = C - A;

        // Calculate the projection vector
        Vec3f projection = (AC.dot(AB) / AB.dot(AB)) * AB;
        Vec3f dir = C - projection;
        std::cout << "Projection of C onto AB: [" << projection.x() << ", " << projection.y() << ", " << projection.z() << "]" << std::endl;
        std::cout << "Projection of C onto AB dir: [" << dir.x() << ", " << dir.y() << ", " << dir.z() << "]" << std::endl;
        return dir.normalized();
    }

    // Calculate the vector perpendicular to the triangle plane and perpendicular to edge AB
    Vec3f calculate_perpendicular_vector(const Vec3f& A, const Vec3f& B, const Vec3f& C)
    {
        // Calculate the vectors AB and AC
        Vec3f AB = B - A;
        Vec3f AC = C - A;

        // Calculate the cross product to get the normal vector perpendicular to the triangle plane
        Vec3f normal = AB.cross(AC);

        // Calculate the cross product of the normal vector and AB to get the vector perpendicular to edge AB
        Vec3f perpendicular_vector = normal.cross(AB);

        // Normalize the perpendicular vector
        perpendicular_vector.normalize();
        std::cout << "Perpendicular vector: [" << perpendicular_vector.x() << ", " << perpendicular_vector.y() << ", "
                  << perpendicular_vector.z() << "]" << std::endl;
        return perpendicular_vector;
    }

    // Calculate the length of the projection of vector a onto vector b
    template<typename T>
    float projection_length(const T& a, const T& b)
    {
        // Calculate the norm of vector b
        float b_norm = b.norm();
        // Calculate the projection length of vector a onto vector b
        float proj_length = a.dot(b) / b_norm;
        return proj_length;
    }

    // Calculate the projection of point P on the plane
    Eigen::Vector3f project_point_to_plane(const Eigen::Vector3f& P, const Eigen::Vector3f& plane_point, const Eigen::Vector3f& plane_normal)
    {
        Eigen::Vector3f P_to_plane = P - plane_point;
        float distance = P_to_plane.dot(plane_normal);
        return P - distance * plane_normal;
    }

    // Calculate the length of the projection of a line segment on the plane
    float projection_line_on_plane_length(const Eigen::Vector3f& P1,
                            const Eigen::Vector3f& P2,
                            const Eigen::Vector3f& plane_point,
                            const Eigen::Vector3f& plane_normal)
    {
        Eigen::Vector3f P1_proj = project_point_to_plane(P1, plane_point, plane_normal);
        Eigen::Vector3f P2_proj = project_point_to_plane(P2, plane_point, plane_normal);
        return (P1_proj - P2_proj).norm();
    }

    // Calculate the rotation matrix
    Eigen::Matrix3f compute_rotation_matrix(const Vec3f& from, const Vec3f& to)
    {
        Vec3f axis = from.cross(to);
        float angle = acos(from.dot(to) / (from.norm() * to.norm()));
        Eigen::Matrix3f rotation_matrix = Eigen::AngleAxisf(angle, axis.normalized()).toRotationMatrix();
        return rotation_matrix;
    }

    // Rotate the vertices of the mesh
    std::vector<Vec3f> rotate_mesh(const std::vector<Vec3f>& vertices, const Eigen::Matrix3f& rotation_matrix)
    {
        std::vector<Vec3f> rotated_vertices;
        for (const auto& vertex : vertices) {
            rotated_vertices.push_back(rotation_matrix * vertex);
        }
        return rotated_vertices;
    }

    float get_overhang_line_volume(const Vec3f& orient, meshProjectParam& mesh_param)
    {
        bool minVolume = params.orient_type == orientation::EOrientType::MinVolume;
        float support_width = minVolume ? SUPPORT_WIDTH_MIN_VOLUME: SUPPORT_WIDTH;
        float total_min_z = mesh_param.z_projected.minCoeff();
        Vec3f upward = {0, 0, 1};
        Matrix3f rotation_matrix = compute_rotation_matrix(orient, upward);
        float z_min = std::numeric_limits<float>::max();
        float z_min1 = (rotation_matrix * Vec3f(0, 0, 0)).z();
        std::vector<std::pair<std::pair<Vec3f, Vec3f>, float>> overhang_edges;
        std::unordered_set<Edge, EdgeHash>                     processed_edges;
        int faceIdx = -1;
        for (const auto& face : mesh->its.indices) {
            if (stopcond && stopcond())
                return std::numeric_limits<float>::max();
            faceIdx++;
            Vec3f normal = normals.row(faceIdx);
            float angle = normal.dot(orient);
            const Vec3f& v1 = rotation_matrix * mesh->its.vertices[face[0]];
            const Vec3f& v2 = rotation_matrix * mesh->its.vertices[face[1]];
            const Vec3f& v3 = rotation_matrix * mesh->its.vertices[face[2]];
            float max_z = std::max({v1.z(), v2.z(), v3.z()});
            float min_z = std::min({v1.z(), v2.z(), v3.z()});
            z_min = std::min(z_min, min_z);
            if (angle < params.ASCENT || angle >= 0) {
                continue;
            }
            auto check_and_add_edge = [&](const Vec3f& a, const Vec3f& b, const Vec3f& c) {
                if (calculateAngleBetweenLineAndPlane(a, b, upward) > std::abs(0.5)) {
                    return;
                }
                Vec3f nor;
                if (nor = calculate_perpendicular_vector(a, b, c); projection_length(nor, upward) < EPSILON) {
                    return;
                }

                Edge edge(a, b, normal, faceIdx);
                if (processed_edges.find(edge) == processed_edges.end()) {
                    processed_edges.insert(edge);
                } else if (auto it = processed_edges.find(edge);
                           acos(calculateAngleBetweenVectors(it->normal, edge.normal)) * (180.0f / M_PI) > 30.f) {
                    Vec3f midPnt = (a + b) / 2;
                    float h_project = projection_length(midPnt, upward);
                    midPnt = rotation_matrix.transpose() * midPnt;
                    // get height
                    float height    = get_support_height(midPnt.cast<double>(), -orient.cast<double>(), {faceIdx, it->faceIdx});
                    height = height < 0 ? h_project : height + total_min_z;
                    overhang_edges.emplace_back(std::make_pair(std::make_pair(a, b), height));
                }
            };

            check_and_add_edge(v1, v2, v3);
            check_and_add_edge(v2, v3, v1);
            check_and_add_edge(v3, v1, v2);
        }
        //export_edges_to_obj(overhang_edges, "horizontal_edges.obj");
        // Traverse overhang_edges and calculate the length of each edge
        // Calculate the product of each length and the corresponding height
        float v = 0.f;
        for (const auto e : overhang_edges) {
            float length = projection_line_on_plane_length(e.first.first, e.first.second, {0, 0, 0}, upward);
            v += length * (e.second - total_min_z) * support_width;
        }
        return v;
    }

    void preprocess()
    {
        int count_apperance = 0;
        {
            int face_count = mesh->facets_count();
            auto its = mesh->its;
            face_normals = its_face_normals(its);
            areas = Eigen::VectorXf::Zero(face_count);
            is_apperance = Eigen::VectorXf::Zero(face_count);
            normals = Eigen::MatrixXf::Zero(face_count, 3);
            normals_quantize = Eigen::MatrixXf::Zero(face_count, 3);
            for (size_t i = 0; i < face_count; i++)
            {
                float area = its.facet_area(i);
                normals.row(i) = face_normals[i];
                normals_quantize.row(i) = quantize_vec3f(face_normals[i]);
                areas(i) = area;
                is_apperance(i) = (its.get_property(i).type == EnumFaceTypes::eExteriorAppearance);
                count_apperance += (is_apperance(i)==1);
            }
        }

        if (orient_mesh)
            BOOST_LOG_TRIVIAL(debug) <<orient_mesh->name<< ", count_apperance=" << count_apperance;

        // get convex hull statistics
        {
            mesh_convex_hull = mesh->convex_hull_3d();
            //mesh_convex_hull.write_binary("convex_hull_debug.stl");

            int face_count = mesh_convex_hull.facets_count();
            auto its = mesh_convex_hull.its;
            face_count_hull = mesh_convex_hull.facets_count();
            face_normals_hull = its_face_normals(its);
            areas_hull = Eigen::VectorXf::Zero(face_count);
            normals_hull = Eigen::MatrixXf::Zero(face_count_hull, 3);
            normals_hull_quantize = Eigen::MatrixXf::Zero(face_count_hull, 3);
            for (size_t i = 0; i < face_count; i++)
            {
                float area = its.facet_area(i);
                //We cannot use quantized vector here, the accumulated error will result in bad orientations.
                normals_hull.row(i) = face_normals_hull[i];
                normals_hull_quantize.row(i) = quantize_vec3f(face_normals_hull[i]);
                areas_hull(i) = area;
            }
        }
    }

    void area_cumulation(const Eigen::MatrixXf& normals_, const Eigen::VectorXf& areas_, int num_directions = 10)
    {
        std::unordered_map<stl_normal, float, VecHash> alignments;
        // init to 0
        for (size_t i = 0; i < areas_.size(); i++)
            alignments.insert(std::pair(normals_.row(i), 0));
        // cumulate areas
        for (size_t i = 0; i < areas_.size(); i++)
        {
            alignments[normals_.row(i)] += areas_(i);
        }

        typedef std::pair<stl_normal, float> PAIR;
        std::vector<PAIR> align_counts(alignments.begin(), alignments.end());
        sort(align_counts.begin(), align_counts.end(), [](const PAIR& p1, const PAIR& p2) {return p1.second > p2.second; });

        num_directions = std::min((size_t)num_directions, align_counts.size());
        for (size_t i = 0; i < num_directions; i++)
        {
            orientations.push_back(align_counts[i].first);
            //orientations.push_back(its_face_normals(mesh->its)[i]);
            BOOST_LOG_TRIVIAL(debug) << align_counts[i].first.transpose() << ", area: " << align_counts[i].second;
        }
    }
    //This function is to make sure to return the accurate normal rather than quantized normal
    void area_cumulation_accurate( std::vector<Vec3f>& normals_, const Eigen::MatrixXf& quantize_normals_, const Eigen::VectorXf& areas_, int num_directions = 10)
    {
        std::unordered_map<stl_normal, std::pair<std::vector<float>, Vec3f>, VecHash> alignments_;
        Vec3f n1 = { 0, 0, 0 };
        std::vector<float> current_areas = {0, 0};
        // init to 0
        for (size_t i = 0; i < areas_.size(); i++) {
            alignments_.insert(std::pair(quantize_normals_.row(i), std::pair(current_areas, n1)));
        }
        // cumulate areas
        for (size_t i = 0; i < areas_.size(); i++)
        {
            alignments_[quantize_normals_.row(i)].first[1] += areas_(i);
            if (areas_(i) > alignments_[quantize_normals_.row(i)].first[0]){
                alignments_[quantize_normals_.row(i)].second = normals_[i];
                alignments_[quantize_normals_.row(i)].first[0] = areas_(i);
            }
        }

        typedef std::pair<stl_normal, std::pair<std::vector<float>, Vec3f>> PAIR;
        std::vector<PAIR> align_counts(alignments_.begin(), alignments_.end());
        sort(align_counts.begin(), align_counts.end(), [](const PAIR& p1, const PAIR& p2) {return p1.second.first[1] > p2.second.first[1]; });

        num_directions = std::min((size_t)num_directions, align_counts.size());
        for (size_t i = 0; i < num_directions; i++)
        {
            orientations.push_back(align_counts[i].second.second);
            BOOST_LOG_TRIVIAL(debug) << align_counts[i].second.second.transpose() << ", area: " << align_counts[i].second.first[1];
        }
    }

    // ���ɾ��ȷֲ��ķ�������
    std::vector<Vec3f> generateUniformNormals(int numPoints)
    {
        std::vector<Vec3f> normals;
        normals.reserve(numPoints);

        const float phi = (1.0 + std::sqrt(5.0)) / 2.0; // �ƽ����
        for (int i = 0; i < numPoints; ++i) {
            float y      = 1 - (i / float(numPoints - 1)) * 2; // y�����1��-1
            float radius = std::sqrt(1 - y * y);               // �뾶
            float theta  = 2 * M_PI * i / phi;                 // �Ƕ�

            float x = std::cos(theta) * radius;
            float z = std::sin(theta) * radius;

            normals.push_back(Vec3f(x, y, z));
        }

        return normals;
    }

    void add_supplements()
    {
        std::vector<Vec3f> vecs = { {0, 0, -1} ,{0.70710678, 0, -0.70710678},{0, 0.70710678, -0.70710678},
            {-0.70710678, 0, -0.70710678},{0, -0.70710678, -0.70710678},
            {1, 0, 0},{0.70710678, 0.70710678, 0},{0, 1, 0},{-0.70710678, 0.70710678, 0},
            {-1, 0, 0},{-0.70710678, -0.70710678, 0},{0, -1, 0},{0.70710678, -0.70710678, 0},
            {0.70710678, 0, 0.70710678},{0, 0.70710678, 0.70710678},
            {-0.70710678, 0, 0.70710678},{0, -0.70710678, 0.70710678},{0, 0, 1} };

        //std::vector<Vec3f> normal = generateUniformNormals(512);
        //orientations.insert(orientations.end(), normal.begin(), normal.end());

        orientations.insert(orientations.end(), vecs.begin(), vecs.end());
    }

    /// <summary>
    /// remove duplicate orientations
    /// </summary>
    /// <param name="tol">tolerance. default 0.01 =sin(0.57\degree)</param>
    void remove_duplicates(double tol=0.0000001)
    {
        for (auto it = orientations.begin()+1; it < orientations.end(); )
        {
            bool duplicate = false;
            for (auto it_ok = orientations.begin(); it_ok < it; it_ok++)
            {
                if (it_ok->isApprox(*it, tol)) {
                    duplicate = true;
                    break;
                }
            }
            const Vec3f all_zero = { 0,0,0 };
            if (duplicate || it->isApprox(all_zero,tol))
                it = orientations.erase(it);
            else
                it++;
        }
    }

    

    float getCostTime(Vec3f orientation,float height,meshProjectParam& mesh_param_sim,bool is_support=true)
    {
         //orientation=Vec3f(0,0,-1);
         int face_count = mesh->facets_count();
         int vertex_count = mesh->its.vertices.size();

         float surf_area = 0.f;

         float fill_area = 0.f;
         float total_area=0.f;
         float overhang_area = 0.f;
         float top_area = 0.f;
         float bot_area = 0.f;
         Vec3f ori = Vec3f(0,0,-1);
         Vec3f up_ori = Vec3f(0, 0, 1);
         Matrix3f m = Eigen::Quaternionf::FromTwoVectors(orientation,ori).toRotationMatrix();
         float bot_z = std::numeric_limits<float>::max();
         float support_volumes=0.f;
         for (int vi = 0; vi < vertex_count; vi++)
         {
             Vec3f tran_v = m * mesh->its.vertices[vi];
             if (tran_v.z() < bot_z)
             {
                 bot_z = tran_v.z();
             }
         }

         float mesh_height = getHeight(orientation);
         if (mesh_height < 1e-4f)
             return 0.f;
         float layer_num = mesh_height / height;
         if (layer_num > 0.f && layer_num < 1.0f)
             layer_num = 1.0f;
         std::vector<float>  layer_area;
         layer_area.resize(layer_num, 0.f);
         std::vector<Vec4f> layer_bbx;
         layer_bbx.resize(layer_num,Vec4f(10000.f,-10000.f,10000.f,-10000.f));
         for (int i = 0; i < face_count; i++)
         {
              Vec3f new_nor = m * face_normals[i];
              double ang = new_nor.normalized().dot(ori);
         
              int v0 = mesh->its.indices[i][0];
              int v1 = mesh->its.indices[i][1];
              int v2 = mesh->its.indices[i][2];

              Vec3f center= (m * mesh->its.vertices[v0] + m * mesh->its.vertices[v1] + m * mesh->its.vertices[v2])/3.0f;
              float center_height = center.z();
             

              if ((center.z() - bot_z) < 1e-2f && (center.z() - bot_z) >= 0)
              {
                  bot_area += areas[i];
              }
             /* else {
                   if (ang >=0.999999999999f)
                   {
                        top_area += areas[i];
                   }
              }*/

              Vec3f xoy_arrow = new_nor;
              xoy_arrow[2] = 0.f;
              xoy_arrow.normalize();
              float zarc = new_nor.dot(ori);
              bool is_overhang = false;
              if (zarc >= 0.7072f && zarc <= 1.0f) 
              {
                  //overz
                  is_overhang = true;
              }

              float fill_arc = new_nor.dot(ori);
              bool is_internet_fill = false;
              if (fill_arc >= 0.54463f && fill_arc <= 1.0f)
              {
                  is_internet_fill = true;
              }

              float fill_uparc=new_nor.dot(up_ori);
              /*if (fill_uparc >=  0.309016f && fill_uparc <= 1.0f)
              {
                  is_internet_fill = true;
              }*/

              if (fill_uparc > 0.98)
              {
                  top_area += areas[i];
              }
              if (is_internet_fill)
              {
                  fill_area += (areas[i]/* * arc*/);
              }

              if (std::abs(xoy_arrow[0]) < 1e-4 && std::abs(xoy_arrow[1]) < 1e-4)
              {
                  //xoy_area.push_back(0.f);
                  continue;
              }
              float arc = new_nor.dot(xoy_arrow);
             // xoy_area.push_back(areas[i]*arc);
              if (!is_overhang)
              {
                  //total_area += (areas[i] * arc);
                  surf_area += (areas[i] * arc);
                  int layer_c = (center_height-bot_z) / height;
                  if (layer_c < 0 || layer_c >= layer_area.size())
                      layer_c -= 1;
                  layer_area[layer_c] += areas[i];
                  if (center.x() < layer_bbx[layer_c].x())
                  {
                      layer_bbx[layer_c](0, 0) = center.x();
                  }
                  if (center.x() > layer_bbx[layer_c].y())
                  {
                      layer_bbx[layer_c](0, 1) = center.x();
                  }
                  if (center.y() < layer_bbx[layer_c].z())
                  {
                      layer_bbx[layer_c](0, 2) = center.y();
                  }
                  if (center.y() > layer_bbx[layer_c].w())
                  {
                      layer_bbx[layer_c](0, 3) = center.y();
                  }
              }
              else
              {
                  surf_area+=(areas[i] * arc);
                  overhang_area += (areas[i] * arc);
                  float triang_vol = center_height * (areas[i] * arc);
                  support_volumes += triang_vol;
              }        
         }


         //all time
         for (int li = 0; li < layer_area.size(); li++)
         {
             float w = layer_bbx[li](0, 1) - layer_bbx[li](0, 0);
             float h = layer_bbx[li](0, 3) - layer_bbx[li](0, 2);
             if (layer_area[li] <100.f&&(layer_area[li]<2*(w+h)*height))
             {
                 layer_area[li] += 200.f;
             }
             total_area += layer_area[li];
         }

         float wall_length = total_area / height;
         float wall_time = wall_length / 200.f;

         float overhang_length = overhang_area / height;
         float overhang_time = overhang_length / 40.f;

         

         float top_length = top_area / 0.42;
         float top_time = top_length / 200.f; 


         float superficial_area = total_area + overhang_area;

         //volume 
         float support_area = 0.f;
         //С��ģ�� ����֧������е����� ֧�ŵ�����ԭ���ǣ�
         //�������Ƕ���أ�
         //float vol = get_volume(-orientation,support_area);

         float vol = get_support_volume(-orientation,mesh_param_sim);
         float xc_vol = get_overhang_line_volume(-orientation,mesh_param_sim);

        /* float support_length = (vol+xc_vol) / (height * 0.42f);
         float support_time = support_length / 200.f;

         float support_area_time = support_area / 200.f;*/
         //angle > 57 72  infill
         
         float fill_length = fill_area / height;
         float fill_time = fill_length / 250.f;


         //Ȩ�ر��� 1:1:?  ��Ҫ��Ŀ���Ż�  ��Ѱ����Ȩ��   ���ڹ̶�һ������
         //18-25  __   10-20    10   __   
         
         //fill 0.0002*x*x+0.8141*x+4.9651
         float fill_quan = 0.0002f * fill_time * fill_time + 0.8141f * fill_time + 4.9651f;

         //outwall 
         //-2e-05x*x+0.6938x-12.877

         float wall_quan = -2e-05f * wall_time * wall_time + 0.6938f * wall_time - 12.877f;
         //toptime
         //6e-05*x*x+0.6852*x+0.7016

         float top_quan = 6e-05f * top_time * top_time + 0.6852f * top_time + 0.7016f;

         //overhang_time
         //this is not visablity

         //support
         //5e-09*x*x+0.0049x+26.955f
         float sup_quan = 5e-09f * (vol + xc_vol) * (vol + xc_vol) + 0.0049f * (vol + xc_vol) + 26.955f;
         // 
         //这里可以加入计算
         float layer_quan = 0.0f;
         if (wall_quan + fill_quan + top_quan < layer_num * 1.0f)
         {
             layer_quan = (layer_num - (wall_quan + fill_quan + top_quan))/layer_num;
         }
         if(is_support)
            return wall_quan /*+ overhang_time*/ + fill_quan+ top_quan+layer_num * layer_quan+sup_quan;
         return 20.f*wall_time + overhang_time + top_time + layer_num * 15.f;
    }


    float getAreaWall(Vec3f orientation)
    {
        float area = 0.f;
        Vec3f ori=Vec3f(0,0,-1);
        Matrix3f m=Eigen::Quaternionf::FromTwoVectors(orientation,ori).toRotationMatrix();
        for (int i = 0; i < face_normals.size(); i++)
        {
            Vec3f new_nor = m * face_normals[i];
            float arc = new_nor.dot(Vec3f(0,0,-1));
           // float tr = std::abs(arc + 1);
            if(arc+1>(1e-4)&&arc-1<-(1e-4))
                area += areas[i];
        }
        return area;
    }


    float getHeight(Vec3f orientation)
    {
        float h=0;
        int vertex_count = mesh->its.vertices.size();
        auto its = mesh->its;
        Vec3f ori=Vec3f(0,0,-1);
        //Matrix3d m = Eigen::Quaterniond().setFromTwoVectors(ori, orientation).toRotationMatrix();
        Matrix3f m=Eigen::Quaternionf::FromTwoVectors(orientation,ori).toRotationMatrix();
        float    min_z(std::numeric_limits<float>::max());
        float    max_z(-std::numeric_limits<float>::max());
        for (size_t i = 0; i < vertex_count; i++)
        {
            Vec3f v=its.vertices[i];
            Vec3f n_v=m*v;
            if (n_v.z()<min_z)
                min_z=n_v.z();
            if (n_v.z()>max_z)
                max_z=n_v.z();
        }
        h = max_z-min_z;
        return h;
    }

    void project_vertices(Vec3f orientation)
    {
        int face_count = mesh->facets_count();
        auto its = mesh->its;
        z_projected.resize(face_count, 3);
        z_max.resize(face_count, 1);
        z_median.resize(face_count, 1);
        z_mean.resize(face_count, 1);
        for (size_t i = 0; i < face_count; i++)
        {
            float z0 = its.get_vertex(i,0).dot(orientation);
            float z1 = its.get_vertex(i,1).dot(orientation);
            float z2 = its.get_vertex(i,2).dot(orientation);
            z_projected(i, 0) = z0;
            z_projected(i, 1) = z1;
            z_projected(i, 2) = z2;
            z_max(i) = MAX3(z0,z1,z2);
            z_median(i) = MEDIAN3(z0,z1,z2);
            z_mean(i) = (z0 + z1 + z2) / 3;
        }

        z_max_hull.resize(mesh_convex_hull.facets_count(), 1);
        its = mesh_convex_hull.its;
        for (size_t i = 0; i < z_max_hull.rows(); i++)
        {
            float z0 = its.get_vertex(i,0).dot(orientation);
            float z1 = its.get_vertex(i,1).dot(orientation);
            float z2 = its.get_vertex(i,2).dot(orientation);
            z_max_hull(i) = MAX3(z0, z1, z2);
        }
    }

    void project_vertices(Vec3f orientation, meshProjectParam& mesh_param)
    {
        int face_count = mesh->facets_count();
        auto its = mesh->its;
        mesh_param.z_projected.resize(face_count, 3);
        mesh_param.z_max.resize(face_count, 1);
        mesh_param.z_median.resize(face_count, 1);
        mesh_param.z_mean.resize(face_count, 1);
        for (size_t i = 0; i < face_count; i++)
        {
            float z0 = its.get_vertex(i,0).dot(orientation);
            float z1 = its.get_vertex(i,1).dot(orientation);
            float z2 = its.get_vertex(i,2).dot(orientation);
            mesh_param.z_projected(i, 0) = z0;
            mesh_param.z_projected(i, 1) = z1;
            mesh_param.z_projected(i, 2) = z2;
            mesh_param.z_max(i) = MAX3(z0,z1,z2);
            mesh_param.z_median(i) = MEDIAN3(z0,z1,z2);
            mesh_param.z_mean(i) = (z0 + z1 + z2) / 3;
        }

        mesh_param.z_max_hull.resize(mesh_convex_hull.facets_count(), 1);
        its = mesh_convex_hull.its;
        for (size_t i = 0; i < mesh_param.z_max_hull.rows(); i++)
        {
            float z0 = its.get_vertex(i,0).dot(orientation);
            float z1 = its.get_vertex(i,1).dot(orientation);
            float z2 = its.get_vertex(i,2).dot(orientation);
            mesh_param.z_max_hull(i) = MAX3(z0, z1, z2);
        }
    }

    static Eigen::VectorXi argsort(const Eigen::VectorXf& vec, std::string order="ascend")
    {
        Eigen::VectorXi ind = Eigen::VectorXi::LinSpaced(vec.size(), 0, vec.size() - 1);//[0 1 2 3 ... N-1]
        std::function<bool(int, int)> rule;
        if (order == "ascend") {
            rule = [vec](int i, int j)->bool {
                return vec(i) < vec(j);
                };
            }
        else {
            rule = [vec](int i, int j)->bool {
                return vec(i) > vec(j);
                };
            }
        std::sort(ind.data(), ind.data() + ind.size(), rule);
        return ind;
    }
    float get_support_height(const Vec3d& startPos, const Vec3d& orient, std::vector<int> idxOverhangs)
    {
        // ray hit startPos
        Vec3d m_startPos = startPos - orient.normalized() * EPSILON;
        // cover to double
        std::vector<AABBMesh::hit_result> hits = m_emesh->query_ray_hits(m_startPos, orient, idxOverhangs);
        if (hits.empty()) {
            return -1.0;
        }
        // inner error face
        if (hits.size() % 2 == 1) {
            return 0;
        }
        for (const auto& hit : hits) {
            bool isUpFace = projection_length(orient, hit.direction()) > 0;
            // inner error face
            if (!isUpFace)
                return 0;

            Vec3d hitPos   = hit.position();
            Vec3d diff     = hitPos - startPos;
            float distance = diff.norm();
            return distance;
        }
        return -1.0;
    }

    // Calculate the centroid of a triangle
    Eigen::Vector3d calculate_centroid(const Eigen::Vector3d& A, const Eigen::Vector3d& B, const Eigen::Vector3d& C)
    {
        return (A + B + C) / 3.0;
    }

    // Recursively subdivide the triangle and get the centroid of each part
    void subdivide_triangle(
        const Eigen::Vector3d& A, const Eigen::Vector3d& B, const Eigen::Vector3d& C, int depth, Eigen::MatrixXd& centroids, int& index)
    {
        if (depth == 0) {
            centroids.col(index++) = calculate_centroid(A, B, C);
            return;
        }

        // Calculate the midpoints of the edges
        Eigen::Vector3d AB_mid = (A + B) / 2.0;
        Eigen::Vector3d BC_mid = (B + C) / 2.0;
        Eigen::Vector3d CA_mid = (C + A) / 2.0;

        // Recursively subdivide each smaller triangle
        subdivide_triangle(A, AB_mid, CA_mid, depth - 1, centroids, index);
        subdivide_triangle(AB_mid, B, BC_mid, depth - 1, centroids, index);
        subdivide_triangle(CA_mid, BC_mid, C, depth - 1, centroids, index);
        subdivide_triangle(AB_mid, BC_mid, CA_mid, depth - 1, centroids, index);
    }

    // Calculate the total number of centroids for a given depth of subdivision
    int calculate_total_centroids(int depth) { return std::pow(4, depth); }

    float get_support_volume(Vec3f orientation, meshProjectParam& mesh_param)
    {
        const Eigen::MatrixXf& z_projected = mesh_param.z_projected;
        const Eigen::VectorXf& z_max       = mesh_param.z_max;
        //const Eigen::VectorXf &z_max_hull = mesh_param.z_max_hull; // max of projected z
        const Eigen::VectorXf& z_median    = mesh_param.z_median;   // median of projected z
        Eigen::VectorXf&        z_mean   = mesh_param.z_mean;     // mean of projected z

        float total_min_z = mesh_param.z_projected.minCoeff();
        float total_max_z = mesh_param.z_projected.maxCoeff();
        // filter bottom area
        auto bottom_condition      = z_max.array() < total_min_z + this->params.FIRST_LAY_H - EPSILON;
        //auto bottom_condition_hull = z_max_hull.array() < total_min_z + this->params.FIRST_LAY_H - EPSILON;
        auto bottom_condition_2nd  = z_max.array() < total_min_z + this->params.FIRST_LAY_H / 2.f - EPSILON;

        // filter overhang
        Eigen::VectorXf normal_projection(normals.rows(), 1); // = this->normals.dot(orientation);
        for (size_t i = 0; i < normals.rows(); i++) {
            normal_projection(i) = normals.row(i).dot(orientation);
        }
        auto areas_appearance = areas.cwiseProduct(
            (is_apperance * params.APPERANCE_FACE_SUPP + Eigen::VectorXf::Ones(is_apperance.rows(), is_apperance.cols())));

        auto overhang_condition = ((normal_projection.array() < params.ASCENT) * (!bottom_condition_2nd));

        auto overhang_areas = overhang_condition.select(areas_appearance, 0);

        auto its = mesh->its;
        Vec3d orient = -orientation.cast<double>().normalized();
        for (int i = 0; i < overhang_condition.size(); i++) {
            if (stopcond && stopcond())
                return std::numeric_limits<float>::max();
            if (overhang_condition(i)) {
                int  idxOverhang = i;
                Vec3d v1 = its.get_vertex(i, 0).cast<double>();
                Vec3d v2 = its.get_vertex(i, 1).cast<double>();
                Vec3d v3 = its.get_vertex(i, 2).cast<double>();
                float area = areas_appearance(i);
                int depth =  area > SAMPLE_AREA ? 2: 0;
                int total_centroids = calculate_total_centroids(depth);

                // Initialize the matrix to store the centroids
                Eigen::MatrixXd centroids(3, total_centroids);
                int index = 0;

                // Recursively subdivide the triangle and get the centroid of each part
                subdivide_triangle(v1, v2, v3, depth, centroids, index);

                centroids = centroids.colwise() - orient * EPSILON;
                // traverse points
                float rst =0.;
                for (int j = 0; j < centroids.cols(); ++j) {
                    Vec3d startPos = centroids.col(j).transpose();
                    float h = get_support_height(startPos, orient, {idxOverhang});
                    rst += h < 0 ? z_mean(idxOverhang) : h + total_min_z;
                }
                rst /= total_centroids;
                z_mean(idxOverhang) = rst;
            }
        }


        Eigen::MatrixXf heights = z_mean.array() - total_min_z;
        if (params.orient_type == orientation::EOrientType::MinVolume) {
            Eigen::MatrixXf inner = normal_projection.array() - params.ASCENT;
            inner                 = inner.cwiseMin(0).cwiseAbs();
            return (heights.array() * overhang_areas.array() * inner.array()).sum();
        }
        float volume = (heights.array() * overhang_areas.array() * normal_projection.array()).sum();
        return std::abs(volume);
    }

    CostItems get_features(Vec3f orientation, bool min_volume = true)
    {
        CostItems costs;
        costs.area_total = mesh->bounding_box().area();
        costs.radius = mesh->bounding_box().radius();
        // volume
        costs.volume = mesh->stats().volume > 0 ? mesh->stats().volume : its_volume(mesh->its);

        float total_min_z = z_projected.minCoeff();
        // filter bottom area
        auto bottom_condition = z_max.array() < total_min_z + this->params.FIRST_LAY_H - EPSILON;
        auto bottom_condition_hull = z_max_hull.array() < total_min_z + this->params.FIRST_LAY_H - EPSILON;
        auto bottom_condition_2nd = z_max.array() < total_min_z + this->params.FIRST_LAY_H/2.f - EPSILON;
        //The first layer is sliced on half of the first layer height. 
        //The bottom area is measured by accumulating first layer area with the facets area below first layer height.
        //By combining these two factors, we can avoid the wrong orientation of large planar faces while not influence the
        //orientations of complex objects with small bottom areas.
        costs.bottom = bottom_condition.select(areas, 0).sum()*0.5 + bottom_condition_2nd.select(areas, 0).sum();

        // filter overhang
        Eigen::VectorXf normal_projection(normals.rows(), 1);// = this->normals.dot(orientation);
        for (size_t i = 0; i < normals.rows(); i++)
        {
            normal_projection(i) = normals.row(i).dot(orientation);
        }
        auto areas_appearance = areas.cwiseProduct((is_apperance * params.APPERANCE_FACE_SUPP + Eigen::VectorXf::Ones(is_apperance.rows(), is_apperance.cols())));
        auto overhang_areas = ((normal_projection.array() < params.ASCENT) * (!bottom_condition_2nd)).select(areas_appearance, 0);
        Eigen::MatrixXf inner = normal_projection.array() - params.ASCENT;
        inner = inner.cwiseMin(0).cwiseAbs();
        if (min_volume)
        {
            Eigen::MatrixXf heights = z_mean.array() - total_min_z;
            costs.overhang = (heights.array()* overhang_areas.array()*inner.array()).sum();
        }
        else {
            costs.overhang = overhang_areas.array().cwiseAbs().sum();
        }

        {
            // contour perimeter
#if 1
            // the simple way for contour is even better for faces of small bridges
            costs.contour = 4 * sqrt(costs.bottom);
#else
            float contour = 0;
            int face_count = mesh->facets_count();
            auto its = mesh->its;
            int contour_amout = 0;
            for (size_t i = 0; i < face_count; i++)
            {
                if (bottom_condition(i)) {
                    Eigen::VectorXi index = argsort(z_projected.row(i));
                    stl_vertex line = its.get_vertex(i, index(0)) - its.get_vertex(i, index(1));
                    contour += line.norm();
                    contour_amout++;
                }
            }
            costs.contour += contour + params.CONTOUR_AMOUNT * contour_amout;
#endif
        }

        // bottom of convex hull
        costs.bottom_hull = (bottom_condition_hull).select(areas_hull, 0).sum();

        // low angle faces
        auto normal_projection_abs = normal_projection.cwiseAbs();
        Eigen::MatrixXf laf_areas = ((normal_projection_abs.array() < params.LAF_MAX) * (normal_projection_abs.array() > params.LAF_MIN) * (z_max.array() > total_min_z + params.FIRST_LAY_H)).select(areas, 0);
        costs.area_laf = laf_areas.sum();

        // height to bottom_hull_area ratio
        //float total_max_z = z_projected.maxCoeff();
        //costs.height_to_bottom_hull_ratio = SQ(total_max_z) / (costs.bottom_hull + 1e-7);

        return costs;
    }

    // previously calc_overhang
    CostItems get_features(Vec3f                     orientation,
                           meshProjectParam&         mesh_param,
                           orientation::EOrientType orient_type)
    {
        CostItems costs;
        costs.area_total = mesh->bounding_box().area();
        costs.radius = mesh->bounding_box().radius();
        // volume
        costs.volume = mesh->stats().volume > 0 ? mesh->stats().volume : its_volume(mesh->its);

        float total_min_z = mesh_param.z_projected.minCoeff();
        float total_max_z = mesh_param.z_projected.maxCoeff();
        // filter bottom area
        auto bottom_condition = mesh_param.z_max.array() < total_min_z + this->params.FIRST_LAY_H - EPSILON;
        auto bottom_condition_hull = mesh_param.z_max_hull.array() < total_min_z + this->params.FIRST_LAY_H - EPSILON;
        auto bottom_condition_2nd  = mesh_param.z_max.array() < total_min_z + this->params.FIRST_LAY_H / 2.f - EPSILON;
        //The first layer is sliced on half of the first layer height. 
        //The bottom area is measured by accumulating first layer area with the facets area below first layer height.
        //By combining these two factors, we can avoid the wrong orientation of large planar faces while not influence the
        //orientations of complex objects with small bottom areas.
        costs.bottom = bottom_condition.select(areas, 0).sum()*0.5 + bottom_condition_2nd.select(areas, 0).sum();

        // filter overhang
        Eigen::VectorXf normal_projection(normals.rows(), 1);// = this->normals.dot(orientation);
        for (size_t i = 0; i < normals.rows(); i++)
        {
            normal_projection(i) = normals.row(i).dot(orientation);
        }
        auto areas_appearance = areas.cwiseProduct((is_apperance * params.APPERANCE_FACE_SUPP + Eigen::VectorXf::Ones(is_apperance.rows(), is_apperance.cols())));

        auto overhang_condition = ((normal_projection.array() < params.ASCENT) * (!bottom_condition_2nd));

        //auto overhang_areas = ((normal_projection.array() < params.ASCENT) * (!bottom_condition_2nd)).select(areas_appearance, 0);
        auto overhang_areas = overhang_condition.select(areas_appearance, 0);

        //auto top_areas = ((normal_projection.array() > -params.ASCENT) * (!top_condition)).select(areas_appearance, 0);

        auto overhang_faces = ((normal_projection.array() < params.ASCENT) * (!bottom_condition_2nd)).select(mesh_param.z_projected, 0);
        auto overhang_mid   = ((normal_projection.array() < params.ASCENT) * (!bottom_condition_2nd)).select(mesh_param.z_median, 0);

        Eigen::MatrixXf inner = normal_projection.array() - params.ASCENT;
        inner = inner.cwiseMin(0).cwiseAbs();
        if (orient_type == orientation::EOrientType::MinVolume)
        {
            Eigen::MatrixXf heights = mesh_param.z_mean.array() - total_min_z;
            float line_volume = get_overhang_line_volume(orientation, mesh_param);
            float area_volume = get_support_volume(orientation, mesh_param);
            costs.overhang = line_volume + area_volume;
        }
        else if(orient_type == orientation::EOrientType::MinArea) {
            costs.overhang = overhang_areas.array().cwiseAbs().sum();
        }

        {
            // contour perimeter
#if 1
            // the simple way for contour is even better for faces of small bridges
            costs.contour = 4 * sqrt(costs.bottom);
#else
            float contour = 0;
            int face_count = mesh->facets_count();
            auto its = mesh->its;
            int contour_amout = 0;
            for (size_t i = 0; i < face_count; i++)
            {
                if (bottom_condition(i)) {
                    Eigen::VectorXi index = argsort(z_projected.row(i));
                    stl_vertex line = its.get_vertex(i, index(0)) - its.get_vertex(i, index(1));
                    contour += line.norm();
                    contour_amout++;
                }
            }
            costs.contour += contour + params.CONTOUR_AMOUNT * contour_amout;
#endif
        }

        // bottom of convex hull
        costs.bottom_hull = (bottom_condition_hull).select(areas_hull, 0).sum();

        // low angle faces
        auto normal_projection_abs = normal_projection.cwiseAbs();
        Eigen::MatrixXf laf_areas = ((normal_projection_abs.array() < params.LAF_MAX) * (normal_projection_abs.array() > params.LAF_MIN) * (mesh_param.z_max.array() > total_min_z + params.FIRST_LAY_H)).select(areas, 0);
        costs.area_laf = laf_areas.sum();

        // height to bottom_hull_area ratio
        //float total_max_z = z_projected.maxCoeff();
        //costs.height_to_bottom_hull_ratio = SQ(total_max_z) / (costs.bottom_hull + 1e-7);

        return costs;
    }

    float get_volume(Vec3f orientation,float& support_areas)
    {
        float volume = 0.f;
        //float total_min_z = z_projected.minCoeff();

        //Eigen::VectorXf normal_projection(normals.rows(), 1);// = this->normals.dot(orientation);
        //for (size_t i = 0; i < normals.rows(); i++)
        //{
        //    normal_projection(i) = normals.row(i).dot(orientation);
        //}
        //auto bottom_condition_2nd = z_max.array() < total_min_z + this->params.FIRST_LAY_H/2.f - EPSILON;
        //auto areas_appearance = areas.cwiseProduct((is_apperance * params.APPERANCE_FACE_SUPP + Eigen::VectorXf::Ones(is_apperance.rows(), is_apperance.cols())));
        //auto overhang_areas = ((normal_projection.array() < params.ASCENT) * (!bottom_condition_2nd)).select(areas_appearance, 0);
        //Eigen::MatrixXf inner = normal_projection.array() - params.ASCENT;
        //inner = inner.cwiseMin(0).cwiseAbs();
        //support_areas = overhang_areas.sum();

        //Eigen::MatrixXf heights = z_mean.array() - total_min_z;
        //volume = (heights.array()* overhang_areas.array()*inner.array()).sum();
        return volume;
    }



    float target_function(CostItems& costs, orientation::EOrientType orient_type)
    {
        float cost=0;
        float bottom = costs.bottom;//std::min(costs.bottom, params.BOTTOM_MAX);
        float bottom_hull = costs.bottom_hull;// std::min(costs.bottom_hull, params.BOTTOM_HULL_MAX);
        if (orient_type == orientation::EOrientType::MinVolume)
        {
            //float overhang = costs.overhang / 25;
            //cost = params.TAR_A * (overhang + params.TAR_B) + params.RELATIVE_F * (/*costs.volume/100*/overhang*params.TAR_C + params.TAR_D + params.TAR_LAF * costs.area_laf * params.use_low_angle_face) / (params.TAR_D + params.CONTOUR_F * costs.contour + params.BOTTOM_F * bottom + params.BOTTOM_HULL_F * bottom_hull + params.TAR_E * overhang + params.TAR_PROJ_AREA * costs.area_projected);
            cost = costs.overhang;
        }
        else if(orient_type == orientation::EOrientType::MinArea)
        {
            float overhang = costs.overhang;
            cost = params.RELATIVE_F * (costs.overhang * params.TAR_C + params.TAR_D + params.TAR_LAF * costs.area_laf * params.use_low_angle_face) / (params.TAR_D + params.CONTOUR_F * costs.contour + params.BOTTOM_F * bottom + params.BOTTOM_HULL_F * bottom_hull + params.TAR_PROJ_AREA * costs.area_projected);
        }
        cost += (costs.bottom < params.BOTTOM_MIN) * 100;// +(costs.height_to_bottom_hull_ratio > params.height_to_bottom_hull_ratio_MIN) * 110;

        costs.unprintability = costs.unprintability = cost;

        return cost;
    }
};

void _orient(OrientMeshs& meshs_,
        const OrientParams           &params,
        std::function<void(unsigned, std::string)> progressfn,
        std::function<bool()>         stopfn)
{
    if (!params.parallel)
    {
        for (size_t i = 0; i != meshs_.size(); ++i) {
            //auto& mesh_ = meshs_[i];
            //progressfn(i, mesh_.name);
            ////auto progressfn_i = [&](unsigned cnt) {progressfn(cnt, "Orienting " + mesh_.name); };
            //AutoOrienter orienter(&mesh_, params, /*progressfn_i*/{}, stopfn);
            //mesh_.orientation = orienter.process();
            //Geometry::rotation_from_two_vectors(mesh_.orientation, { 0,0,1 }, mesh_.axis, mesh_.angle, &mesh_.rotation_matrix);
            //BOOST_LOG_TRIVIAL(info) << std::fixed << std::setprecision(3) << "v,phi: " << mesh_.axis.transpose() << ", " << mesh_.angle;
            //flush_logs();
             auto& mesh_ = meshs_[i];
            progressfn(i, mesh_.name);
            AutoOrienter orienter(&mesh_, params, {}, stopfn);
            mesh_.orientation = orienter.process();
            Geometry::rotation_from_two_vectors(mesh_.orientation, { 0,0,-1 }, mesh_.axis, mesh_.angle, &mesh_.rotation_matrix);
            mesh_.euler_angles = Geometry::extract_euler_angles(mesh_.rotation_matrix);
        }
    }
    else {
        tbb::parallel_for(tbb::blocked_range<size_t>(0, meshs_.size()), [&meshs_, &params, progressfn, stopfn](const tbb::blocked_range<size_t>& range) {
            for (size_t i = range.begin(); i != range.end(); ++i) {
                auto& mesh_ = meshs_[i];
                progressfn(i, mesh_.name);
                AutoOrienter orienter(&mesh_, params, {}, stopfn);
                mesh_.orientation = orienter.process();
                Geometry::rotation_from_two_vectors(mesh_.orientation, { 0,0,-1 }, mesh_.axis, mesh_.angle, &mesh_.rotation_matrix);
                mesh_.euler_angles = Geometry::extract_euler_angles(mesh_.rotation_matrix);
                BOOST_LOG_TRIVIAL(debug) << "rotation_from_two_vectors: " << mesh_.orientation << "; " << mesh_.axis << "; " << mesh_.angle << "; euler: " << mesh_.euler_angles.transpose();
            }});
    }
}

void orient(OrientMeshs &      arrangables,
             const OrientMeshs &excludes,
             const OrientParams &  params)
{

    auto &cfn = params.stopcondition;
    auto &pri = params.progressind;

    _orient(arrangables, params, pri, cfn);

}

void orient(ModelObject* obj)
{
    auto m = obj->mesh();
    AutoOrienter orienter(&m);
    Vec3d orientation = orienter.process();
    Vec3d axis;
    double angle;
    Geometry::rotation_from_two_vectors(orientation, { 0,0,1 }, axis, angle);

    obj->rotate(angle, axis);
    obj->ensure_on_bed();
}

void orient(ModelInstance* instance)
{
    auto m = instance->get_object()->mesh();
    AutoOrienter orienter(&m);
    Vec3d orientation = orienter.process();
    Vec3d axis;
    double angle;
    Matrix3d rotation_matrix;
    Geometry::rotation_from_two_vectors(orientation, { 0,0,1 }, axis, angle, &rotation_matrix);
    instance->rotate(rotation_matrix);
}


} // namespace arr
} // namespace Slic3r
