/******************************************************************************
 * Copyright (c) 2017 Simon Schreiberhuber
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ******************************************************************************/

#pragma once
#include <v4r/segmentation/plane_extractor.h>
#include <v4r/common/camera.h>
#include <opencv/cv.h>

namespace v4r
{
class
PlaneExtractorTileParameter
{
public:
    int minNrPatches_; ///< The minimum number of blocks that are allowed to spawn a plane
    int patchDim_; ///< Patches are made of pixel squares that have exactly these side length
    float minBlockInlierRatio_; ///< The minimum ratio of points that have to be in a patch before it would get discarded.
    bool pointwiseNormalCheck_; ///< Activating this allowes to reduce a lot of calculations and improve speed by a lot
    float maxDistance_; ///< A maximum distance at which no plane segmentation is possible anymore
    float maxStepSize_; ///< maxStepSize to calculate normals there is a maximum step size which allowes to correctly distinguish between 2 adjacent plane segments.
    float maxInlierDist_; ///< The maximum distance a point is allowed to be out of his plane
    bool useVariableThresholds_; ///< useVariableThresholds
    float maxInlierBlockDist_; ///< The maximum distance two adjacent patches are allowed to be out of plane

    PlaneExtractorTileParameter()
        :
          minNrPatches_(5),
          patchDim_(10),
          minBlockInlierRatio_(0.95), //0.9f
          pointwiseNormalCheck_(false),
          maxDistance_(4.f),
          maxStepSize_(0.05f), //0.1f
          maxInlierDist_(0.01f),
          useVariableThresholds_(true),
          maxInlierBlockDist_(0.005f)
    {}
};

template<typename PointT>
class V4R_EXPORTS PlaneExtractorTile : public PlaneExtractor<PointT>
{
protected:
    using PlaneExtractor<PointT>::cloud_;
    using PlaneExtractor<PointT>::normal_cloud_;
    using PlaneExtractor<PointT>::all_planes_;

    Camera::ConstPtr cam_;
    PlaneExtractorTileParameter param_;

public:
    PlaneExtractorTile( const PlaneExtractorTileParameter &p = PlaneExtractorTileParameter() );

    virtual bool getRequiresNormals() const { return true; }

private:

    /**
     * @brief The PlaneMatrix struct
     * structure to store a symmetrical 3 by 3 matrix
     */
    struct PlaneMatrix{
        Eigen::Vector3d sum;
        double xx;
        double xy;
        double xz;
        double yy;
        double yz;
        double zz;
        int nrPoints;
        inline PlaneMatrix operator+(const PlaneMatrix &b)
        {
            PlaneMatrix a;
            a.sum=sum+b.sum;
            a.xx=xx+b.xx;
            a.xy=xy+b.xy;
            a.xz=xz+b.xz;
            a.yy=yy+b.yy;
            a.yz=yz+b.yz;
            a.zz=zz+b.zz;
            a.nrPoints=nrPoints+b.nrPoints;
            return a;
        }
        inline void operator+=(const PlaneMatrix &b)
        {
            sum=sum+b.sum;
            xx=xx+b.xx;
            xy=xy+b.xy;
            xz=xz+b.xz;
            yy=yy+b.yy;
            yz=yz+b.yz;
            zz=zz+b.zz;
            nrPoints=nrPoints+b.nrPoints;
        }
    };

    /**
     * @brief The PlaneSegment struct
     * plane segment
     */
    struct PlaneSegment
    {
        float x;
        float y;
        float z;
        int nrInliers;
    };

    //maybe supply this with a list of additional
    struct Plane{
        Eigen::Vector3f plane;
        int nrElements;
    };

    std::vector<PlaneMatrix> matrices;
    std::vector<Plane> planeList;
    std::vector<PlaneMatrix> planeMatrices;
    cv::Mat planes;
    cv::Mat centerPoints;
    cv::Mat patchIds;
    //cv::Mat planeList;
    //Eigen::Vector3f* planes;

    //big todo for speed: switch to Vector4f elements
    //http://eigen.tuxfamily.org/index.php?title=FAQ#Vectorization

    //two versions of is inlier one is also regarding the normal
    bool isInlier(const Eigen::Vector4f &point, const Eigen::Vector4f &normal, const Eigen::Vector4f &plane,
                  float cosThreshold, float distThreshold, bool doNormalTest = true);

    bool isInlier(const Eigen::Vector4f &point, const Eigen::Vector4f &normal, const Eigen::Vector4f &plane, float _planeNorm,
                  float cosThreshold, float distThreshold, bool doNormalTest = true);

    bool isInPlane(const Eigen::Vector4f &plane1, const Eigen::Vector4f &plane2, const Eigen::Vector4f &centerPlane2,
                   float cosThreshold, float distThreshold);

    bool isParallel(Eigen::Vector4f plane1, Eigen::Vector4f plane2,
                    float cosThreshold);

    Eigen::Vector4f calcPlaneFromMatrix(PlaneMatrix mat);

    void replace(int from,int to,int maxIndex);


    cv::Mat getDebugImage();
    cv::Mat getDebugImage(int channel, bool doNormalTest);

    int minAbsBlockInlier;
    int colsOfPatches; ///< The dimensions of the downsampled image of patches
    int rowsOfPatches; ///< The dimensions of the downsampled image of patches
    int maxId; ///< The highest used id of


    int allocateMemory();

    void calculatePlaneSegments(bool doNormalTest);

    //TODO: this uses up too much time... get rid of it
    void rawPatchClustering();

    void postProcessing1Direction(const int offsets[][2], bool doNormalTest, bool reverse, bool zTest);
    void postProcessing(bool doNormalTest, bool zTest);

    /**
     * @brief zBuffer
     * This buffer contains the distance of a point to the assumed plane.
     * Only used when doZTest is set to true
     */
    cv::Mat zBuffer;

    /**
     * @brief thresholdsBuffer
     * Stores the thresholds for the according patches:
     * channel1 maxBlockDistance
     * channel2 minCosBlockAngle
     * channel3 maxInlierDistance
     * channel4 minCosAngle
     */
    cv::Mat thresholdsBuffer;

    float maxBlockAngle; ///< The maximum angle that is allowed between two adjacent blocks to be able to connect them
    float minCosBlockAngle; ///< The cos of this block angle
    float (*minCosBlockAngleFunc)(float depth) = NULL; ///< minCosBlockAngleFunc
    float maxAngle; ///< The angle the normal vector of a pointer is never allowed to be off of a patch befor getting discarded.
    float minCosAngle; ///< The cos of this angle
    float (*minCosAngleFunc)(float depth) = NULL; ///< minCosAngleFunc
    float (*maxInlierDistFunc)(float depth) = NULL; ///< maxInlierDistFunc
    float (*maxInlierBlockDistFunc)(float depth) = NULL; ///< inlierBlockDistanceFunc

public:

    /**
     * @brief doZTest
     * Only the closest possible points get added to a plane
     */
    bool doZTest=true;

    //Some parameters for maximum
    cv::Mat segmentation;
    std::vector<Plane> resultingPlanes;

    //two structures that are mainly used internally:
    //maybe combine them to one structure for better memory access)
    cv::Mat normals;
    cv::Mat debug;

    void setMaxAngle(float angle)
    {
        maxAngle=angle;
        minCosAngle=cos(maxAngle);
    }

    void setMaxBlockAngle(float angle)
    {
        maxBlockAngle=angle;
        minCosBlockAngle=cos(maxBlockAngle);
    }

    void compute();

    cv::Mat generateDebugTextureForPlane(Eigen::Vector4f plane, int index, bool doNormalTest);

    /**
     * @brief generateColorCodedTexture
     * @return
     * a rgb image of the segmentation result
     */
    cv::Mat generateColorCodedTexture();

    /**
     * @brief generateColorCodedTextureDebug
     * @return
     */
    cv::Mat generateColorCodedTextureDebug();

    void
    setCamera(const Camera::ConstPtr &cam)
    {
        cam_ = cam;
    }


    typedef boost::shared_ptr< PlaneExtractorTile<PointT> > Ptr;
    typedef boost::shared_ptr< PlaneExtractorTile<PointT> const> ConstPtr;
};

}
