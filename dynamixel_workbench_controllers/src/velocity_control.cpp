/*******************************************************************************
* Copyright 2016 ROBOTIS CO., LTD.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/* Authors: Taehoon Lim (Darby) */

#include "dynamixel_workbench_controllers/velocity_control.h"

VelocityControl::VelocityControl()
    :node_handle_(""),
     dxl_cnt_(2)
{
  std::string device_name   = node_handle_.param<std::string>("device_name", "/dev/ttyUSB0");
  uint32_t dxl_baud_rate    = node_handle_.param<int>("baud_rate", 1000000);

  uint32_t profile_velocity     = node_handle_.param<int>("profile_velocity", 200);
  uint32_t profile_acceleration = node_handle_.param<int>("profile_acceleration", 50);

  dxl_id_[0] = node_handle_.param<int>("left_wheel", 1);
  dxl_id_[1] = node_handle_.param<int>("right_wheel", 2);

  dxl_wb_ = new DynamixelWorkbench;

  dxl_wb_->begin(device_name.c_str(), dxl_baud_rate);

  for (int index = 0; index < dxl_cnt_; index++)
  {
    uint16_t get_model_number;
    if (dxl_wb_->ping(dxl_id_[index], &get_model_number) != true)
    {
      ROS_ERROR("Not found Motors, Please check id and baud rate");

      ros::shutdown();
      return;
    }
  }

  initMsg();

  for (int index = 0; index < dxl_cnt_; index++)
    dxl_wb_->wheelMode(dxl_id_[index], profile_velocity, profile_acceleration);

  dxl_wb_->addSyncWrite("Moving_Speed");

  initPublisher();
  initServer();
}

VelocityControl::~VelocityControl()
{
  for (int index = 0; index < 2; index++)
    dxl_wb_->itemWrite(dxl_id_[index], "Torque_Enable", 0);

  ros::shutdown();
}

void VelocityControl::initMsg()
{
  printf("-----------------------------------------------------------------------\n");
  printf("  dynamixel_workbench controller; velocity control example             \n");
  printf("-----------------------------------------------------------------------\n");
  printf("\n");

  for (int index = 0; index < dxl_cnt_; index++)
  {
    printf("MODEL   : %s\n", dxl_wb_->getModelName(dxl_id_[index]));
    printf("ID      : %d\n", dxl_id_[index]);
    printf("INDEX      : %d\n", index);
    printf("\n");
  }
  printf("-----------------------------------------------------------------------\n");
}

void VelocityControl::initPublisher()
{
  dynamixel_state_list_pub_ = node_handle_.advertise<dynamixel_workbench_msgs::DynamixelStateList>("dynamixel_state", 10);
}

void VelocityControl::initServer()
{
  wheel_command_server_ = node_handle_.advertiseService("wheel_command", &VelocityControl::wheelCommandMsgCallback, this);
}

void VelocityControl::dynamixelStatePublish()
{
  dynamixel_workbench_msgs::DynamixelState     dynamixel_state[dxl_cnt_];
  dynamixel_workbench_msgs::DynamixelStateList dynamixel_state_list;

  for (int index = 0; index < dxl_cnt_; index++)
  {
    dynamixel_state[index].model_name          = std::string(dxl_wb_->getModelName(dxl_id_[index]));
    dynamixel_state[index].id                  = dxl_id_[index];
    dynamixel_state[index].torque_enable       = dxl_wb_->itemRead(dxl_id_[index], "Torque_Enable");
    dynamixel_state[index].present_position    = dxl_wb_->itemRead(dxl_id_[index], "Present_Position");
    dynamixel_state[index].present_velocity    = dxl_wb_->itemRead(dxl_id_[index], "Present_Speed");
    dynamixel_state[index].goal_position       = dxl_wb_->itemRead(dxl_id_[index], "Goal_Position");
    dynamixel_state[index].goal_velocity       = dxl_wb_->itemRead(dxl_id_[index], "Moving_Speed");
    dynamixel_state[index].moving              = dxl_wb_->itemRead(dxl_id_[index], "Moving");

    dynamixel_state_list.dynamixel_state.push_back(dynamixel_state[index]);
  }
  dynamixel_state_list_pub_.publish(dynamixel_state_list);
}

void VelocityControl::controlLoop()
{
  dynamixelStatePublish();
}

bool VelocityControl::wheelCommandMsgCallback(dynamixel_workbench_msgs::WheelCommand::Request &req,
                                              dynamixel_workbench_msgs::WheelCommand::Response &res)
{
  static int32_t goal_velocity[2] = {0, 0};

  goal_velocity[0] = req.left_vel;//dxl_wb_->convertVelocity2Value(dxl_id_[0], req.left_vel);
  goal_velocity[1] = req.right_vel;dxl_wb_->convertVelocity2Value(dxl_id_[1], (-1) * req.right_vel);
  printf("goal0      : %d\n", goal_velocity[0]);
  printf("ID      : %d\n", goal_velocity[1]);

  //dxl_wb_->itemWrite(dxl_id_[0], "Moving_Speed", goal_velocity[0]);
  //dxl_wb_->itemWrite(dxl_id_[1], "Moving_Speed", goal_velocity[1]);
  printf("caccaaaaaaaaaa\n");

  bool ret = dxl_wb_->syncWrite("Moving_Speed", goal_velocity);

  res.result = ret;
}

int main(int argc, char **argv)
{
  // Init ROS node
  ros::init(argc, argv, "velocity_control");
  VelocityControl vel_ctrl;

  ros::Rate loop_rate(250);

  while (ros::ok())
  {
    vel_ctrl.controlLoop();
    ros::spinOnce();
    loop_rate.sleep();
  }

  return 0;
}
