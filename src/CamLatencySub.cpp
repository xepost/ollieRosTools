
#include <ros/ros.h>
#include <ollieRosTools/CamLatencySub.hpp>
#include <sensor_msgs/image_encodings.h>
#include <boost/bind.hpp>
#include <cmath>
#include <opencv2/core/core.hpp>
#include <boost/lexical_cast.hpp> //boost::lexical_cast<std::string>(i);
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <cv_bridge/cv_bridge.h>

using namespace boost::accumulators;


/// Initialise ROS Node
CamLatencySub::CamLatencySub(ros::NodeHandle& _n):
    n(_n),
    imTransport(_n),
    preMean(0)
    {   

    ROS_INFO("Starting CamLatencySub node\nAvailable params:\n_image:=/image_raw\n_lag=8.8");

    subSequence = n.subscribe<std_msgs::Header>("/camLatency", 1, &CamLatencySub::incomingSequence, this);
    /// Publish to Topics
    pubImage   = imTransport.advertise("/camLatency/image_raw", 1);
    n.param("image", inputTopic, std::string("/image_raw"));
    n.param("lag", monitorLag, 8.8); // monitor input lag in ms
    ROS_INFO("Starting <%s> node with <%s> as image source and <%.2f> input lag", /*ros::this_node::getNamespace().c_str(),*/ ros::this_node::getName().c_str(), inputTopic.c_str(), monitorLag);



    /// Subscribe to Topics
    subImage = imTransport.subscribe(inputTopic, 1, &CamLatencySub::incomingImage, this);
    reset();
}


CamLatencySub::~CamLatencySub() {
    ROS_INFO("Shutting <%s> node down", ros::this_node::getName().c_str());
}


void CamLatencySub::incomingSequence(const std_msgs::HeaderConstPtr& msg){
    if (static_cast<int>(msg->seq)<=imageSentSeq){
        reset();
    }
    imageSentTime = msg->stamp;
    imageSentSeq = msg->seq;
}

void CamLatencySub::reset(){
    ROS_INFO("RESET");
    imageSentSeq = 0;
    imageSentTime = ros::Time::now();
    tReceived = imageSentTime;
    delays.clear();
}


void CamLatencySub::incomingImage(const sensor_msgs::ImageConstPtr& msg){
    /// Get Image
    cv_bridge::CvImageConstPtr cvPtr;
    try {
        cvPtr = cv_bridge::toCvShare(msg, sensor_msgs::image_encodings::MONO8);
    } catch (cv_bridge::Exception& e) {
        ROS_ERROR_STREAM_THROTTLE(1,"Failed to understand incoming image:" << e.what());
        return;
    }



    cv::Mat img = cvPtr->image;

    int m = static_cast<int>(0.5+cv::mean(img.col(0.5+img.cols/2))[0]);
    /// publish debug image
    if (abs(preMean-m)>100){
        std::string s = boost::lexical_cast<std::string>(m);
        cv::putText(img, s, cv::Point(20,25), cv::FONT_HERSHEY_PLAIN, 2, CV_RGB(0,0,0));
        cv::putText(img, s, cv::Point(21,26), cv::FONT_HERSHEY_PLAIN, 2, CV_RGB(255,255,255));


        tReceived = msg->header.stamp;
        cv_bridge::CvImage cvi;
        cvi.header = msg->header;
        cvi.encoding = "mono8";
        cvi.image = img;
        pubImage.publish(cvi.toImageMsg());
        delays.push_back((tReceived-imageSentTime).toSec()*1000 - monitorLag);

        accumulator_set<double, stats<tag::variance> > acc;
        std::for_each(delays.begin(), delays.end(), boost::bind<void>(boost::ref(acc), _1));

        ROS_INFO("[%d] Mean: %.2f   Std: %.2f   Raw: %.2f   Samples: %3lu)", imageSentSeq, mean(acc), sqrt(variance(acc)), delays.back(), delays.size() );
    }
    preMean = m;

}



//Initialize ROS node.
int main(int argc, char** argv){

    ros::init(argc, argv, "CamLatencySub");
    ros::NodeHandle n("~");

    CamLatencySub node(n);

    ros::spin();
    return 0;
}

