//
// Computer Vision Helper Class aimed at easing relevant computer vision tasks
//
// Created by gishr on 19-3-29.
//

#ifndef SCENE_RETRIEVING_CV_HELPER_H
#define SCENE_RETRIEVING_CV_HELPER_H

#include <iostream>

#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/common/transforms.h>
#include <pcl/registration/gicp.h>
#include <pcl/conversions.h>
#include <pcl/PCLPointCloud2.h>

#include <Eigen/Core>
#include <opencv2/core/eigen.hpp>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/highgui.hpp>

#include <ros/ros.h>
#include <visualization_msgs/Marker.h>
#include <geometry_msgs/Quaternion.h>
#include <geometry_msgs/PoseStamped.h>



using namespace std;
using namespace Eigen;

class cv_helper{

public:

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;


    cv_helper(double _fx, double _fy, double _cx, double _cy, double _bf = 0)
            : fx(_fx), fy(_fy), cx(_cx), cy(_cy), bf(_bf)
    {

        K << fx, 0, cx,
             0, fy, cy,
             0, 0, 1;


        fxinv = 1 / fx;
        fyinv = 1 / fy;
        Kinv = K.inverse();
        f = (fx + fy) * 0.5;
        b = bf / f;

        cv::eigen2cv(K, Kmat);

        cout<<"cv_helper K: \n"<<K<<endl;
        cout<<"cv_helper Kmat: \n"<<Kmat<<endl;
        cout<<"cv_helper fx: "<<fx<<endl;
        cout<<"cv_helper fy: "<<fy<<endl;
        cout<<"cv_helper cx: "<<cx<<endl;
        cout<<"cv_helper cy: "<<cy<<endl;
        cout<<"cv_helper f: "<<f<<endl;
        cout<<"cv_helper b: "<<b<<endl;

        this->PosePublisher = this->nh.advertise<visualization_msgs::Marker>("/pose_visualizer",10);
        ros::Duration(1).sleep();

    }


    void setMask(string mask_path)
    {
        this->mMask = cv::imread(mask_path);

        assert(!this->mMask.empty());
    }

    void applyMask(cv::Mat& image)
    {
        cv::bitwise_and(image, this->mMask, image);
    }


    // project image points to world frame given camera K and current frame R and t
    vector<cv::Point3f> image2world(vector<cv::Point2f>& image_points, vector<float>& points_disparity, cv::Mat& R, cv::Mat& t) {

        assert(image_points.size() == points_disparity.size());

        cv::Point3f camera_point, map_point;
        vector<cv::Point3f> map_points;

        for(size_t i=0; i<image_points.size(); i++)
        {

            cv::Point2f pt = image_points[i];

            //points_disparity[i] = (this->bf) / (points_disparity[i] + 1e-5);
            float depth;
            depth = (this->bf) / ( points_disparity[i] );

            //cout<<"depth: "<<depth<<endl;

            camera_point.x = (pt.x - cx) * fxinv * depth;
            camera_point.y = (pt.y - cy) * fyinv * depth;
            camera_point.z = depth;

            Matrix3f R_eigen;
            Vector3f t_eigen;
            Vector3f pt_eigen;
            Vector3f map;

            cv::cv2eigen(R, R_eigen);
            cv::cv2eigen(t, t_eigen);

            pt_eigen[0] = camera_point.x;
            pt_eigen[1] = camera_point.y;
            pt_eigen[2] = camera_point.z;

            map = (R_eigen * pt_eigen + t_eigen);

            map_point.x = map[0];
            map_point.y = map[1];
            map_point.z = map[2];

            map_points.push_back(map_point);
        }

        return map_points;
    }


    // project image points to camera frame
    vector<cv::Point3f> image2cam(vector<cv::Point2f> image_points, vector<float> points_disparity) {

        assert(image_points.size() == points_disparity.size());

        cv::Point3f camera_point;
        vector<cv::Point3f> cam_points;

        cout<<"image2cam 1"<<endl;

        for(size_t i=0; i<image_points.size(); i++)
        {

            cv::Point2f pt = image_points[i];

            cout<<"image2cam 2"<<endl;

            points_disparity[i] = (this->bf) / (points_disparity[i] + 1e-5);

            camera_point.x = (pt.x - cx) * fxinv * points_disparity[i];
            camera_point.y = (pt.y - cy) * fyinv * points_disparity[i];
            camera_point.z = points_disparity[i];

            cam_points.push_back(camera_point);
        }

        cout<<"image2cam 3"<<endl;
        cout<<"image2cam 3: "<<cam_points.size()<<endl;

        return cam_points;
    }


    pcl::PointCloud<pcl::PointXYZ> PtsVec2PointCloud(vector<cv::Point3f> input_vector)
    {
        assert(!input_vector.empty());

        pcl::PointCloud<pcl::PointXYZ> cloud;

        cloud.width = input_vector.size();
        cloud.height = 1;
        cloud.is_dense = false;
        cloud.points.resize(cloud.width * cloud.height);

        for(size_t i=0; i< input_vector.size(); i++)
        {
            cloud.points[i].x = input_vector[i].x;
            cloud.points[i].y = input_vector[i].y;
            cloud.points[i].z = input_vector[i].z;
        }

        return cloud;
    }


    vector<cv::Point3f> Points3d2Points3f(vector<cv::Point3d> input_points)
    {
        vector<cv::Point3f> output_points;

        cv::Point3f temp_p;
        for(auto& p : input_points)
        {
            temp_p.x = p.x;
            temp_p.y = p.y;
            temp_p.z = p.z;

            output_points.push_back(temp_p);
        }

        return output_points;
    }

    vector<cv::Point3d> Points3f2Points3d(vector<cv::Point3f> input_points)
    {
        vector<cv::Point3d> output_points;

        cv::Point3d temp_p;
        for(auto& p : input_points)
        {
            temp_p.x = p.x;
            temp_p.y = p.y;
            temp_p.z = p.z;

            output_points.push_back(temp_p);
        }

        return output_points;
    }

    vector<cv::Point2d> Points2f2Points2d(vector<cv::Point2f> input_points)
    {
        vector<cv::Point2d> output_points;

        cv::Point2d temp_p;
        for(auto& p : input_points)
        {
            temp_p.x = p.x;
            temp_p.y = p.y;

            output_points.push_back(temp_p);
        }

        return output_points;
    }


    void image2KpAndDesp(cv::Mat image, vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors)
    {
        cv::Ptr<cv::ORB> orb = cv::ORB::create(1200);
        cv::Mat mask;

        orb->detectAndCompute(image, mask, keypoints, descriptors);

        cout<<"image2KpAndDesp size: "<<keypoints.size()<<", "<<descriptors.size()<<endl;
    }


    void StereoImage2MapPoints(cv::Mat& image_left_rect,
                               cv::Mat& image_right_rect,
                               cv::Mat& R,
                               cv::Mat& t,
                               vector<cv::KeyPoint>& Keypoints_left,
                               cv::Mat& descriptors_left,
                               vector<cv::Point3f>& mps)
    {

        //step 1, detect and compute kps and desp
        this->image2KpAndDesp(image_left_rect, Keypoints_left, descriptors_left);

        if(Keypoints_left.size()< 10)
            return;

        //step 2, convert kps to pt2f
        vector<cv::Point2f> InputKeypoints;
        cv::KeyPoint::convert(Keypoints_left, InputKeypoints);

        //step 3, conduct LK flow
        std::vector<unsigned char> PyrLKResults;
        std::vector<float> err;
        std::vector<cv::Point2f> PyrLKmatched_points;

        cv::calcOpticalFlowPyrLK(image_left_rect,
                                 image_right_rect,
                                 InputKeypoints,
                                 PyrLKmatched_points,
                                 PyrLKResults,
                                 err
        );

        //step 4, eliminate good points and only keep matched ones
        std::vector<cv::Point2f> matched_points;
        std::vector<float> disparity_of_points;

        for(int index = 0; index < InputKeypoints.size(); index++)
        {
            if(PyrLKResults[index] == 1)
            {
                matched_points.push_back(InputKeypoints[index]);
                disparity_of_points.push_back(InputKeypoints[index].x - PyrLKmatched_points[index].x);
            }
        }

        //step 5, given pts2f, disps, R and t, compute mps
        mps = this->image2world(matched_points, disparity_of_points, R, t);
    }




    void publishPose(cv::Mat R, cv::Mat t, int useRed = 1)
    {

//        ros::Duration(0.02).sleep();
        ros::Duration(0.002).sleep();
//        ros::Rate(30);

        visualization_msgs::Marker mark;
        mark.header.frame_id="/map";

        mark.id = this->PoseId;

        mark.color.a = 1.0;

        if(useRed)
        {
            mark.color.r = 1.0;
            mark.color.g = 0.0;
            mark.color.b = 0.0;
        }
        else
        {
            mark.color.r = 0.0;
            mark.color.g = 1.0;
            mark.color.b = 0.0;
        }

        mark.pose.position.x = t.at<double> (0,0);
        mark.pose.position.y = t.at<double> (0,1);
        mark.pose.position.z = t.at<double> (0,2);

//        mark.pose.orientation.x = quat.x();
//        mark.pose.orientation.y = quat.y();
//        mark.pose.orientation.z = quat.z();
//        mark.pose.orientation.w = quat.w();

        mark.pose.orientation.x = 1;
        mark.pose.orientation.y = 0;
        mark.pose.orientation.z = 0;
        mark.pose.orientation.w = 0;

        if(useRed)
        {
            mark.scale.x = 0.2;
            mark.scale.y = 0.2;
            mark.scale.z = 0.2;
        }
        else
        {
            mark.scale.x = 1;
            mark.scale.y = 1;
            mark.scale.z = 1;
        }

        mark.action = visualization_msgs::Marker::ADD;
        mark.type = visualization_msgs::Marker::ARROW;

        this->PosePublisher.publish(mark);

        this->PoseId+=1;
    }


    bool match2Images(vector<cv::KeyPoint>& kps1, cv::Mat& desps1,
                      vector<cv::KeyPoint>& kps2, cv::Mat& desps2,
                      vector<cv::DMatch>& result_matches)
    {

        //step 1, match raw desps
        std::vector<cv::DMatch> matches;
        cv::FlannBasedMatcher matcher = cv::FlannBasedMatcher(cv::makePtr<cv::flann::LshIndexParams>(12,20,2));
        matcher.match(desps1, desps2, matches);

        cout<<"desps1 shape: "<<desps1.size()<<endl;
        cout<<"desps2 shape: "<<desps2.size()<<endl;
        cout<<"match2Images 1: matches.size() "<<matches.size()<<endl;

        //step 2, find first bunch of good matches using desp distance
        double max_dist = 0;
        double min_dist = 100;
        for( int i = 0; i < matches.size(); i++ )
        {
            double dist = matches[i].distance;
            if(dist < min_dist)
                min_dist = dist;
            if(dist > max_dist)
                max_dist = dist;
        }

        std::vector< cv::DMatch > good_matches;

//        for( int i = 0; i < matches.size(); i++ )
//        {
//            if( matches[i].distance <= 2*min_dist && matches[i].distance< 20) // 3.0 too large;2.0 too large.
//            {
//                good_matches.push_back( matches[i]);
//            }
//        }

        for( int i = 0; i < matches.size(); i++ )
        {
            if( matches[i].distance <= 4*min_dist && matches[i].distance< 20) // 3.0 too large;2.0 too large.
            {
                good_matches.push_back( matches[i]);
            }
        }

        cout<<"match2Images: good_matches.size() "<<good_matches.size()<<endl;

        if (good_matches.size()<8)
            return false;

        //for debug
        result_matches = good_matches;
        return true;
        //--------------------------------------------------------------------------------

        vector<cv::Point2f> good_kps_old, good_kps_cur;
        for( size_t i = 0; i < good_matches.size(); i++ )
        {
            good_kps_old.push_back( kps1[good_matches[i].queryIdx].pt );
            good_kps_cur.push_back( kps2[good_matches[i].trainIdx].pt );
        }

        cout<<"good_kps_old size: "<<good_kps_old.size()<<endl;
        cout<<"good_kps_cur size: "<<good_kps_cur.size()<<endl;

        //step 3, use H to find second bunch of good matches
        cv::Mat isOutlierMask;
        cv::Mat fundamental_matrix = cv::findFundamentalMat(good_kps_old, good_kps_cur, cv::FM_RANSAC, 3, 0.99, isOutlierMask);

        std::vector<cv::DMatch> final_good_matches;
        for(int i = 0; i<good_matches.size(); i++)
        {
            if (isOutlierMask.at<int>(i)!=0)
            {
                final_good_matches.push_back(good_matches[i]);
            }
        }

        cout<<"final_good_matches size: "<<final_good_matches.size()<<endl;

        if(final_good_matches.size()<5)
            return false;


        result_matches = final_good_matches;
        return true;
    }


    // R and t from YGZ-SLAM are Rwc and Twc, which means they are from camera to world
    bool solvePnP(cv::Mat old_image_left, cv::Mat old_image_right,
                  cv::Mat cur_image_left, cv::Mat cur_image_right,
                  cv::Mat R, cv::Mat t,
                  cv::Mat& result_R, cv::Mat& result_t)
    {

        this->index++;

        this->applyMask(old_image_left);
        this->applyMask(old_image_right);
        this->applyMask(cur_image_left);
        this->applyMask(cur_image_right);


        //step 1, given old stereo images, R and t, get kps, desps and mps of old frame
        vector<cv::KeyPoint> Keypoints_old_left;
        cv::Mat descriptors_old_left;
        vector<cv::Point3f> MapPoints_old;

        cout<<"solve pnp 1: R and t: \n"<<R<<"\n"<<t<<endl;

        this->StereoImage2MapPoints(old_image_left,      //input
                                    old_image_right,     //input
                                    R, t,                //input
                                    Keypoints_old_left,  //output kps
                                    descriptors_old_left,//output desps
                                    MapPoints_old);      //output MapPoints in world frame


        //step 2, get kps and desps of current image
        vector<cv::KeyPoint> Keypoints_current_left;
        cv::Mat descriptors_current_left;
        this->image2KpAndDesp(cur_image_left, Keypoints_current_left, descriptors_current_left);


        //for test , cv::CV_FM_8POINT
//        vector<cv::Point2f> kps1, kps2;
//        cv::KeyPoint::convert(Keypoints_old_left, kps1);
//        cv::KeyPoint::convert(Keypoints_current_left, kps2);
//
//        cv::Mat fundamental_matrix;
//        fundamental_matrix = cv::findFundamentalMat (kps1, kps2);
//        cout<<"fundamental_matrix is "<<endl<< fundamental_matrix<<endl;
        //-------------------------------------------------------------------


        //step 3, find good matches between old frame and current frame and return good old_kps, old_mps and cur_kps
        vector<cv::DMatch> good_matches;
        vector<cv::KeyPoint> kps_old_left, kps_cur_left;
        vector<cv::Point3f> mps_old;


        if(this->match2Images(Keypoints_old_left, descriptors_old_left, Keypoints_current_left, descriptors_current_left, good_matches))
        {
            for(int i=0; i<good_matches.size(); i++)
            {
                kps_old_left.push_back(Keypoints_current_left[good_matches[i].queryIdx]);
                mps_old.push_back(MapPoints_old[good_matches[i].queryIdx]);

                kps_cur_left.push_back(Keypoints_current_left[good_matches[i].trainIdx]);
            }
        }
        else
        {
            return false;
        }


        // ---------------------------------------------------for debugging----------------------------------------------------------
        cv::Mat test_image;
        cv::drawMatches(old_image_left, Keypoints_old_left, cur_image_left, Keypoints_current_left, good_matches, test_image);
        if(!test_image.empty())
            cv::imwrite("./loopclosure_result2/" + std::to_string(this->index) + ".png", test_image);
        // --------------------------------------------------------------------------------------------------------------------------


//        cout<<"Fetched kps_old_left size: "<<kps_old_left.size()<<endl;
        cout<<"Fetched kps_cur_left size: "<<kps_cur_left.size()<<endl;
        cout<<"Fetched mps_old size: "<<mps_old.size()<<endl;

        assert(mps_old.size() == kps_cur_left.size());



        //step 3, conduct PnP ransac given old mps and current kps
        vector<cv::Point2f> image_pts_cur;
        cv::KeyPoint::convert(kps_cur_left, image_pts_cur);

        cout<<"converted image_pts_cur size: "<<image_pts_cur.size()<<endl;

        cv::Mat rvec, tvec;
        cv::Mat intrinstic;
        cv::eigen2cv(this->K, intrinstic);
        cv::Mat inliers;

        cv::Mat D = cv::Mat::zeros(4,1,cv::DataType<double>::type);

        //cv::solvePnPRansac(mps_old, image_pts_cur, intrinstic, NULL, rvec, tvec, 0, 30, 0.2, 5);
        cout<<"mps_old size(): "<<mps_old.size()<<endl;
        cout<<"image_pts_cur size(): "<<image_pts_cur.size()<<endl;


//        for(auto p: mps_old)
//            cout<<"mps_old: "<<p<<endl;
//
//        for(auto p: image_pts_cur)
//            cout<<"image_pts_cur: "<<p<<endl;


        //SOLVEPNP_P3P
        //SOLVEPNP_UPNP
        //SOLVEPNP_AP3P

        //NOTE this could be a good indication of the result
        //http://answers.opencv.org/question/87546/solvepnp-fails-with-perfect-coordinates-and-cvposit-passes/

        cv::Rodrigues(R, rvec);
        cv::Rodrigues(t, tvec);

        cv::solvePnPRansac(mps_old, image_pts_cur, this->Kmat, cv::Mat(), rvec, tvec, true, 100, 0.02, 0.99, inliers);

        cout<<"inliers 1: "<<inliers<<endl;

        cout<<"solvepnpransac 1"<<endl;

        cout<<"solve pnp 1"<<endl;

        cout<<"cv helper solve R 1: \n"<<R<<endl;
        cout<<"cv helper solve t 1: \n"<<t<<endl;
        cout<<"cv helper solve rvec 1: \n"<<rvec<<endl;
        cout<<"cv helper solve tvec 1: \n"<<tvec<<endl;


        cv::solvePnPRansac(mps_old, image_pts_cur, this->Kmat, cv::Mat(), rvec, tvec, true, 100, 0.02, 0.99, inliers, cv::SOLVEPNP_P3P);

        cout<<"inliers 2: "<<inliers<<endl;

        cout<<"solvepnpransac 2"<<endl;

        cout<<"solve pnp 2"<<endl;

        cout<<"cv helper solve rvec 2: \n"<<rvec<<endl;
        cout<<"cv helper solve tvec 2: \n"<<tvec<<endl;


        cv::solvePnPRansac(mps_old, image_pts_cur, this->Kmat, cv::Mat(), rvec, tvec, false, 100, 0.02, 0.99, inliers, cv::SOLVEPNP_EPNP);

        cout<<"inliers 3: "<<inliers<<endl;

        cout<<"solvepnpransac 3"<<endl;

        cout<<"solve pnp 3"<<endl;

        cout<<"cv helper solve rvec 3: \n"<<rvec<<endl;
        cout<<"cv helper solve tvec 3: \n"<<tvec<<endl;


        cv::solvePnPRansac(mps_old, image_pts_cur, this->Kmat, cv::Mat(), rvec, tvec, false, 100, 0.02, 0.99, inliers, cv::SOLVEPNP_UPNP);

        cout<<"inliers 3: "<<inliers<<endl;

        cout<<"solvepnpransac 3"<<endl;

        cout<<"solve pnp 3"<<endl;

        cout<<"cv helper solve rvec 3: \n"<<rvec<<endl;
        cout<<"cv helper solve tvec 3: \n"<<tvec<<endl;


        cv::Rodrigues (rvec, result_R);
        result_t = tvec;

        cv::Mat homographyR = this->getRotationfromEssential(kps_old_left, kps_cur_left);

        cout<<"homographyR is: "<<homographyR<<endl;

        cv::Mat homographyRvec;

        cv::Rodrigues (homographyR, homographyRvec);

        cout<<"homographyRvec is: "<<homographyRvec<<endl;
        cout<<"rvec is: "<<rvec<<endl;

        float distanceR = this->Vec3Distance(homographyR, rvec);

        cout<<"distanceR is: "<<distanceR<<endl;

        float distanceT = this->Vec3Distance(t, tvec);

        //if (distanceR < 1.5) // works OK, with few outliers, the majority are inliers, DBow query score threshold 0.07
        //if (distanceR < 2.0) // works moderate, with more outliers, the majority are inliers, DBow query score threshold 0.07
        //if (distanceR < 1.0) // works good, with nearly no outlier, score threshold 0.005
        //if (distanceR < 1.0) // works good, with nearly no outlier, score threshold 0.001
        //if (distanceR < 1.5) // works good, with 4 outliers, score threshold 0.001
        //if (distanceR < 3) // works poor, with 30 outliers, score threshold 0.001
        //if (distanceR < 2.5 && distanceT < 100) // works poor, with 10 outliers, score threshold 0.001
        //if (distanceR < 2.5 && distanceT < 50) // works poor than last, poor recall, with 10 outliers, score threshold 0.001
        //if (distanceR < 2.0 && distanceT < 100) // works good, with 5 outliers, score threshold 0.001
        //if (distanceR < 2.0 && distanceT < 50) // poor recall, with 4 outliers, score threshold 0.001
        if (distanceR < 1.5 && distanceT < 150) // decent recall, 2 outliers, score threshold 0.001
            return true;
        else
            return false;


        cv::solvePnPRansac(mps_old, image_pts_cur, this->Kmat, cv::Mat(), rvec, tvec, false, 100, 0.5, 0.99, inliers, cv::SOLVEPNP_AP3P);

        cout<<"inliers 4: "<<inliers<<endl;

        cout<<"solvepnpransac 4"<<endl;

        cout<<"solve pnp 4"<<endl;

        cout<<"cv helper solve rvec 4: \n"<<rvec<<endl;
        cout<<"cv helper solve tvec 4: \n"<<tvec<<endl;

        //-----------------------final result-------------------------
        cv::Rodrigues (rvec, result_R);
        result_t = tvec;

        return true;
    }


    // a wrapper for pcl::GeneralizedIterativeClosestPoint to return the transformation matrix between a input point cloud and a
    // target point cloud
    // input points cloud size should be greater than 20
    Eigen::Matrix4f GeneralICP(vector<cv::Point3f> input_cloud, vector<cv::Point3f> target_cloud, int num_iter = 50, double transformationEpsilon = 1e-8)
    {

        cout<<"cv helper::GeneralICP points size: "<<input_cloud.size()<<", "<<target_cloud.size()<<endl;

        typedef pcl::PointXYZ PointT;

        // define input point cloud
        pcl::PointCloud<PointT>::Ptr src (new pcl::PointCloud<PointT>);
        pcl::PointCloud<PointT> src_cloud = PtsVec2PointCloud(input_cloud);
        *src  = src_cloud;

        // define target point cloud
        pcl::PointCloud<PointT>::Ptr tgt (new pcl::PointCloud<PointT>);
        pcl::PointCloud<PointT> tgt_cloud = PtsVec2PointCloud(target_cloud);
        *tgt = tgt_cloud;

        // define output point cloud
        pcl::PointCloud<PointT> output;

        // parameter setter
        pcl::GeneralizedIterativeClosestPoint<PointT, PointT> reg;
        reg.setInputSource(src);
        reg.setInputTarget(tgt);
        reg.setMaximumIterations(num_iter);
        reg.setTransformationEpsilon(transformationEpsilon);

        reg.align (output);

        // expect fitness score is less than a certain value
        cout<<"GeneralICP::General ICP fitness score: "<<reg.getFitnessScore()<<endl;

        Eigen::Matrix4f transformation;
        transformation = reg.getFinalTransformation();

        cout<<"GeneralICP::transformation matrix: "<<transformation<<endl;
    }

    // print type of cv::Mat

    string type2str(int type) {
        string r;

        uchar depth = type & CV_MAT_DEPTH_MASK;
        uchar chans = 1 + (type >> CV_CN_SHIFT);

        switch ( depth ) {
            case CV_8U:  r = "8U"; break;
            case CV_8S:  r = "8S"; break;
            case CV_16U: r = "16U"; break;
            case CV_16S: r = "16S"; break;
            case CV_32S: r = "32S"; break;
            case CV_32F: r = "32F"; break;
            case CV_64F: r = "64F"; break;
            default:     r = "User"; break;
        }

        r += "C";
        r += (chans+'0');

        return r;
    }

    float Vec3Distance(cv::Mat a, cv::Mat b)
    {
        float result = abs(a.at<double>(0,0) - b.at<double>(0,0))
                    +  abs(a.at<double>(0,1) - b.at<double>(0,1))
                    +  abs(a.at<double>(0,2) - b.at<double>(0,2));

        cout<<"Vec3Distance result is: "<<result<<endl;

        return result;
    }

    cv::Mat getRotationfromEssential(vector<cv::KeyPoint>& pts1, vector<cv::KeyPoint>& pts2)
    {
        cv::Point2d principal_point(this->cx, this->cy);
        int focal_length = this->f;

        vector<cv::Point2f> pts1f, pts2f;
        cv::KeyPoint::convert(pts1, pts1f);
        cv::KeyPoint::convert(pts2, pts2f);

        cv::Mat essential_mat = cv::findEssentialMat(pts1f, pts2f, focal_length, principal_point, cv::RANSAC);

        cout<<"Recovered essential mat is: "<<essential_mat<<endl;

        cv::Mat R, t;
        cv::recoverPose(essential_mat, pts1f, pts2f, R, t, focal_length, principal_point);

        return R;
    }

public:

    double fx = 0;
    double fy = 0;
    double cx = 0;
    double cy = 0;
    double fxinv = 0;
    double fyinv = 0;
    double b = 0;
    double f = 0;
    double bf = 0;

    Eigen::Matrix3d K = Matrix3d::Identity();     // intrinsics

    cv::Mat Kmat;

    Eigen::Matrix3d Kinv = Matrix3d::Identity();  // inverse K

    int index=0;

    cv::Mat mMask;

    //for pose visualization
    size_t PoseId = 0;

    ros::NodeHandle nh;
    ros::Publisher PosePublisher;
};


#endif //SCENE_RETRIEVING_CV_HELPER_H
