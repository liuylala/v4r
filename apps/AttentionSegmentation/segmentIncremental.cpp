/**
 *  Copyright (C) 2012  
 *    Ekaterina Potapova, Andreas Richtsfeld, Johann Prankl, Thomas Mörwald, Michael Zillich
 *    Automation and Control Institute
 *    Vienna University of Technology
 *    Gusshausstraße 25-29
 *    1170 Vienna, Austria
 *    ari(at)acin.tuwien.ac.at
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @file segmentAttention.cpp
 * @author Andreas Richtsfeld, Ekaterina Potapova
 * @date January 2014
 * @version 0.1
 * @brief Segments the scene using attention.
 */


#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <fstream>

#include <pcl/io/pcd_io.h>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>

#include "v4r/attention_segmentation/PCLUtils.h"
#include "v4r/attention_segmentation/segmentation.h"
#include "v4r/attention_segmentation/AttentionModule.h"
#include "v4r/attention_segmentation/EPEvaluation.h"

void readData(std::string filename, pcl::PointCloud<pcl::PointXYZRGB>::Ptr &pcl_cloud, pcl::PointCloud<pcl::PointXYZRGBL>::Ptr &pcl_cloud_l)
{

  if(!(pclAddOns::readPointCloud<pcl::PointXYZRGBL>(filename.c_str(),pcl_cloud_l)))
  {
    //exit(0);
    if(!(pclAddOns::readPointCloud<pcl::PointXYZRGB>(filename.c_str(),pcl_cloud)))
    {
      exit(0);
    }
    v4r::ClipDepthImage(pcl_cloud);
    return;
  }
  
  v4r::ConvertPCLCloud(pcl_cloud_l,pcl_cloud);
  v4r::ClipDepthImage(pcl_cloud);
}

/*    std::vector<int> tp_indices;
    std::vector<int> fp_indices;
    std::vector<int> fn_indices;
    evaluateSegmentation(tp_indices,fp_indices,fn_indices,originalIndex,attentionPoints.at(i));
    std::string output_filename = "segmentation_results_attention.txt";
    printSegmentationEvaluation(output_filename,filename,tp_indices,fp_indices,fn_indices,attentionPoints.at(i));*/

void showSegmentation(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud, cv::Mat &kImage, cv::Mat &mask)
{
  srand (time(NULL));
  
  // create color image
  kImage = cv::Mat_<cv::Vec3b>(cloud->height, cloud->width);
  for (unsigned int row = 0; row < cloud->height; row++)
  {
    for (unsigned int col = 0; col < cloud->width; col++)
    {
      cv::Vec3b &cvp = kImage.at<cv::Vec3b> (row, col);
      int idx = row * cloud->width + col;
      const pcl::PointXYZRGB &pt = cloud->points.at(idx);
      cvp[2] = pt.r;
      cvp[1] = pt.g;
      cvp[0] = pt.b;
    }
  }
  
  int maxLabel = 0;
  for(int i = 0; i < mask.rows; ++i)
  {
    for(int j = 0; j < mask.cols; ++j)
    {
      if(mask.at<uchar>(i,j) > maxLabel)
        maxLabel = mask.at<uchar>(i,j);
    }
  }
  
  if(maxLabel <= 0)
  {
    printf("Error: there are not objects in the mask! \n");
    return;
  }
  
  uchar r[maxLabel], g[maxLabel], b[maxLabel];
  for(int i = 0; i < maxLabel; i++)
  {
    r[i] = std::rand()%255;
    g[i] = std::rand()%255;
    b[i] = std::rand()%255;
  }
  
  for(int i = 0; i < mask.rows; ++i)
  {
    for(int j = 0; j < mask.cols; ++j)
    {
      if(mask.at<uchar>(i,j) > 0)
      {
        cv::Vec3b &cvp = kImage.at<cv::Vec3b> (i,j);
        cvp[0] = r[mask.at<uchar>(i,j)-1];
        cvp[1] = g[mask.at<uchar>(i,j)-1];
        cvp[2] = b[mask.at<uchar>(i,j)-1];
      }
    }
  }
}

void createAttention(std::string &saliency_filename, pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud, std::vector<cv::Mat> &saliencyMaps)
{
  // saliency map
  cv::Mat salMap = cv::imread(saliency_filename,-1);
  salMap.convertTo(salMap,CV_32F,1.0/255);
    
  saliencyMaps.resize(1);
  salMap.copyTo(saliencyMaps.at(0));
}

void printUsage(char *av)
{
  printf("Usage: %s cloud.pcd model.txt scaling_params.txt objNumber saliency.png save_image.png times_file.txt evaluation.txt\n"
    " Options:\n"
    "   [-h] ... show this help.\n"
    "   cloud.pcd               ... specify rgbd-image filename\n"
    "   model.txt               ... model filename\n"
    "   scaling_params.txt      ... file with scaling params\n"
    "   objNumber               ... number of objects to segment\n"
    "   saliency.png            ... saliency map\n" 
    "   save_image.png          ... image with segmentation results\n"
    "   times.txt               ... file to write times to\n"
    "   evaluation.txt          ... file to write segmentation evaluation\n", av);
  std::cout << " Example: " << av << " cloud.pcd model.txt scaling_params.txt 2 saliency.png [save_image.png times.txt evaluation.txt] " << std::endl;
}

int main(int argc, char *argv[])
{
  if(argc < 6)
  {
    printUsage(argv[0]);
    exit(0);
  }
  
  std::string rgbd_filename = argv[1];
  std::string model_file_name = argv[2];
  std::string scaling_params_name = argv[3];
  
  int objNumber = atoi(argv[4]);
  std::string saliency_name = argv[5];
  
  bool saveImage = false;
  std::string save_image_filename;
  if(argc >= 7)
  {
    saveImage = true;
    save_image_filename = argv[6];
  }
  
  bool writeTime = false;
  std::string times_filename;
  if(argc >= 8)
  {
    writeTime = true;
    times_filename = argv[7];
  }
  
  bool writeEvaluation = false;
  std::string evaluation_filename;
  if(argc >= 9)
  {
    writeEvaluation = true;
    evaluation_filename = argv[8];
  }
  
  pcl::PointCloud<pcl::PointXYZRGBL>::Ptr pcl_cloud_l( new pcl::PointCloud<pcl::PointXYZRGBL>() );        ///< labeled pcl-cloud
  pcl::PointCloud<pcl::PointXYZRGB>::Ptr pcl_cloud( new pcl::PointCloud<pcl::PointXYZRGB>() );           ///< original pcl point cloud
  
  readData(rgbd_filename,pcl_cloud,pcl_cloud_l);
  
  // read attention points or saliency maps
  std::vector<cv::Mat> saliencyMaps;
  createAttention(saliency_name,pcl_cloud,saliencyMaps);
  
  v4r::Segmenter segmenter; 
  segmenter.setPointCloud(pcl_cloud);
  segmenter.setModelFilename(model_file_name);
  segmenter.setScaling(scaling_params_name);
  segmenter.setSaliencyMaps(saliencyMaps);
  
  segmenter.attentionSegmentInit();
  
  for(int k = 0; k < objNumber; ++k)
  {
    if(!segmenter.attentionSegmentNext())
    {
      printf("No more objects left!\n");
      return(0);
    }
    
    std::vector<cv::Mat> masks = segmenter.getMasks();
  
    if(writeTime)
    {
      v4r::TimeEstimates tEst = segmenter.getTimeEstimates();
      FILE *f;
    
      if ( !boost::filesystem::exists(times_filename) )
      {
        f = fopen(times_filename.c_str(), "a");
        fprintf(f,"%12s %20s %20s %20s %20s %20s %20s %20s %10s %30s\n","image_name","normals","patches","patchImage","neighbours","border","preRelations","initModelSurf",
                  "masksNum","<the rest> ... total");
      }
      else
      {
        f = fopen(times_filename.c_str(), "a");
      }
    
      assert(tEst.times_saliencySorting.size() == masks.size());
      assert(tEst.times_surfaceModelling.size() == masks.size());
      assert(tEst.times_relationsComputation.size() == masks.size());
      assert(tEst.times_graphBasedSegmentation.size() == masks.size());
      assert(tEst.times_maskCreation.size() == masks.size());
      assert(tEst.times_neigboursUpdate.size() == masks.size());
      assert(tEst.time_totalPerSegment.size() == masks.size());
    
      boost::filesystem::path rgbd_filename_path(rgbd_filename);
      fprintf(f,"%12s %20lld %20lld %20lld %20lld %20lld %20lld %20lld %10ld",rgbd_filename_path.filename().c_str(),
              tEst.time_normalsCalculation,
              tEst.time_patchesCalculation,
              tEst.time_patchImageCalculation,
              tEst.time_neighborsCalculation,
              tEst.time_borderCalculation,
              tEst.time_relationsPreComputation,
              tEst.time_initModelSurfaces,
              masks.size());

      fprintf(f,"%20lld %20lld %20lld %20lld %20lld %20lld %20lld",
              tEst.times_saliencySorting.at(0),
              tEst.times_surfaceModelling.at(0),
              tEst.times_relationsComputation.at(0),
              tEst.times_graphBasedSegmentation.at(0),
              tEst.times_maskCreation.at(0),
              tEst.times_neigboursUpdate.at(0),
              tEst.time_totalPerSegment.at(0));

      fprintf(f,"%20Ld\n",tEst.time_total);
    
      fclose(f);
    }
  
    // if we are not saving something -- show it
    if(!saveImage)
    {
      cv::Mat kImage;
      showSegmentation(pcl_cloud,kImage,masks.at(0));
      
      //v4r::drawSegmentationMask(kImage,mask,color,2);
      //circle(kImage,attentionPoint,3,color,-1);
      
      cv::imshow("kImage",kImage);
      cv::imshow("salMap",saliencyMaps.at(0));
      cv::waitKey(-1);      
    }
  
    if(saveImage)
    {
      cv::Mat kImage;
      showSegmentation(pcl_cloud,kImage,masks.at(0));
      
      char a[30];
      sprintf(a,"_%d.png",k);
      
      std::string image_name = save_image_filename + a;
      cv::imwrite(image_name,kImage);
    
      std::string mask_name = save_image_filename + "_mask" + a;
      cv::imwrite(mask_name,255*(masks.at(0)));    
    }
  
    if(writeEvaluation)
    {
      EPEvaluation::evaluate(pcl_cloud_l,masks.at(0),rgbd_filename,evaluation_filename);
    }
  }
  
  return(0);
}


