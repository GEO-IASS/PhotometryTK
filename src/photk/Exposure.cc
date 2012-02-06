//__BEGIN_LICENSE__
//  Copyright (c) 2009-2012, United States Government as represented by the
//  Administrator of the National Aeronautics and Space Administration. All
//  rights reserved.
//
//  The NGT platform is licensed under the Apache License, Version 2.0 (the
//  "License"); you may not use this file except in compliance with the
//  License. You may obtain a copy of the License at
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
// __END_LICENSE__


#include <iostream>
#include <time.h>

#include <vw/Core.h>
#include <vw/Image.h>
#include <vw/FileIO.h>
#include <vw/Cartography.h>
#include <vw/Math.h>
using namespace vw;
using namespace vw::cartography;

#include <photk/Reflectance.h>
#include <photk/Reconstruct.h>
#include <photk/ReconstructError.h>
#include <photk/Misc.h>
#include <photk/Weights.h>
#include <photk/Exposure.h>
using namespace photometry;

//determines the best guess for the exposure time from the reflectance model
//forces unit exposure time for the first frame
//Ara Nefian
void Init_Exposure(float *reflectanceAvgArray,
                   int numImages, float *exposureTimeArray) {
  int i;
  exposureTimeArray[0] = 1.0;
  for (i = 0; i < numImages; i++){
       exposureTimeArray[i] = (exposureTimeArray[0]*reflectanceAvgArray[0])/reflectanceAvgArray[i];
       printf("init exposure time[%d]=%f\n", i, exposureTimeArray[i]);
  }
}

//Ara Nefian
float ComputeGradient_Exposure(float T, float albedo) {
  float grad;
  grad = T*albedo;

  return grad;
}

//josh - moved to ReconstructError.cc and renamed to ComputeError
////Ara Nefian
//float ComputeError_Exposure(float intensity, float T,
//                            float albedo, float reflectance,
//                            Vector3 /*xyz*/,
//                            Vector3 /*xyz_prior*/) {
//  float error;
//  error = (intensity-T*albedo*reflectance);
//  return error;
//}
//
//float ComputeError_Exposure(float intensity, float T,
//                            float albedo, float reflectance) {
//  float error;
//  error = (intensity-T*albedo*reflectance);
//  return error;
//}
/*
//these functions will be removed - START
//void AppendExposureInfoToFile(string exposureFilename, string currInputFile, ModelParams currModelParams)
void photometry::AppendExposureInfoToFile(std::string exposureFilename,
                                              ModelParams currModelParams) {
  FILE *fp;
  std::string currInputFile = currModelParams.inputFilename;

  fp = fopen(exposureFilename.c_str(), "a");

  fprintf(fp, "%s %f\n", currInputFile.c_str(), currModelParams.exposureTime);

  fclose(fp);
}

std::vector<float>
photometry::ReadExposureInfoFile(std::string exposureFilename,
                                     int numEntries) {
  FILE *fp;
  std::vector<float> exposureTimeVector(numEntries);

  fp = fopen(exposureFilename.c_str(), "r");

  for (int i = 0; i < numEntries; i++){
    char *filename = new char[500];
    float exposureTime;
    fscanf(fp, "%s %f\n", filename, &exposureTime);
    printf("%f\n", exposureTime);
    delete filename;
    exposureTimeVector[i] = exposureTime;
  }
  fclose(fp);

  return exposureTimeVector;
}
*/

void photometry::AppendExposureInfoToFile(ModelParams modelParams)
{
  // Append the current exposure to the file.
  // This way when we do multiple albedo iterations we keep all the
  // exposures.
  FILE *fp;
  fp = fopen((char*)(modelParams.exposureFilename).c_str(), "a");
  std::cout << "Writing " << modelParams.exposureFilename << std::endl;
  fprintf(fp, "%f\n", modelParams.exposureTime);
  fclose(fp);
}

void photometry::ReadExposureInfoFromFile(ModelParams *modelParams)
{
  // Read the latest exposure from the file

  std::ifstream fp((modelParams->exposureFilename).c_str());
  //std::cout << "Reading " << modelParams->exposureFilename << std::endl;
  if (!fp.is_open()){
    modelParams->exposureTime = 1;
  }
  else{
    float val = 0;
    std::cout << "Reading from: " << modelParams->exposureFilename << std::endl;
    while( (!fp.eof()) && (fp >> val) ){
      std::cout << "Read value: " << val << std::endl;
      modelParams->exposureTime = val;
    }
    fp.close();
  }
}


//computes the exposure time for image mosaicing (no reflectance model)
void photometry::ComputeExposure(ModelParams *currModelParams,
                                     GlobalParams /*globalParams*/) {

  std::string curr_input_file = currModelParams->inputFilename;
  std::string curr_albedo_file = currModelParams->outputFilename;

  DiskImageView<PixelMask<PixelGray<uint8> > > curr_image(curr_input_file);
  DiskImageView<PixelMask<PixelGray<uint8> > > curr_albedo(curr_albedo_file);

  GeoReference curr_geo;
  read_georeference(curr_geo, curr_input_file);

  float currReflectance;

  printf("init exposure time = %f, file = %s\n", currModelParams->exposureTime, curr_input_file.c_str());

  float delta_nominator = 0.0;
  float delta_denominator = 0.0;

  for (int k=0; k < (int)curr_image.rows(); ++k) {
    for (int l=0; l < (int)curr_image.cols(); ++l) {

      Vector2 curr_sample_pix(l,k);

      if ( is_valid(curr_image(l,k)) ) {

        currReflectance = 1;
        float error = ComputeError((float)curr_image(l,k), currModelParams->exposureTime,
                                            (float)curr_albedo(l,k), currReflectance);
//        float error = ComputeError_Exposure((float)curr_image(l,k), currModelParams->exposureTime,
//                                            (float)curr_albedo(l,k), currReflectance);
        float gradient = ComputeGradient_Exposure( (float)curr_albedo(l,k), currReflectance);

        delta_nominator = delta_nominator + error*gradient;
        delta_denominator = delta_denominator + gradient*gradient;

      }
    }
  }

  float delta = delta_nominator/delta_denominator;
  currModelParams->exposureTime = currModelParams->exposureTime+delta;
  printf("updated exposure time = %f\n", currModelParams->exposureTime);
}

void photometry::ComputeExposureAlbedo(ModelParams *currModelParams,
                                           GlobalParams globalParams) {

  std::string curr_input_file = currModelParams->inputFilename;
  std::string curr_albedo_file = currModelParams->outputFilename;
  std::string DEM_file = currModelParams->meanDEMFilename;

  DiskImageView<PixelMask<PixelGray<uint8> > > curr_image(curr_input_file);
  DiskImageView<PixelMask<PixelGray<uint8> > > curr_albedo(curr_albedo_file);

  GeoReference curr_geo;
  read_georeference(curr_geo, curr_input_file);

  float currReflectance;

  //read the DEM file
  DiskImageView<PixelGray<float> >  dem_image(DEM_file);
  GeoReference curr_dem_geo;
  read_georeference(curr_dem_geo, DEM_file);

  printf("init exposure time = %f, file = %s\n", currModelParams->exposureTime, curr_input_file.c_str());

  float delta_nominator = 0.0;
  float delta_denominator = 0.0;

  for (int k=0; k < (int)curr_image.rows(); ++k) {
    for (int l=0; l < (int)curr_image.cols(); ++l) {

      Vector2 curr_sample_pix(l,k);

      if ( is_valid(curr_image(l,k)) ) {

        Vector2 lon_lat = curr_geo.pixel_to_lonlat(curr_sample_pix);
        Vector2 sample_pix_dem = curr_dem_geo.lonlat_to_pixel(curr_geo.pixel_to_lonlat(curr_sample_pix));

        int x = (int)sample_pix_dem[0];
        int y = (int)sample_pix_dem[1];

        //check for valid DEM coordinates
        if ((x>=0) && (x < dem_image.cols()) && (y>=0) && (y< dem_image.rows())){

          Vector3 longlat3(lon_lat(0),lon_lat(1),(dem_image)(x, y));
          Vector3 xyz = curr_geo.datum().geodetic_to_cartesian(longlat3);

          Vector2 sample_pix_dem_left;
          sample_pix_dem_left(0) = x-1;
          sample_pix_dem_left(1) = y;

          Vector2 sample_pix_dem_top;
          sample_pix_dem_top(0) = x;
          sample_pix_dem_top(1) = y-1;


          //check for valid DEM pixel value and valid left and top coordinates
          if ((sample_pix_dem_left(0) >= 0) && (sample_pix_dem_top(1) >= 0) && (dem_image(x,y) != globalParams.noDEMDataValue/*-10000*/)){

            Vector2 lon_lat_left = curr_dem_geo.pixel_to_lonlat(sample_pix_dem_left);
            Vector3 longlat3_left(lon_lat_left(0),lon_lat_left(1),(dem_image)(sample_pix_dem_left(0), sample_pix_dem_left(1)));
            Vector3 xyz_left = curr_geo.datum().geodetic_to_cartesian(longlat3_left);

            Vector2 lon_lat_top= curr_dem_geo.pixel_to_lonlat(sample_pix_dem_top);
            Vector3 longlat3_top(lon_lat_top(0),lon_lat_top(1),(dem_image)(sample_pix_dem_top(0), sample_pix_dem_top(1)));
            Vector3 xyz_top = curr_geo.datum().geodetic_to_cartesian(longlat3_top);

            //Vector3 normal = computeNormalFrom3DPoints(xyz, xyz_left, xyz_top);
            Vector3 normal = computeNormalFrom3DPointsGeneral(xyz, xyz_left, xyz_top);
            float phaseAngle;
            currReflectance = ComputeReflectance(normal, xyz, *currModelParams, globalParams, phaseAngle);
            float error = ComputeError((float)curr_image(l,k), currModelParams->exposureTime,
                                                (float)curr_albedo(l,k), currReflectance);
//            float error = ComputeError_Exposure((float)curr_image(l,k), currModelParams->exposureTime,
//                                                (float)curr_albedo(l,k), currReflectance, xyz, xyz);

            float gradient = ComputeGradient_Exposure(currModelParams->exposureTime, (float)curr_albedo(l,k));

            if (globalParams.useWeights == 0){
              delta_nominator = delta_nominator + error*gradient;
              delta_denominator = delta_denominator + gradient*gradient;

            }
            else{
              //float weight = ComputeWeights(curr_sample_pix, currModelParams->center2D, currModelParams->maxDistance);
              float weight = ComputeLineWeightsHV(curr_sample_pix, *currModelParams);
              delta_nominator = delta_nominator + error*gradient*weight;
              delta_denominator = delta_denominator + gradient*gradient*weight;
            }

            //delta_nominator = delta_nominator + error*gradient;
            //delta_denominator = delta_denominator + gradient*gradient;
          }
        }
      }
    }
  }

  float delta = delta_nominator/delta_denominator;
  printf("delta = %f\n", delta);
  currModelParams->exposureTime = currModelParams->exposureTime+delta;
  printf("updated exposure time = %f\n", currModelParams->exposureTime);
}

