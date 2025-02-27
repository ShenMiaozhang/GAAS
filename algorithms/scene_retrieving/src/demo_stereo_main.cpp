//receive ros msg:1.image_l,image_r   2.position.
//output:scene struct.
#include <memory>
#include <opencv2/opencv.hpp>
#include "scene_retrieve.h"
#include <iostream>

#include <ros/ros.h>

using namespace std;

int main(int argc,char** argv)
{


    //ros init
    ros::init(argc, argv, "scene_retrieve");

    //NOTE simple test, for fast serialization test
//     std::shared_ptr<Scene> pSceneTest(new Scene());
//     string test_scene_path = "../../scene.scn";
//     pSceneTest->loadFile(test_scene_path);
//     pSceneTest->test();
    
    if (argc!=5)
    {
        cout<<"Usage: demo [scene_file_path] [voc_file_path] [l_image_path] [r_image_path] [Q_mat_file_path]"<<endl;
    }

    std::string scene_path(argv[1]), voc_file_path(argv[2]) , l_img_path(argv[3]), r_img_path(argv[4]), Q_mat_path(argv[5]);
    
    cout<<"scene path: "<<scene_path<<endl;
    cout<<"voc_file_path: "<<voc_file_path<<endl;
    cout<<"l_img_path: "<<l_img_path<<endl;
    cout<<"r_img_path: "<<r_img_path<<endl;
    cout<<"Q_mat_path: "<<Q_mat_path<<endl;
    
    cv::FileStorage fsSettings(Q_mat_path, cv::FileStorage::READ);
    cv::Mat Q_mat;
    fsSettings["Q_mat"] >> Q_mat;
        
    if (Q_mat.empty())
    {
        cout<<"Q mat empty, exit."<<endl;
        return -1;
    }
    

    cout<<"Q_mat: "<<endl<<Q_mat<<endl;
    
    cv::Mat RT_mat = (cv::Mat_<float >(4,4) << 1, 0, 0, 0,
                                           0, 1, 0, 1,
                                           0, 0, 1, 0,
                                           0, 0, 0, 1);

    bool match_success;

    std::shared_ptr<SceneRetriever> pSceneRetriever(new SceneRetriever(voc_file_path, scene_path));

    int image_num = 7000;
    vector<string> left_image_path, right_image_path;
    for (int i=0; i<image_num; i++)
    {
        string left_path = "./image/left/" + to_string(i) + ".png";
        left_image_path.push_back(left_path);
        
        string right_path = "./image/right/" + to_string(i) + ".png";
        right_image_path.push_back(right_path);
    }
    


    for (int i=0; i<image_num; i++)
    {
        pSceneRetriever->retrieveSceneFromStereoImage(cv::imread(left_image_path[i]), cv::imread(right_image_path[i]), Q_mat, RT_mat, match_success);

        pSceneRetriever->setImageVecPath(left_image_path, 1);
        pSceneRetriever->setImageVecPath(right_image_path, 0);

        if(match_success)
        {
            cout<<to_string(i)<<" Match success!\tRT mat:"<<RT_mat<<endl;
        }
    }


    
    return 0;
}








