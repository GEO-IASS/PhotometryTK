#!/bin/bash
# __BEGIN_LICENSE__
#  Copyright (c) 2009-2012, United States Government as represented by the
#  Administrator of the National Aeronautics and Space Administration. All
#  rights reserved.
#
#  The NGT platform is licensed under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance with the
#  License. You may obtain a copy of the License at
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
# __END_LICENSE__

# Orthoproject an ISIS cube onto each of the provided DEM tiles and combine the obtained output images.
# This script requires the availability of Ames Stereo Pipeline, ISIS libraries, and GDAL tools.

if [ "$#" -lt 6 ]; then 
    echo Usage: "$0 input.cub input_isis_adjust DEM_dir mpp num_proc output.tif"
    exit
fi

# Set up for ISIS and path to the Stereo Pipeline orthoproject tool
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/projects/base_system/lib/
export ISISROOT=$HOME/projects/base_system/isis
export ISIS3DATA=isis3data
$ISISROOT/scripts/isis3Startup.sh
ORTHOPROJECT=$HOME/StereoPipeline/src/asp/Tools/orthoproject

cub=$1; isis_adjust=$2; demDir=$3; mpp=$4; numProc=$5; outputTif=$6

# Spice init on the cube
$ISISROOT/bin/spiceinit from=$cub

# Create a temporary directory
tmpDir=$(echo $cub | perl -pi -e "s#[\/\.]#_#g")
tmpDir=/tmp/$tmpDir
rm -rfv $tmpDir
mkdir -p $tmpDir

# Orthoproject on individual DEM tiles
ls $demDir/*tif | xargs -P $numProc -I {} orthoproject_cube_parallel.sh $ORTHOPROJECT {} $cub $isis_adjust $mpp $tmpDir

# Combine the output files into one virtual image
vrtFile=$tmpDir/output.tif
gdalbuildvrt $vrtFile $tmpDir/*tif

# Create a proper image from the virtual image
gdal_translate -co compress=lzw $vrtFile $outputTif

rm -rfv $tmpDir
