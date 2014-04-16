#ifndef AUX_HPP
#define AUX_HPP


#include <string>
#include <Eigen/Core>
#include <Eigen/Geometry>

extern bool USE_IMU;
extern std::string IMU_FRAME;
extern std::string WORLD_FRAME;
extern std::string CAM_FRAME;
extern Eigen::Affine3d IMU2CAM;
#include <boost/assign.hpp>


#include <opengv/types.hpp>
#include <opencv2/opencv.hpp>

#include <tf/tf.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <visualization_msgs/MarkerArray.h>
#include <eigen_conversions/eigen_msg.h>

//#include <ollieRosTools/Landmark.hpp>
//#include <ollieRosTools/Frame.hpp>
class Landmark;
class Frame;


#define __SHORTFILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// 3d
typedef opengv::bearingVector_t Bearing;
typedef opengv::bearingVectors_t Bearings;
typedef opengv::point_t Point3d;
typedef opengv::points_t Points3d;
typedef Eigen::Affine3d Pose;

// 2d
typedef std::vector< cv::KeyPoint > KeyPoints;
typedef std::vector< cv::Point2d > Points2d;
typedef std::vector< cv::Point2f > Points2f;
typedef Points2d Points2;

// Associations
typedef std::vector< cv::DMatch > DMatches;
typedef std::vector< DMatches > DMatchesKNN;
typedef std::vector< bool > Bools;
typedef std::vector< int > Ints;
typedef std::vector< float > Floats;
typedef std::vector< uchar > UChars;
typedef std::vector< uint > UInts;
typedef std::vector< double > Doubles;



// Containers
typedef std::vector< cv::Mat > Mats;
typedef cv::Ptr<Landmark> LandmarkPtr;
typedef std::deque<LandmarkPtr> LandMarkPtrs;

typedef cv::Ptr<Frame> FramePtr;
typedef std::deque<FramePtr> FramePtrs;




const float toRad = M_PI/180;
const float toDeg = 180/M_PI;



namespace OVO {

    /// ENUMS
    enum BEARING_ERROR {BVERR_OneMinusAdotB, // 1-A.B
                    BVERR_ATAN2,             // ATAN2(||AxB||, A.B)
                    BVERR_NormAminusB,       // ||A-B||
                    BVERR_SUM_AminusBSqr     // sum((A-B)**2
                   };

    const BEARING_ERROR BVERR_DEFAULT = BVERR_OneMinusAdotB;


    /// Other
    cv::Mat getRosImage(const sensor_msgs::ImageConstPtr& msg, int colorId = 0);
    void tf2RPY(const tf::Transform& T, double& R, double& P, double& Y);
    visualization_msgs::Marker getPointsMarker(const Points3d& worldPoints);
    void testColorMap();


    /// Drawing Functions
    // Returns a color based on an internal colour map Green->Red (via blue, missing yellow on purpose) TODO: put this in a look up table
    CvScalar getColor(const float range_min, const float range_max, const float depth, bool reverse = false);
    // Draws text centered in ROI img
    void drawTextCenter(cv::Mat& img, const std::string& text, const CvScalar RGB, const float textScale, const int textThickness);
    // Draw a number onto an image
    void putInt(cv::Mat& img,  const float nr, const cv::Point& p, const CvScalar& col, const bool round, const std::string& str,const std::string& post="");
    // used to convert [cant remember] to ros types
    static std::map<int, std::string> COLORS= boost::assign::map_list_of
            (-1, std::string(""))
            (0, sensor_msgs::image_encodings::BGR8)
            (1, sensor_msgs::image_encodings::RGB8)
            (2, sensor_msgs::image_encodings::MONO8)
            (3, sensor_msgs::image_encodings::YUV422);
    cv::Mat rotateImage(const cv::Mat& in, const double angleRad, const int interpolation=CV_INTER_LINEAR, const double scale=1.0, const double threshDeg=0.1);








    /// Reprojection Functions
    // points in world frame, model in world frame, bv in model frame
    Eigen::VectorXd reprojectErrPointsVsBV(const Pose& model, const Points3d& points,const Bearings& bv, const BEARING_ERROR method = BVERR_DEFAULT);
    // Same as above but points are already in frame of bv
    Eigen::VectorXd reprojectErrPointsVsBV(const Points3d& points, const Bearings& bv, const BEARING_ERROR method = BVERR_DEFAULT);
    // Computes an error given an angular difference (between two bearing vectors)
    double angle2error(const double angleDeg, const BEARING_ERROR method = BVERR_DEFAULT );
    // Compute the error between bearing vectors
    double errorBV(const Bearing& bv1, const Bearing& bv2, const BEARING_ERROR method = BVERR_DEFAULT );
    // returns the error given px dist on image plane with focal length f
    double px2error(const double px, const double horiFovDeg = 110., const double width = 720, const BEARING_ERROR method = BVERR_DEFAULT);
    // returns the median of a list of values



    /// Utility Functions
    // Return approximate median of a list of values. Note: may change the input vector!!
    template <typename T> T medianApprox(std::vector<T>& values){
        uint middle = values.size() / 2;
        nth_element(values.begin(), values.begin()+middle, values.end()); //fancy median sort pivot search
        return values[middle];
    }
    // Applies the transformation to the 3d points
    void transformPoints(const Pose& transform, Points3d& points);
    // computes the relative rotation of a camera between two imu measurements
    void relativeRotation(const Eigen::Matrix3d& ImuRotFrom, const Eigen::Matrix3d& ImuRotTo, Eigen::Matrix3d& rotRelative);



    /// Vector / MAT Alignment functions
    // Extracts query and train ids into two seperate vectors
    void match2ind(const DMatches& ms, Ints& query, Ints& train);
    // Removes rows of Mat if index is not in ind. Can also change the order of elements.
    void matReduceInd (const cv::Mat& matIn, cv::Mat& matOut, const Ints& ind);
    void matReduceInd (cv::Mat& matInOut, const Ints& ind);
    void matReduceInd (const Eigen::MatrixXd& bvm1, Bearings& bv1, const Ints& ind);
    // align bearing vectors
    void alignedBV (const Eigen::MatrixXd& bvm1, const Eigen::MatrixXd& bvm2, const DMatches& ms, Bearings& bv1, Bearings& bv2);
    // aligns two vectors using matches
    template <class T, class T2> void vecAlignMatch (const T& vec1in, const T2& vec2in,
                                                     T& vec1out,      T2& vec2out,
                                                     const DMatches& ms){
        vec1out.clear();
        vec2out.clear();
        vec1out.reserve(ms.size());
        vec2out.reserve(ms.size());
        for(uint i=0;i<ms.size(); ++i){
            vec1out.push_back(vec1in[ms[i].queryIdx]);
            vec2out.push_back(vec2in[ms[i].trainIdx]);
        }
    }
    //reduce vector using only indicies. eg [z a b c d e f],[1 2 5 3] -> [a b e c]
    template <class T> void vecReduceInd (const T& vecIn, T& vecOut, const Ints& ind){
        vecOut.clear();
        vecOut.reserve(ind.size());
        for(uint i=0;i<ind.size(); ++i){
            vecOut.push_back(vecIn[ind[i]]);
        }
    }
    // same as above but replaces contents
    template <class T> void vecReduceInd (T& vecInOut, const Ints& ind){
        T temp;
        temp.reserve(ind.size());
        for(uint i=0;i<ind.size(); ++i){
            temp.push_back(vecInOut[ind[i]]);
        }
        std::swap(vecInOut, temp);
    }
}

#endif // AUX_HPP
