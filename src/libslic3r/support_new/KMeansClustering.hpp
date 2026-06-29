// [FORMATTED BY CLANG-FORMAT 2026-05-09 13:35:57]
#ifndef slic3r_KMeansClustering_hpp
#define slic3r_KMeansClustering_hpp

#include "../Point.hpp"
#include <vector>
#include <cstdint>

namespace Slic3r { namespace TreeSupport3D {

// Result of K-Means clustering
struct KMeansResult
{
    // Centroid positions for each cluster
    std::vector<Point> centroids;
    // Cluster assignment for each input point (indices match input order)
    std::vector<int> labels;
};

// K-Means++ clustering for 2D points
// Returns centroids and cluster assignments
KMeansResult kmeans_2d(const std::vector<Point>& points, int k, int max_iterations = 20, double convergence_threshold = 1.0);

}} // namespace Slic3r::TreeSupport3D

#endif // slic3r_KMeansClustering_hpp
