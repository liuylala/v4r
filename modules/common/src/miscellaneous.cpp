#include <v4r/common/miscellaneous.h>
#include <pcl/impl/instantiate.hpp>
#include <pcl/point_types.h>
#include <pcl/kdtree/kdtree_flann.h>

namespace v4r
{

void transformNormals(const pcl::PointCloud<pcl::Normal> & normals_cloud,
                             pcl::PointCloud<pcl::Normal> & normals_aligned,
                             const Eigen::Matrix4f & transform)
{
    normals_aligned.points.resize (normals_cloud.points.size ());
    normals_aligned.width = normals_cloud.width;
    normals_aligned.height = normals_cloud.height;

    #pragma omp parallel for schedule(dynamic)
    for (size_t k = 0; k < normals_cloud.points.size (); k++)
    {
        Eigen::Vector3f nt (normals_cloud.points[k].normal_x, normals_cloud.points[k].normal_y, normals_cloud.points[k].normal_z);
        normals_aligned.points[k].normal_x = static_cast<float> (transform (0, 0) * nt[0] + transform (0, 1) * nt[1]
                + transform (0, 2) * nt[2]);
        normals_aligned.points[k].normal_y = static_cast<float> (transform (1, 0) * nt[0] + transform (1, 1) * nt[1]
                + transform (1, 2) * nt[2]);
        normals_aligned.points[k].normal_z = static_cast<float> (transform (2, 0) * nt[0] + transform (2, 1) * nt[1]
                + transform (2, 2) * nt[2]);

        normals_aligned.points[k].curvature = normals_cloud.points[k].curvature;
    }
}


void transformNormals(const pcl::PointCloud<pcl::Normal> & normals_cloud,
                             pcl::PointCloud<pcl::Normal> & normals_aligned,
                             const std::vector<int> & indices,
                             const Eigen::Matrix4f & transform)
{
    normals_aligned.points.resize (indices.size ());
    normals_aligned.width = indices.size();
    normals_aligned.height = 1;
    for (size_t k = 0; k < indices.size(); k++)
    {
        Eigen::Vector3f nt (normals_cloud.points[indices[k]].normal_x,
                normals_cloud.points[indices[k]].normal_y,
                normals_cloud.points[indices[k]].normal_z);

        normals_aligned.points[k].normal_x = static_cast<float> (transform (0, 0) * nt[0] + transform (0, 1) * nt[1]
                + transform (0, 2) * nt[2]);
        normals_aligned.points[k].normal_y = static_cast<float> (transform (1, 0) * nt[0] + transform (1, 1) * nt[1]
                + transform (1, 2) * nt[2]);
        normals_aligned.points[k].normal_z = static_cast<float> (transform (2, 0) * nt[0] + transform (2, 1) * nt[1]
                + transform (2, 2) * nt[2]);

        normals_aligned.points[k].curvature = normals_cloud.points[indices[k]].curvature;

    }
}


void voxelGridWithOctree(pcl::PointCloud<pcl::PointXYZRGB>::Ptr & cloud,
                                pcl::PointCloud<pcl::PointXYZRGB> & voxel_grided,
                                float resolution)
{
    pcl::octree::OctreePointCloudPointVector<pcl::PointXYZRGB> octree(resolution);
    octree.setInputCloud(cloud);
    octree.addPointsFromInputCloud();

    pcl::octree::OctreePointCloudPointVector<pcl::PointXYZRGB>::LeafNodeIterator it2;
    const pcl::octree::OctreePointCloudPointVector<pcl::PointXYZRGB>::LeafNodeIterator it2_end = octree.leaf_end();

    int leaves = 0;
    for (it2 = octree.leaf_begin(); it2 != it2_end; ++it2, leaves++)
    {

    }

    voxel_grided.points.resize(leaves);
    voxel_grided.width = leaves;
    voxel_grided.height = 1;
    voxel_grided.is_dense = true;

    int kk=0;
    for (it2 = octree.leaf_begin(); it2 != it2_end; ++it2, kk++)
    {
        pcl::octree::OctreeContainerPointIndices& container = it2.getLeafContainer();
        std::vector<int> indexVector;
        container.getPointIndices (indexVector);

        int r,g,b;
        r = g = b = 0;
        pcl::PointXYZRGB p;
        p.getVector3fMap() = Eigen::Vector3f::Zero();

        for(size_t k=0; k < indexVector.size(); k++)
        {
            p.getVector3fMap() = p.getVector3fMap() +  cloud->points[indexVector[k]].getVector3fMap();
            r += cloud->points[indexVector[k]].r;
            g += cloud->points[indexVector[k]].g;
            b += cloud->points[indexVector[k]].b;
        }

        p.getVector3fMap() = p.getVector3fMap() / static_cast<int>(indexVector.size());
        p.r = r / static_cast<int>(indexVector.size());
        p.g = g / static_cast<int>(indexVector.size());
        p.b = b / static_cast<int>(indexVector.size());
        voxel_grided.points[kk] = p;
    }
}

template<typename PointInT>
void
getIndicesFromCloud(const typename pcl::PointCloud<PointInT>::ConstPtr & full_input_cloud,
                    const typename pcl::PointCloud<PointInT>::ConstPtr & search_points,
                    std::vector<int> & indices,
                    float resolution)
{
    pcl::octree::OctreePointCloudSearch<PointInT> octree (resolution);
    octree.setInputCloud (full_input_cloud);
    octree.addPointsFromInputCloud ();

    std::vector<int> pointIdxNKNSearch;
    std::vector<float> pointNKNSquaredDistance;

    indices.resize( search_points->points.size() );
    size_t kept=0;

    for(size_t j=0; j < search_points->points.size(); j++)
    {
        if (octree.nearestKSearch (search_points->points[j], 1, pointIdxNKNSearch, pointNKNSquaredDistance) > 0)
        {
            indices[kept] = pointIdxNKNSearch[0];
            kept++;
        }
    }
    indices.resize(kept);
}

template<typename PointT, typename Type>
void
getIndicesFromCloud(const typename pcl::PointCloud<PointT>::ConstPtr & full_input_cloud,
                    const typename pcl::PointCloud<PointT> & search_pts,
                    typename std::vector<Type> & indices,
                    float resolution)
{
    pcl::octree::OctreePointCloudSearch<PointT> octree (resolution);
    octree.setInputCloud (full_input_cloud);
    octree.addPointsFromInputCloud ();

    std::vector<int> pointIdxNKNSearch;
    std::vector<float> pointNKNSquaredDistance;

    indices.resize( search_pts.points.size() );
    size_t kept=0;

    for(size_t j=0; j < search_pts.points.size(); j++)
    {
        if (octree.nearestKSearch (search_pts.points[j], 1, pointIdxNKNSearch, pointNKNSquaredDistance) > 0)
        {
            indices[kept] = pointIdxNKNSearch[0];
            kept++;
        }
    }
    indices.resize(kept);
}

bool
incrementVector(const std::vector<bool> &v, std::vector<bool> &inc_v)
{
    inc_v = v;

    bool overflow=true;
    for(size_t bit=0; bit<v.size(); bit++)
    {
        if(!v[bit])
        {
            overflow = false;
            break;
        }
    }

    bool carry = v.back();
    inc_v.back() = !v.back();
    for(int bit=v.size()-2; bit>=0; bit--)
    {
        inc_v[bit] = v[ bit ] != carry;
        carry = v[ bit ] && carry;
    }
    return overflow;
}

template<typename PointT>
float computeMeshResolution (const typename pcl::PointCloud<PointT>::ConstPtr & input)
{
    typedef typename pcl::KdTree<PointT>::Ptr KdTreeInPtr;
    KdTreeInPtr tree (new pcl::KdTreeFLANN<PointT>);
    tree->setInputCloud (input);

    std::vector<int> nn_indices (9);
    std::vector<float> nn_distances (9);

    float sum_distances = 0.0;
    std::vector<float> avg_distances (input->points.size ());
    // Iterate through the source data set
    for (size_t i = 0; i < input->points.size (); ++i)
    {
        tree->nearestKSearch (input->points[i], 9, nn_indices, nn_distances);

        float avg_dist_neighbours = 0.0;
        for (size_t j = 1; j < nn_indices.size (); j++)
            avg_dist_neighbours += sqrtf (nn_distances[j]);

        avg_dist_neighbours /= static_cast<float> (nn_indices.size ());

        avg_distances[i] = avg_dist_neighbours;
        sum_distances += avg_dist_neighbours;
    }

    std::sort (avg_distances.begin (), avg_distances.end ());
    float avg = avg_distances[static_cast<int> (avg_distances.size ()) / 2 + 1];
    return avg;
}


template<typename DistType>
void
convertToFLANN ( const std::vector<std::vector<float> > &data, boost::shared_ptr< typename flann::Index<DistType> > &flann_index)
{
    if(data.empty()) {
        std::cerr << "No data provided for building Flann index!" << std::endl;
        return;
    }

    size_t rows = data.size ();
    size_t cols = data[0].size(); // number of histogram bins

    flann::Matrix<float> flann_data ( new float[rows * cols], rows, cols );

    for ( size_t i = 0; i < rows; ++i )
    {
        for ( size_t j = 0; j < cols; ++j )
            flann_data.ptr() [i * cols + j] = data[i][j];
    }
    flann_index.reset (new flann::Index<DistType> ( flann_data, flann::KDTreeIndexParams ( 4 ) ) );
    flann_index->buildIndex ();
}

template <typename DistType>
void nearestKSearch ( typename boost::shared_ptr< flann::Index<DistType> > &index, std::vector<float> descr, int k, flann::Matrix<int> &indices,
                        flann::Matrix<float> &distances )
{
    flann::Matrix<float> p = flann::Matrix<float> ( new float[descr.size()], 1, descr.size() );
    memcpy ( &p.ptr () [0], &descr[0], p.cols * p.rows * sizeof ( float ) );
    index->knnSearch ( p, indices, distances, k, flann::SearchParams ( 128 ) );
    delete[] p.ptr ();
}


#define PCL_INSTANTIATE_computeMeshResolution(T) template V4R_EXPORTS float computeMeshResolution<T>(const typename pcl::PointCloud<T>::ConstPtr &);
PCL_INSTANTIATE(computeMeshResolution, PCL_XYZ_POINT_TYPES )


template V4R_EXPORTS void
getIndicesFromCloud<pcl::PointXYZRGB, int>(const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr &full_input_cloud,
                                           const pcl::PointCloud<pcl::PointXYZRGB> &search_points,
                                           std::vector<int> &indices, float resolution);

template V4R_EXPORTS void
getIndicesFromCloud<pcl::PointXYZ, int>(const pcl::PointCloud<pcl::PointXYZ>::ConstPtr &full_input_cloud,
                                           const pcl::PointCloud<pcl::PointXYZ> &search_points,
                                           std::vector<int> &indices, float resolution);

template V4R_EXPORTS void
getIndicesFromCloud<pcl::PointXYZRGB, size_t>(const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr &full_input_cloud,
                                           const pcl::PointCloud<pcl::PointXYZRGB> &search_points,
                                           std::vector<size_t> &indices, float resolution);

template V4R_EXPORTS void
getIndicesFromCloud<pcl::PointXYZ, size_t>(const pcl::PointCloud<pcl::PointXYZ>::ConstPtr &full_input_cloud,
                                           const pcl::PointCloud<pcl::PointXYZ> &search_points,
                                           std::vector<size_t> &indices, float resolution);


template V4R_EXPORTS
std::vector<size_t>
createIndicesFromMask(const boost::dynamic_bitset<> &mask, bool invert);

template V4R_EXPORTS
std::vector<int>
createIndicesFromMask(const boost::dynamic_bitset<> &mask, bool invert);

template V4R_EXPORTS void convertToFLANN<flann::L1<float> > (const std::vector<std::vector<float> > &, boost::shared_ptr< flann::Index<flann::L1<float> > > &flann_index); // explicit instantiation.
template V4R_EXPORTS void convertToFLANN<flann::L2<float> > (const std::vector<std::vector<float> > &, boost::shared_ptr< flann::Index<flann::L2<float> > > &flann_index); // explicit instantiation.
template V4R_EXPORTS void nearestKSearch<flann::L1<float> > ( boost::shared_ptr< flann::Index< flann::L1<float> > > &index, std::vector<float> descr, int k, flann::Matrix<int> &indices,
flann::Matrix<float> &distances );
template void V4R_EXPORTS nearestKSearch<flann::L2<float> > ( boost::shared_ptr< flann::Index< flann::L2<float> > > &index, std::vector<float> descr, int k, flann::Matrix<int> &indices,
flann::Matrix<float> &distances );

}
