#ifndef CONTACT_MODEL_GRAPH_H
#define CONTACT_MODEL_GRAPH_H

#include <vector>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <memory>
#include <fcl/collision.h>
#include <fcl/shape/geometric_shapes.h>
#include <fcl/shape/geometric_shapes_utility.h>

#include "contact_motion_planner/planning_scene.h"
#include "robot_dynamics/robot_dynamics_model.h"

#ifndef MAX_NODE
#define MAX_NODE 10000
#endif

namespace suhan_contact_planner
{

template <class T>
class ContactModelGraph
{
  std::vector<int> adjacency_list_[MAX_NODE];
  std::vector<std::shared_ptr<T> > contact_models_;
  std::vector<std::shared_ptr<T> > result_;

  PlanningScenePtr planning_scene_;
  RobotDynamicsModelPtr robot_dynamics_model_;

  int max_depth_;
  double discrete_resolution_;

public:

  ContactModelGraph() : discrete_resolution_(0.05) {}

  void setStart(const std::shared_ptr<T> &start);
  void setGoal(const std::shared_ptr<T> &goal);
  void makeGraph()
  {
    contact_models_.clear();
    contact_models_.push_back(start_);

    makeTree(0, 0);
  }

private:
  void makeTree(int current_index, int depth)
  {
    auto current_node = contact_models_[current_index];

    for(int dir=ContactModel::DIR_X; dir<ContactModel::DIR_YAW; dir++) // Test every direction
    {
      auto new_node = std::make_shared<T>(*current_node); // Creation of new node
      if(new_node->operate(static_cast<ContactModel::OperationDirection>(dir), discrete_resolution_)) // If operation is available + operate it
      {
        bool isSamePoseDetected = false;
        for(auto it=contact_models_.begin(); it!=contact_models_.end(); it++) //
        {
          if (new_node->isSamePose(*(*it),0.01))
          {
            isSamePoseDetected = true;
            int index = std::distance(contact_models_.begin(), it);
            for(int connected_index : adjacency_list_[index])
            {
              adjacency_list_[connected_index].push_back(current_index);
              adjacency_list_[current_index].push_back(connected_index);
            }
          }
        }
        if(!isSamePoseDetected)
        {
          planning_scene_->setTargetObject(new_node); // set planning scene to check whether it is possible
          if (planning_scene_->isPossible() && robot_dynamics_model_->isReachable(new_node->getPosition())) // if feasible point
          {
            int index = contact_models_.size();
            contact_models_.push_back(new_node);
            adjacency_list_[index].push_back(current_index);
            adjacency_list_[current_index].push_back(index);

            makeTree(index, depth+1); // Do this at next node
          }
        }
      }
    }
  }

  std::shared_ptr<T> start_; // Initial contact information should be provided.
  std::shared_ptr<T> goal_; // Goal contact information should be provided.

};

}
#endif
