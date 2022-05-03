#include "ros/ros.h"
#include "sensor_msgs/JointState.h"
#include "geometry_msgs/TwistStamped.h"
#include <math.h>

class ComputeVelocity {

public:

	ComputeVelocity(){
		this->sensorInput=this->n.subscribe("/wheel_states", 1000, &ComputeVelocity::sensorCallback,this);
		this->velocitiesPub=this->n.advertise<geometry_msgs::TwistStamped>("/cmd_vel", 1000);

        this->ticfl0=0.0;
        this->ticfr0=0.0;
        this->ticrr0=0.0;
        this->ticrl0=0.0;
        this->ts0.sec = 0;
        this->ts0.nsec = 0;
        n.getParam("/gearRatio", this->gearRatio);
        n.getParam("/wheelRadius", this->wheelRadius);
        n.getParam("/halfLenght", this->halfLength);
        n.getParam("/halfWidth", this->halfWidth);
        n.getParam("/tickRes", this->tickResolution);
	}

    void mainLoop(){
        ROS_INFO("Geometry node started\n");

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
        return msg->header.stamp.sec + msg->header.stamp.nsec * pow(10, -9);
    }
    
    void sensorCallback(const sensor_msgs::JointState::ConstPtr& msg){
        
        if(msgCount%msgInterval == 0){
            // The velocities are computed every msgInterval number of messages received
            // to reduce noise

            if(msgCount == 0) {
                // When the first message is received, the first values are saved but
                // the velocities are not computed

                ts0 = msg->header.stamp;

                ticfl0 = msg->position[0]/gearRatio;
                ticfr0 = msg->position[1]/gearRatio;
                ticrl0 = msg->position[2]/gearRatio;
                ticrr0 = msg->position[3]/gearRatio;
            }
            else {
                // Starting from the second iteration, velocities are computed and published

                ts = msg->header.stamp;
                deltats = ts - ts0;
                ROS_INFO("deltats: %lf", deltats.toSec());
                ROS_INFO("msg count: %d", msgCount);

                // front-left wheel
                ticfl = msg->position[0]/gearRatio;
                wfl = (ticfl - ticfl0)/deltats.toSec()*2*M_PI/tickResolution;
                ROS_INFO("deltatic: %d", ticfl-ticfl0);
                ROS_INFO("Rot fl: %lf", wfl);
                ticfl0 = ticfl;

                // front-right wheel
                ticfr = msg->position[1]/gearRatio;
                wfr = (ticfr - ticfr0)/deltats.toSec()*2*M_PI/tickResolution;
                ROS_INFO("deltatic: %d", ticfr-ticfr0);
                ROS_INFO("Rot fr: %lf", wfr);
                ticfr0 = ticfr;

                // rear-left wheel
                ticrl = msg->position[2]/gearRatio;
                wrl = (ticrl - ticrl0)/deltats.toSec()*2*M_PI/tickResolution;
                ROS_INFO("deltatic: %d", ticrl-ticrl0);
                ROS_INFO("Rot rl: %lf", wrl);
                ticrl0 = ticrl;

                // rear-right wheel
                ticrr = msg->position[3]/gearRatio;
                wrr = (ticrr - ticrr0)/deltats.toSec()*2*M_PI/tickResolution;
                ROS_INFO("deltatic: %d", ticrr-ticrr0);
                ROS_INFO("Rot rr: %lf", wrr);
                ticrr0 = ticrr;

                ts0 = ts;

                // robot velocities
                vx = (wfl+wfr+wrr+wrl)*wheelRadius/4;
                ROS_INFO("Vel x: %lf", vx);
                vy = (-wfl+wfr+wrl-wrr)*wheelRadius/4;
                ROS_INFO("Vel y: %lf", vy);
                wz = (-wfl+wfr-wrl+wrr)*wheelRadius/(4*(wheelRadius+halfLength));
                ROS_INFO("W z: %lf", wz);

                // publishing cmd_vel message
                geometry_msgs::TwistStamped velMsg;
                velMsg.header.stamp.sec = msg->header.stamp.sec;
                velMsg.header.stamp.nsec = msg -> header.stamp.nsec;
                velMsg.twist.linear.x = this -> vx;
                velMsg.twist.linear.y = this -> vy;
                velMsg.twist.angular.z = this -> wz;
                this -> velocitiesPub.publish(velMsg);
            }
            
        }

        ++msgCount;

    }

private:

	ros::NodeHandle n;
	ros::Subscriber sensorInput;
	ros::Publisher velocitiesPub;
    int gearRatio = 5;
    double wheelRadius = 0.07;
    double halfLength = 0.2;
    double halfWidth = 0.169;
    int tickResolution = 42;
    double wfl, wfr, wrr, wrl;
    double ticfl, ticfr, ticrr, ticrl;
    double ticfl0, ticfr0, ticrr0, ticrl0;
    ros::Time ts, ts0;
    ros::Duration deltats;
    double vx, vy, wz;
    int msgCount = 0;
    const int msgInterval = 5;

};

int main (int argc, char **argv) {
    
	ros::init(argc, argv, "ComputeVelocitiesTick");

    ComputeVelocity compVel;
    compVel.mainLoop();

    return 0;
}