#include "ros/ros.h"
#include "geometry_msgs/TwistStamped.h"
#include "nav_msgs/Odometry.h"
#include "project_1/Reset.h"
#include <dynamic_reconfigure/server.h>
#include <project_1/parametersConfig.h>

class ComputeOdometry {

public:

    ComputeOdometry() {
        
        this->velInput=this->n.subscribe("/cmd_vel", 1000, &ComputeOdometry::callOdometryMethod, this);
        this->odomPub=this->n.advertise<nav_msgs::Odometry>("/odom", 1000);
        this-> resetService = this->n.advertiseService("reset", &ComputeOdometry::resetCallback, this);
                
        this->x0 = 0.0;
        this->y0 = 0.0;
        this->theta0 = 0.0;
        this->ts0 = 0.0;
        this->set_method = 0;       // Euler integration method is set to default


        // dynamic reconfigure setup
        this->f = boost::bind(&ComputeOdometry::parametersCallback, this, _1, _2);
        this->dynServer.setCallback(f);
    }

    void mainLoop(){
        ROS_INFO("Odometry node started\n");

        ros::spin();
        /*
        ros::Rate loop_rate(10);
        while (ros::ok()) {
            ros::spinOnce();
            loop_rate.sleep();
        }
        */
    }

    double computeTimeStamp(geometry_msgs::TwistStamped::ConstPtr &msg) {
        // function that returns a timestamp variable of type double, from the
        // variables sec and nsec of type uint32_t contained in the header of the message

        return msg->header.stamp.sec + msg->header.stamp.nsec * pow(10, -9);
    }

    void publishOdometry() {
        // function that creates and publishes a nav_msgs::Odometry message on topic /odom

        nav_msgs::Odometry odomMsg;
        odomMsg.pose.pose.position.x = this -> x0;
        odomMsg.pose.pose.position.y = this -> y0;
        odomMsg.pose.pose.orientation.z = this -> theta0;
        this -> odomPub.publish(odomMsg);
    }

    void callOdometryMethod(const geometry_msgs::TwistStamped::ConstPtr &msg) {
        // This function is executed everytime a message is received and calls the
        // correct integration method function, according to the set_method variable

        if(set_method == 0) {
            eulerOdometry(msg);
        }
        else {
            rungeKuttaOdometry(msg);
        }
    }

    void eulerOdometry(const geometry_msgs::TwistStamped::ConstPtr &msg) {
        // 1st order Euler integration, rotation is performed after the translation

        if(ts0 == 0.0) {
            ts0 = msg->header.stamp.sec + msg->header.stamp.nsec * pow(10, -9);
        }
        else {
            ts = msg->header.stamp.sec + msg->header.stamp.nsec * pow(10, -9);
            deltats = ts - ts0;
            theta = theta0 + msg->twist.angular.z * deltats;
            ROS_INFO("Theta: %f", theta);
            x = x0 + deltats * (msg->twist.linear.x * cos (theta) + msg->twist.linear.y * sin (90.0 + theta));
            ROS_INFO("X: %f", x);
            y = y0 + deltats * (msg->twist.linear.x * sin (theta) + msg->twist.linear.y * sin (90.0 + theta));
            ROS_INFO("Y: %f", y);
            theta0 = theta;
            x0 = x;
            y0 = y;
            ts0 = ts;

            // creating and publishing the odometry message
            publishOdometry();
            /*
            nav_msgs::Odometry odomMsg;
            odomMsg.pose.pose.position.x = this -> x;
            odomMsg.pose.pose.position.y = this -> y;
            odomMsg.pose.pose.orientation.z = this -> theta;
            this -> odomPub.publish(odomMsg);
            */
        }
    }

    void rungeKuttaOdometry(const geometry_msgs::TwistStamped::ConstPtr &msg) {
        // 2nd order Runge Kutta integration, theta changes during the translation

        if(ts0 == 0.0) {
            // When the first message is received, the new timestamp is saved but no
            // odometry is performed since we only have the first value of v and omega
            ts0 = msg->header.stamp.sec + msg->header.stamp.nsec * pow(10, -9);
        }
        else {
            // starting from the second message received, the odometry is computed
            ts = msg->header.stamp.sec + msg->header.stamp.nsec * pow(10, -9);
            deltats = ts - ts0;
            theta = theta0 + msg->twist.angular.z * deltats;
            v_k = sqrt(pow(msg->twist.linear.x,2) + pow(msg->twist.linear.y,2));
            x = x0 + v_k * deltats * cos(theta0 + (msg->twist.angular.z * deltats / 2));
            y = y0 + v_k * deltats * sin(theta0 + (msg->twist.angular.z * deltats / 2));

            // updating past values with current values
            theta0 = theta;
            x0 = x;
            y0 = y;
            ts0 = ts;

            // creating and publishing the odometry message
            publishOdometry();
        }
    }

    bool resetCallback(project_1::Reset::Request  &req, project_1::Reset::Response &res) {
        // reset the robot pose to the desired values

        res.x_old = this->x0;
        res.y_old = this->y0;
        res.theta_old = this->theta0;

        this->x0 = req.x_new;
        this->y0 = req.y_new;
        this->theta0 = req.theta_new;

        publishOdometry();

        ROS_INFO("\nOld pose: (%lf,%lf,%lf)\nNew pose: (%lf,%lf,%lf)", 
            res.x_old, res.y_old, res.theta_old, req.x_new, req.y_new, req.theta_new);

        return true;
    }

    void parametersCallback(project_1::parametersConfig &config, uint32_t level) {
        // integration method is changed
        
        if(config.set_method == 0) {  
            this->set_method = 0;       // Euler
            ROS_INFO("Dynamic reconfigure: integration method set to Euler");
        }
        else {
            this->set_method = 1;       // Runge-Kutta
            ROS_INFO("Dynamic reconfigure: integration method set to Runge-Kutta");
        }
}

private:

    ros::NodeHandle n;
	ros::Subscriber velInput;
    ros::Publisher odomPub;
    ros::ServiceServer resetService;

    dynamic_reconfigure::Server<project_1::parametersConfig> dynServer;
    dynamic_reconfigure::Server<project_1::parametersConfig>::CallbackType f;

    double x, y, theta;
    double x0, y0, theta0;
    double ts0;
    double ts;
    double deltats;
    double v_k;             // absolute value of the linear velocity
    int set_method;         // integration method: Euler = 0 , RK = 1
};

int main (int argc, char **argv) {

    ros::init(argc, argv, "ComputeOdometry");

    ComputeOdometry compOdo;
    compOdo.mainLoop();

    return 0;
}