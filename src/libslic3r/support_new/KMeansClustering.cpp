// [FORMATTED BY CLANG-FORMAT 2026-05-09 13:35:52]
#include "KMeansClustering.hpp"
#include <cmath>
#include <algorithm>
#include <random>
#include <limits>
#include <cassert>

namespace Slic3r { namespace TreeSupport3D {

static double squared_distance(const Point& a, const Point& b)
{
    double dx = double(a.x() - b.x());
    double dy = double(a.y() - b.y());
    return dx * dx + dy * dy;
}

// K-Means++ initialization: select k initial centroids with probability proportional to squared distance
static std::vector<Point> kmeans_plus_plus_init(const std::vector<Point>& points, int k)
{
    assert(!points.empty() && k > 0 && k <= static_cast<int>(points.size()));

    std::vector<Point> centroids;
    centroids.reserve(k);

    // Use fixed seed for deterministic results
    std::mt19937 rng(42);

    // Select first centroid randomly
    std::uniform_int_distribution<size_t> dist(0, points.size() - 1);
    centroids.push_back(points[dist(rng)]);

    // Select remaining k-1 centroids
    for (int c = 1; c < k; ++c) {
        // Compute squared distances to nearest centroid
        std::vector<double> distances(points.size());
        double              total_dist = 0.0;
        for (size_t i = 0; i < points.size(); ++i) {
            double min_dist = std::numeric_limits<double>::max();
            for (const auto& centroid : centroids) {
                double d = squared_distance(points[i], centroid);
                if (d < min_dist)
                    min_dist = d;
            }
            distances[i] = min_dist;
            total_dist += min_dist;
        }

        // Select next centroid with probability proportional to distance squared
        std::uniform_real_distribution<double> prob_dist(0.0, total_dist);
        double                                 target     = prob_dist(rng);
        double                                 cumulative = 0.0;
        for (size_t i = 0; i < points.size(); ++i) {
            cumulative += distances[i];
            if (cumulative >= target) {
                centroids.push_back(points[i]);
                break;
            }
        }
        // Fallback: if we didn't select due to floating point issues, pick the last point
        if (static_cast<int>(centroids.size()) == c) {
            centroids.push_back(points.back());
        }
    }

    return centroids;
}

KMeansResult kmeans_2d(const std::vector<Point>& points, int k, int max_iterations, double convergence_threshold)
{
    assert(!points.empty() && k > 0);

    KMeansResult result;
    if (k >= static_cast<int>(points.size())) {
        // Each point is its own cluster
        result.centroids = points;
        result.labels.resize(points.size());
        for (size_t i = 0; i < points.size(); ++i)
            result.labels[i] = static_cast<int>(i);
        return result;
    }

    // Initialize centroids using K-Means++
    result.centroids = kmeans_plus_plus_init(points, k);
    result.labels.resize(points.size());

    // Main loop
    for (int iter = 0; iter < max_iterations; ++iter) {
        // Assignment step: assign each point to nearest centroid
        bool changed = false;
        for (size_t i = 0; i < points.size(); ++i) {
            double min_dist     = std::numeric_limits<double>::max();
            int    best_cluster = 0;
            for (int c = 0; c < k; ++c) {
                double d = squared_distance(points[i], result.centroids[c]);
                if (d < min_dist) {
                    min_dist     = d;
                    best_cluster = c;
                }
            }
            if (result.labels[i] != best_cluster) {
                result.labels[i] = best_cluster;
                changed          = true;
            }
        }

        // Update step: compute new centroids
        std::vector<std::vector<Point>> clusters(k);
        for (size_t i = 0; i < points.size(); ++i) {
            clusters[result.labels[i]].push_back(points[i]);
        }

        bool converged = true;
        for (int c = 0; c < k; ++c) {
            if (clusters[c].empty()) {
                // Empty cluster: reinitialize to a random point
                result.centroids[c] = points[iter % points.size()];
                converged           = false;
            } else {
                // Compute centroid (mean)
                double sum_x = 0.0, sum_y = 0.0;
                for (const auto& p : clusters[c]) {
                    sum_x += p.x();
                    sum_y += p.y();
                }
                Point new_centroid(coord_t(std::round(sum_x / clusters[c].size())), coord_t(std::round(sum_y / clusters[c].size())));

                // Check convergence
                if (squared_distance(result.centroids[c], new_centroid) > convergence_threshold * convergence_threshold) {
                    converged = false;
                }
                result.centroids[c] = new_centroid;
            }
        }

        // Check for convergence
        if (converged && !changed)
            break;
    }

    return result;
}

}} // namespace Slic3r::TreeSupport3D
