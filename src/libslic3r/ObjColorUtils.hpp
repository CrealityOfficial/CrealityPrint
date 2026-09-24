#pragma once
#include <iostream>
#include <ctime>

#include "opencv2/opencv.hpp"

class QuantKMeans
{
public:
    int     m_alpha_thres;
    cv::Mat m_flatten_labels;
    cv::Mat m_centers8UC3;
    QuantKMeans(int alpha_thres = 10) : m_alpha_thres(alpha_thres) {}
    void apply(cv::Mat &ori_image, cv::Mat &new_image, int num_cluster, int color_space)
    {
        cv::Mat image;
        convert_color_space(ori_image, image, color_space);
        cv::Mat flatten_image = flatten(image);

        apply(flatten_image, num_cluster, color_space);
        replace_centers(ori_image, new_image);
    }
    void apply_aplha(cv::Mat &ori_image, cv::Mat &new_image, int num_cluster, int color_space)
    {
        // cout << " *** DoAlpha *** " << endl;
        cv::Mat flatten_image8UC3 = flatten_alpha(ori_image);
        cv::Mat image8UC3;
        convert_color_space(flatten_image8UC3, image8UC3, color_space);
        cv::Mat image32FC3(image8UC3.rows, 1, CV_32FC3);
        for (int i = 0; i < image8UC3.rows; i++)
            image32FC3.at<cv::Vec3f>(i, 0) = image8UC3.at<cv::Vec3b>(i, 0);

        apply(image32FC3, num_cluster, color_space);
        repalce_centers_aplha(ori_image, new_image);
    }
    void apply(cv::Mat &flatten_image, int num_cluster, int color_space)
    {
        cv::Mat centers32FC3;
        num_cluster = fmin(flatten_image.rows, num_cluster);
        kmeans(flatten_image, num_cluster, this->m_flatten_labels, cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 300, 0.5), 3, cv::KMEANS_PP_CENTERS,
               centers32FC3);
        this->m_centers8UC3 = cv::Mat(num_cluster, 1, CV_8UC3);
        for (int i = 0; i < num_cluster; i++) this->m_centers8UC3.at<cv::Vec3b>(i) = centers32FC3.at<cv::Vec3f>(i);

        convert_color_space(this->m_centers8UC3, this->m_centers8UC3, color_space, true);
    }
    void apply(const std::vector<std::array<float, 4>> &ori_colors,
               std::vector<std::array<float, 4>> &      cluster_results,
               std::vector<int> &                       labels,
               int                                      num_cluster = -1,
               int                                      max_cluster = 15,
               int                                      color_space = 2)
    {
        // 0~255
        cv::Mat flatten_image8UC3 = flatten_vector(ori_colors);

        this->apply(flatten_image8UC3, cluster_results, labels, num_cluster, max_cluster, color_space);
    }
    void apply(const cv::Mat &                    flatten_image8UC3,
               std::vector<std::array<float, 4>> &cluster_results,
               std::vector<int> &                 labels,
               int                                num_cluster = -1,
               int                                max_cluster = 15,
               int                                color_space = 2)
    {
        cv::Mat image8UC3;
        convert_color_space(flatten_image8UC3, image8UC3, color_space);

        cv::Mat image32FC3(image8UC3.rows, 1, CV_32FC3);
        for (int i = 0; i < image8UC3.rows; i++)
            image32FC3.at<cv::Vec3f>(i, 0) = image8UC3.at<cv::Vec3b>(i, 0);

        int  real_num_cluster = num_cluster < 1 ? max_cluster : std::min(num_cluster, max_cluster);
        bool convert_centers_to_rgb = true;
        cv::Mat centers32FC3;
        if (!this->more_than_request(flatten_image8UC3, real_num_cluster)) {
            // Preserve the original colors exactly when they already fit within the limit.
            real_num_cluster     = compute_num_colors(flatten_image8UC3);
            convert_centers_to_rgb = false;
            apply_color_cluster_image(flatten_image8UC3, centers32FC3);
        } else if (!this->more_than_request(image8UC3, real_num_cluster)) {
            // Color-space conversion may merge colors which were distinct in RGB.
            real_num_cluster = compute_num_colors(image8UC3);
            apply_color_cluster_image(image8UC3, centers32FC3);
        } else {
            // The old automatic mode ran K-means for every K from 1 to the limit even
            // though its raw compactness score normally selects the largest K. Run the
            // intended maximum K once instead.
            apply_color_cluster_kmeans(image32FC3, real_num_cluster, centers32FC3);
        }

        this->m_centers8UC3 = cv::Mat(real_num_cluster, 1, CV_8UC3);
        for (int i = 0; i < real_num_cluster; i++) {
            auto center                          = centers32FC3.row(i);
            this->m_centers8UC3.at<cv::Vec3b>(i) = {uchar(center.at<float>(0)), uchar(center.at<float>(1)), uchar(center.at<float>(2))};
        }
        if (convert_centers_to_rgb)
            convert_color_space(this->m_centers8UC3, this->m_centers8UC3, color_space, true);
        real_num_cluster = merge_duplicate_centers_and_relabel();

        cluster_results.clear();
        labels.clear();
        labels.reserve(flatten_image8UC3.rows);
        for (int i = 0; i < flatten_image8UC3.rows; i++)
            labels.emplace_back(this->m_flatten_labels.at<int>(i, 0));
        cluster_results.reserve(real_num_cluster);
        for (int i = 0; i < real_num_cluster; i++) {
            cv::Vec3f center = this->m_centers8UC3.at<cv::Vec3b>(i, 0);
            cluster_results.emplace_back(std::array<float, 4>{center[0] / 255.f, center[1] / 255.f, center[2] / 255.f, 1.f});
        }
    }

private:
    void apply_color_cluster_kmeans(cv::Mat &image32FC3, int num_cluster, cv::Mat &centers32FC3)
    {
        const uint64 old_state = cv::theRNG().state;
        cv::theRNG().state = 12345;
        cv::kmeans(image32FC3, num_cluster, this->m_flatten_labels,
                   cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 300, 0.5), 3,
                   cv::KMEANS_PP_CENTERS, centers32FC3);
        cv::theRNG().state = old_state;
    }

    void apply_color_cluster_image(const cv::Mat &image8UC3, cv::Mat &centers32FC3)
    {
        std::vector<cv::Vec3b> unique_colors;
        this->m_flatten_labels = cv::Mat(image8UC3.rows, 1, CV_32S);
        for (int i = 0; i < image8UC3.rows; i++) {
            const cv::Vec3b color = image8UC3.at<cv::Vec3b>(i, 0);
            int             index = find_color_index(color, unique_colors);
            if (index == -1) {
                index = static_cast<int>(unique_colors.size());
                unique_colors.emplace_back(color);
            }
            this->m_flatten_labels.at<int>(i, 0) = index;
        }

        cv::Mat centers8UC3(static_cast<int>(unique_colors.size()), 1, CV_8UC3, unique_colors.data());
        centers8UC3.convertTo(centers32FC3, CV_32FC3);
    }

    static int find_color_index(const cv::Vec3b &cur_color, const std::vector<cv::Vec3b> &uniqueImage)
    {
        for (int i = 0; i < static_cast<int>(uniqueImage.size()); i++)
            if (cur_color[0] == uniqueImage[i][0] && cur_color[1] == uniqueImage[i][1] && cur_color[2] == uniqueImage[i][2]) return i;
        return -1;
    }

    int merge_duplicate_centers_and_relabel()
    {
        std::vector<cv::Vec3b> unique_centers;
        std::vector<int>       center_remap(m_centers8UC3.rows);
        unique_centers.reserve(m_centers8UC3.rows);

        for (int i = 0; i < m_centers8UC3.rows; i++) {
            const cv::Vec3b center = m_centers8UC3.at<cv::Vec3b>(i, 0);
            int             index  = find_color_index(center, unique_centers);
            if (index == -1) {
                index = static_cast<int>(unique_centers.size());
                unique_centers.emplace_back(center);
            }
            center_remap[i] = index;
        }

        for (int i = 0; i < m_flatten_labels.rows; i++) {
            int &label = m_flatten_labels.at<int>(i, 0);
            label      = center_remap[label];
        }

        cv::Mat merged_centers(static_cast<int>(unique_centers.size()), 1, CV_8UC3);
        for (int i = 0; i < static_cast<int>(unique_centers.size()); i++)
            merged_centers.at<cv::Vec3b>(i, 0) = unique_centers[i];
        m_centers8UC3 = merged_centers;

        return static_cast<int>(unique_centers.size());
    }

public:
    bool more_than_request(const cv::Mat &image8UC3, int target_num)
    {
        std::vector<cv::Vec3b> uniqueImage;
        cv::Vec3b              cur_color;
        for (int i = 0; i < image8UC3.rows; i++) {
            cur_color = image8UC3.at<cv::Vec3b>(i, 0);
            if (!is_in(cur_color, uniqueImage)) {
                uniqueImage.emplace_back(cur_color);
                if (uniqueImage.size() > target_num) return true;
            }
        }
        return false;
    }

    int compute_num_colors(const cv::Mat &image8UC3)
    {
        std::vector<cv::Vec3b> uniqueImage;
        cv::Vec3b              cur_color;
        for (int i = 0; i < image8UC3.rows; i++) {
            cur_color = image8UC3.at<cv::Vec3b>(i, 0);
            if (!is_in(cur_color, uniqueImage)) uniqueImage.emplace_back(cur_color);
        }

        return uniqueImage.size();
    }

    bool is_in(const cv::Vec3b &cur_color, const std::vector<cv::Vec3b> &uniqueImage)
    {
        for (auto &color : uniqueImage)
            if (cur_color[0] == color[0] && cur_color[1] == color[1] && cur_color[2] == color[2]) return true;
        return false;
    }

    bool repeat_center(int cur_cluster, const cv::Mat &centers32FC3, int color_space)
    {
        cv::Mat centers8UC3 = cv::Mat(cur_cluster, 1, CV_8UC3);
        for (int i = 0; i < cur_cluster; i++) {
            auto center = centers32FC3.row(i);
            centers8UC3.at<cv::Vec3b>(i) = {uchar(center.at<float>(0)), uchar(center.at<float>(1)), uchar(center.at<float>(2))};
        }
        convert_color_space(centers8UC3, centers8UC3, color_space, true);
        std::vector<cv::Vec3b> unique_centers;
        cv::Vec3b              cur_center;
        for (int i = 0; i < cur_cluster; i++) {
            cur_center = centers8UC3.at<cv::Vec3b>(i, 0);
            if (!is_in(cur_center, unique_centers))
                unique_centers.emplace_back(cur_center);
            else
                return true;
        }
        return false;
    }

    void replace_centers(cv::Mat &ori_image, cv::Mat &new_image)
    {
        for (int i = 0; i < ori_image.rows; i++) {
            for (int j = 0; j < ori_image.cols; j++) {
                int       idx                 = this->m_flatten_labels.at<int>(i * ori_image.cols + j, 0);
                cv::Vec3b pixel               = this->m_centers8UC3.at<cv::Vec3b>(idx);
                new_image.at<cv::Vec3b>(i, j) = pixel;
            }
        }
    }
    void repalce_centers_aplha(cv::Mat &ori_image, cv::Mat &new_image)
    {
        int       cnt = 0;
        int       idx;
        cv::Vec3b center;
        for (int i = 0; i < ori_image.rows; i++) {
            for (int j = 0; j < ori_image.cols; j++) {
                cv::Vec4b pixel = ori_image.at<cv::Vec4b>(i, j);
                if ((int) pixel[3] < this->m_alpha_thres)
                    new_image.at<cv::Vec4b>(i, j) = pixel;
                else {
                    idx                           = this->m_flatten_labels.at<int>(cnt++, 0);
                    center                        = this->m_centers8UC3.at<cv::Vec3b>(idx);
                    new_image.at<cv::Vec4b>(i, j) = cv::Vec4b(center[0], center[1], center[2], pixel[3]);
                }
            }
        }
    }

    void convert_color_space(const cv::Mat &ori_image, cv::Mat &image, int color_space, bool reverse = false)
    {
        switch (color_space) {
        case 0: image = ori_image; break;
        case 1:
            if (reverse)
                cvtColor(ori_image, image, cv::COLOR_HSV2BGR);
            else
                cvtColor(ori_image, image, cv::COLOR_BGR2HSV);
            break;
        case 2:
            if (reverse)
                cvtColor(ori_image, image, cv::COLOR_Lab2BGR);
            else
                cvtColor(ori_image, image, cv::COLOR_BGR2Lab);
            break;
        default: break;
        }
    }

    cv::Mat flatten(cv::Mat &image)
    {
        int     num_pixels = image.rows * image.cols;
        cv::Mat img(num_pixels, 1, CV_32FC3);
        for (int i = 0; i < image.rows; i++) {
            for (int j = 0; j < image.cols; j++) {
                cv::Vec3f pixel                          = image.at<cv::Vec3b>(i, j);
                img.at<cv::Vec3f>(i * image.cols + j, 0) = pixel;
            }
        }
        return img;
    }
    cv::Mat flatten_alpha(cv::Mat &image)
    {
        int num_pixels = image.rows * image.cols;
        for (int i = 0; i < image.rows; i++)
            for (int j = 0; j < image.cols; j++) {
                cv::Vec4b pixel = image.at<cv::Vec4b>(i, j);
                if ((int) pixel[3] < this->m_alpha_thres) num_pixels--;
            }

        cv::Mat img(num_pixels, 1, CV_8UC3);
        int     cnt = 0;
        for (int i = 0; i < image.rows; i++) {
            for (int j = 0; j < image.cols; j++) {
                cv::Vec4b pixel = image.at<cv::Vec4b>(i, j);
                if ((int) pixel[3] >= this->m_alpha_thres) img.at<cv::Vec3b>(cnt++, 0) = cv::Vec3b(pixel[0], pixel[1], pixel[2]);
            }
        }
        return img;
    }
    cv::Mat flatten_vector(const std::vector<std::array<float, 4>> &ori_colors)
    {
        int num_pixels = ori_colors.size();

        cv::Mat image8UC3(num_pixels, 1, CV_8UC3);
        for (int i = 0; i < num_pixels; i++) {
            std::array<float, 4> pixel    = ori_colors[i];
            image8UC3.at<cv::Vec3b>(i, 0) = cv::Vec3b((int) (pixel[0] * 255.f), (int) (pixel[1] * 255.f), (int) (pixel[2] * 255.f));
        }
        return image8UC3;
    }
};
