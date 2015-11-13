/*
 * global.h
 *
 *  Created on: Mar 9, 2012
 *      Author: aitor
 */

#ifndef FAAT_PCL_REC_FRAMEWORK_GLOBAL_PIPELINE_H_
#define FAAT_PCL_REC_FRAMEWORK_GLOBAL_PIPELINE_H_

#include <flann/flann.h>
#include <pcl/common/common.h>
#include "source.h"
#include <v4r/common/faat_3d_rec_framework_defines.h>
#include <v4r/features/global_estimator.h>

namespace v4r
{
template<typename PointInT>
class V4R_EXPORTS GlobalClassifier {

protected:
    std::string training_dir_;

public:
    typedef typename pcl::PointCloud<PointInT>::Ptr PointInTPtr;

    virtual void
    setNN (int nn) = 0;

    virtual void
    getCategory (std::vector<std::string> & categories) const = 0;

    virtual void
    getConfidence (std::vector<float> & conf) const = 0;

    virtual void
    classify () = 0;

    virtual void
    setIndices (const std::vector<int> & indices) = 0;

    virtual void
    setInputCloud (const PointInTPtr & cloud) = 0;


    void
    setTrainingDir (const std::string & dir)
    {
        training_dir_ = dir;
    }
};

/**
     * \brief Nearest neighbor search based classification of PCL point type features.
     * FLANN is used to identify a neighborhood, based on which different scoring schemes
     * can be employed to obtain likelihood values for a specified list of classes.
     * Available features: ESF, VFH, CVFH
     * See apps/3d_rec_framework/tools/apps/global_classification.cpp for usage
     * \author Aitor Aldoma
     */

template<template<class > class Distance, typename PointInT, typename FeatureT>
class V4R_EXPORTS GlobalNNPipeline : public GlobalClassifier<PointInT>
{

protected:
    using GlobalClassifier<PointInT>::training_dir_;

    struct index_score
    {
        int idx_models_;
        int idx_input_;
        double score_;

        std::string model_name_;
    };

    struct sortIndexScores
    {
        bool
        operator() (const index_score& d1, const index_score& d2)
        {
            return d1.score_ < d2.score_;
        }
    } sortIndexScoresOp;

    struct sortIndexScoresDesc
    {
        bool
        operator() (const index_score& d1, const index_score& d2)
        {
            return d1.score_ > d2.score_;
        }
    } sortIndexScoresOpDesc;

    typedef typename pcl::PointCloud<PointInT>::Ptr PointInTPtr;
    typedef Distance<double> DistT;
    typedef Model<PointInT> ModelT;
    typedef boost::shared_ptr<ModelT> ModelTPtr;

    /** \brief Point cloud to be classified */
    PointInTPtr input_;

    /** \brief Model data source */
    typename boost::shared_ptr<Source<PointInT> > source_;

    /** \brief Computes a feature */
    typename boost::shared_ptr<GlobalEstimator<PointInT> > estimator_;

    /** \brief Descriptor name */
    std::string descr_name_;

    typedef std::pair<ModelTPtr, std::vector<double> > flann_model;
    flann::Matrix<double> flann_data_;
    flann::Index<DistT> * flann_index_;
    std::vector<flann_model> flann_models_;

    std::vector<int> indices_;

    //load features from disk and create flann structure
    void
    loadFeaturesAndCreateFLANN ();

    inline void
    convertToFLANN (const std::vector<flann_model> &models, flann::Matrix<double> &data)
    {
        data.rows = models.size ();
        data.cols = models[0].second.size (); // number of histogram bins

        flann::Matrix<double> flann_data (new double[models.size () * models[0].second.size ()], models.size (), models[0].second.size ());

        for (size_t i = 0; i < data.rows; ++i)
            for (size_t j = 0; j < data.cols; ++j)
            {
                flann_data.ptr ()[i * data.cols + j] = models[i].second[j];
            }

        data = flann_data;
    }

    void
    nearestKSearch (flann::Index<DistT> * index, const flann_model &model, int k, flann::Matrix<int> &indices, flann::Matrix<double> &distances);

    size_t NN_;
    std::vector<std::string> categories_;
    std::vector<float> confidences_;

    std::string first_nn_category_;

public:

    GlobalNNPipeline ()
    {
        NN_ = 1;
    }

    ~GlobalNNPipeline ()
    {
    }

    void
    setNN (int nn)
    {
        NN_ = nn;
    }

    void
    getCategory (std::vector<std::string> & categories) const
    {
        categories = categories_;
    }

    void
    getConfidence (std::vector<float> & conf) const
    {
        conf = confidences_;
    }

    /**
         * \brief Initializes the FLANN structure from the provided source
         */

    void
    initialize (bool force_retrain = false);

    /**
         * \brief Performs classification
         */

    void
    classify ();

    /**
         * \brief Sets the model data source_
         */
    void
    setDataSource (const typename boost::shared_ptr<Source<PointInT> > & source)
    {
        source_ = source;
    }

    /**
         * \brief Sets the model data source_
         */

    void
    setFeatureEstimator (const typename boost::shared_ptr<GlobalEstimator<PointInT> > & feat)
    {
        estimator_ = feat;
    }

    void
    setIndices (const std::vector<int> & indices)
    {
        indices_ = indices;
    }

    /**
         * \brief Sets the input cloud to be classified
         */
    void
    setInputCloud (const PointInTPtr & cloud)
    {
        input_ = cloud;
    }

    void
    setDescriptorName (const std::string & name)
    {
        descr_name_ = name;
    }
};
}
#endif /* REC_FRAMEWORK_GLOBAL_PIPELINE_H_ */
