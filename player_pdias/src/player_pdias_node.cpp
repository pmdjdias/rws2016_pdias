#include <iostream>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <ros/ros.h>
#include <tf/transform_listener.h>
#include <tf/transform_broadcaster.h>

#define RED 0
#define GREEN 1
#define BLUE 2

using namespace std;

namespace rws2016_pdias
{

/***************************************************************
 *
 * Player
 *
 **************************************************************/
class Player
{
public:
    //Constructor with the same name as the class
    Player(string name) {this->name = name;}

    int setTeamName(int team_index = 0 /*default value*/)
    {
        switch (team_index)
        {
        case 0:
            return setTeamName("red"); break;
        case 1:
            return setTeamName("green"); break;
        case 2:
            return setTeamName("blue");  break;
        default:
            cout << "wrong team index given. Cannot set team" << endl; break;
        }
    }

    //Set team name, if given a correct team name (accessor)
    int setTeamName(string team)
    {
        if (team=="red" || team=="green" || team=="blue")
        {
            this->team = team;
            return 1;
        }
        else
        {
            cout << "cannot set team name to " << team << endl;
            return 0;
        }
    }

    //Gets team name (accessor)
    string getTeamName(void) {return team;}

    /**
    * @brief Gets the pose (calls updatePose first)
    *
    * @return the transform
    */
    tf::Transform getPose(void)
    {
        ros::Duration(0.1).sleep(); //To allow the listener to hear messages
        tf::StampedTransform st; //The pose of the player
        try{
            listener.lookupTransform("/map", name, ros::Time(0), st);
        }
        catch (tf::TransformException ex){
            ROS_ERROR("%s",ex.what());
            ros::Duration(1.0).sleep();
        }

        tf::Transform t;
        t.setOrigin(st.getOrigin());
        t.setRotation(st.getRotation());
        return t;
    }

    void getDistanceToAllHunters(void)
    {
        //for hunter in hunters

    }

    double getDistance(Player& p)
    {
        //computing the distance
        string first_refframe = p.name;
        string second_refframe = name;

        ros::Duration(0.1).sleep(); //To allow the listener to hear messages
        tf::StampedTransform st; //The pose of the player
        try{
            listener.lookupTransform(first_refframe, second_refframe, ros::Time(0), st);
        }
        catch (tf::TransformException& ex){
            ROS_ERROR("%s",ex.what());
            ros::Duration(1.0).sleep();
        }

        tf::Transform t;
        t.setOrigin(st.getOrigin());
        t.setRotation(st.getRotation());

        double x = t.getOrigin().x();
        double y = t.getOrigin().y();

        double norm = sqrt(x*x + y*y);
        return norm;

    }

    std::string name;

private:

    std::string team;
    tf::TransformListener listener; //gets transforms from the system
};

/***************************************************************
 *
 * Team
 *
 **************************************************************/
class Team
{
public:

    Team(string team, vector<string>& player_names)
    {
        name = team;

        //Cycle all player names, and create a class player for each
        for (size_t i=0; i < player_names.size(); ++i)
        {
            //Why? Copy constructable ...
            boost::shared_ptr<Player> p(new Player(player_names[i]));
            p->setTeamName(name);
            players.push_back(p);
        }

    }


    void printTeamInfo(void)
    {
        cout << "Team " << name << " has the following players:" << endl;

        for (size_t i=0; i < players.size(); ++i)
            //cout << players[i]->name << endl;
            //cout << players.at(i).name << endl;
            cout << players[i]->name << endl;
    }

    std::string name;

    vector<boost::shared_ptr<Player> > players;
};


/***************************************************************
 *
 * Class MyPlayer extends class Player
 *
 **************************************************************/
class MyPlayer: public Player
{
public:

    boost::shared_ptr<Team> my_team;
    boost::shared_ptr<Team> prey_team;
    boost::shared_ptr<Team> hunter_team;

    MyPlayer(std::string name, std::string team = "blue"): Player(name)
    {
        setTeamName(team);

        //Initialize position to 0,0,0
        tf::Transform t;
        t.setOrigin( tf::Vector3(0.0, 0.0, 0.0) );
        tf::Quaternion q; q.setRPY(0, 0, 0);
        t.setRotation(q);
        br.sendTransform(tf::StampedTransform(t, ros::Time::now(), "/map", name));

        // set prey team
        vector<string> prey_names;
        prey_names.push_back("lalmeida");
        prey_team = (boost::shared_ptr<Team>) new Team("green", prey_names);
    }

    /**
     * @brief Moves MyPlayer
     *
     * @param displacement the linear movement of the player, bounded by [-0.1, 1]
     * @param turn_angle the turn angle of the player, bounded by  [-M_PI/60, M_PI/60]
     */
     void move(double displacement, double turn_angle)
     {

         //Put arguments withing authorized boundaries
         double max_d =  1;
         displacement = (displacement > max_d ? max_d : displacement);

         double min_d =  -0.1;
         displacement = (displacement > min_d ? min_d : displacement);

         double max_t =  (M_PI/60);
         if (turn_angle > max_t)
             turn_angle = max_t;
         else if (turn_angle < -max_t)
             turn_angle = -max_t;

         //Compute the new reference frame
         tf::Transform t_mov;
         t_mov.setOrigin( tf::Vector3(displacement , 0, 0.0) );
         tf::Quaternion q;
         q.setRPY(0, 0, turn_angle);
         t_mov.setRotation(q);

         tf::Transform t = getPose();
         t = t  * t_mov;

         //Send the new transform to ROS
         br.sendTransform(tf::StampedTransform(t, ros::Time::now(), "/map", name));
     }

     /*double getDistance(std::string playername)
     {
         tf::Transform myPose = this->getPose();

     }*/


private:
     tf::TransformBroadcaster br; //publish the transform
};


} // end namespace rws2016_pdias

int main(int argc, char** argv)
{
    //initialize ROS stuff
    ros::init(argc, argv, "pdias_node");
    ros::NodeHandle node;

    //Creating an instance of class MyPlayer
    rws2016_pdias::MyPlayer my_player("pdias", "blue");

    vector<string> myteam_names;
    myteam_names.push_back("pdias");
    rws2016_pdias::Team my_team("blue", myteam_names);
    my_team.printTeamInfo();

    vector<string> prey_names;
    prey_names.push_back("lalmeida");
    rws2016_pdias::Team prey_team("red", prey_names);
    prey_team.printTeamInfo();

    rws2016_pdias::Player lalmeida_player("moliveira");

    //Infinite loop
    ros::Rate loop_rate(10);
    while (ros::ok())
    {
        //Test the get pose method
        tf::Transform t = my_player.getPose();
        cout << "x = " << t.getOrigin().x() << " y = " << t.getOrigin().y() << endl;

        double dist_from_my_player_to_moliveira = my_player.getDistance(lalmeida_player);
        cout << "dist_from_my_player_to_moliveira= " << dist_from_my_player_to_moliveira << endl;

        //my_player.move(0.1, M_PI/60);
        my_player.move(0.1, -M_PI/6);

        ros::spinOnce();
        loop_rate.sleep();

        // PMD
        //my_player.move(0.1, M_PI/60);
        //ros::spinOnce();
        //loop_rate.sleep();

    }

    return 1;
}
