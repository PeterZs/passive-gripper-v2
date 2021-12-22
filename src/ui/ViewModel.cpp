#include "ViewModel.h"

#include <igl/readSTL.h>
#include <igl/writeSTL.h>
#include <fstream>

#include "../core/GeometryUtils.h"
#include "../core/Initialization.h"
#include "../core/SweptVolume.h"
#include "../core/TopoOpt.h"
#include "../core/robots/Robots.h"

namespace psg {
namespace ui {

ViewModel::ViewModel() {
  using namespace std::placeholders;
  psg_.RegisterInvalidatedDelegate(
      std::bind(&ViewModel::OnPsgInvalidated, this, _1));

  SetCurrentPose(kInitPose);
}

void ViewModel::SetMesh(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F) {
  Eigen::Vector3d minimum = V.colwise().minCoeff();
  Eigen::Vector3d maximum = V.colwise().maxCoeff();

  Eigen::Vector3d translate(
      -minimum.x() / 2. - maximum.x() / 2., -minimum.y(), 0.07 - minimum.z());

  Eigen::MatrixXd SV = V.rowwise() + translate.transpose();
  Eigen::Translation3d mesh_trans(
      (SV.colwise().minCoeff() + SV.colwise().maxCoeff()) / 2.);

  Eigen::Affine3d trans = robots::Forward(current_pose_);
  SV = (trans * (SV.transpose().colwise().homogeneous())).transpose();

  double min_y = SV.colwise().minCoeff().y();
  SV.col(1).array() -= min_y;

  psg_.SetMesh(SV, F);
  psg_.SetMeshTrans(Eigen::Translation3d(0, -min_y, 0) * trans * mesh_trans);
}

void ViewModel::SetCurrentPose(const Pose& pose) {
  current_pose_ = pose;
  Eigen::Affine3d trans = robots::Forward(current_pose_);
  eff_position_ = trans.translation();
  eff_angles_ = trans.linear().eulerAngles(1, 0, 2);
  std::swap(eff_angles_(1), eff_angles_(0));
  ComputeIK();
  PoseChanged();
}

void ViewModel::SetCurrentPose(const Eigen::Affine3d& trans) {
  eff_position_ = trans.translation();
  eff_angles_ = trans.linear().eulerAngles(1, 0, 2);
  std::swap(eff_angles_(1), eff_angles_(0));
  ComputeIK();
  PoseChanged();
}

void ViewModel::SetCurrentPose(const Eigen::Vector3d& pos,
                               const Eigen::Vector3d& ang) {
  eff_position_ = pos;
  eff_angles_ = ang;
  ComputeIK();
  PoseChanged();
}

void ViewModel::TogglePose() {
  if (ik_sols_index_ != -1) {
    ik_sols_index_ = (ik_sols_index_ + 1) % ik_sols_.size();
    current_pose_ = ik_sols_[ik_sols_index_];
    PoseChanged();
  }
}

void ViewModel::AnimateTo(const Pose& pose) {
  src_pose_ = current_pose_;
  dst_pose_ = FixAngles(current_pose_, pose);
  is_animating_ = true;
  cur_step_ = 0;
}

void ViewModel::NextFrame() {
  if (!is_animating_) return;
  cur_step_++;
  double t = (double)cur_step_ / kAnimationSteps;
  Pose p;
  for (size_t i = 0; i < kNumDOFs; i++) {
    p[i] = (1 - t) * src_pose_[i] + t * dst_pose_[i];
  }
  SetCurrentPose(p);
  if (cur_step_ == kAnimationSteps) is_animating_ = false;
}

void ViewModel::ComputeNegativeVolume() {
  if (!is_neg_valid_) {
    NegativeSweptVolumePSG(psg_, neg_V_, neg_F_);
    is_neg_valid_ = true;
    InvokeLayerInvalidated(Layer::kNegVol);
  }
}

void ViewModel::LoadResultBin(const std::string& filename) {
  psg::core::LoadResultBin(psg_, filename, gripper_V_, gripper_F_);
  InvokeLayerInvalidated(Layer::kGripper);
}

void ViewModel::RefineGripper() {
  Eigen::MatrixXd V = gripper_V_;
  Eigen::MatrixXi F = gripper_F_;
  ComputeNegativeVolume();
  psg::core::RefineGripper(psg_, V, F, neg_V_, neg_F_, gripper_V_, gripper_F_);
  InvokeLayerInvalidated(Layer::kGripper);
}

bool ViewModel::LoadGripper(const std::string& filename) {
  std::ifstream f(filename, std::ios::in | std::ios::binary);
  if (!f.is_open()) return false;
  Eigen::MatrixXd N;
  igl::readSTL(f, gripper_V_, gripper_F_, N);
  InvokeLayerInvalidated(Layer::kGripper);
  return true;
}

bool ViewModel::SaveGripper(const std::string& filename) {
  return igl::writeSTL(
      filename, gripper_V_, gripper_F_, igl::FileEncoding::Binary);
}

void ViewModel::ComputeIK() {
  Eigen::Affine3d trans =
      Eigen::Translation3d(eff_position_) *
      Eigen::AngleAxisd(eff_angles_(1), Eigen::Vector3d::UnitY()) *
      Eigen::AngleAxisd(eff_angles_(0), Eigen::Vector3d::UnitX()) *
      Eigen::AngleAxisd(eff_angles_(2), Eigen::Vector3d::UnitZ());

  if (robots::Inverse(trans, ik_sols_)) {
    // Find solution closest to previous solution
    const Pose& prev = current_pose_;
    size_t best = 0;
    double diff = std::numeric_limits<double>::max();
    for (size_t i = 0; i < ik_sols_.size(); i++) {
      double curDiff = SumSquaredAngularDistance(prev, ik_sols_[i]);
      if (curDiff < diff) {
        diff = curDiff;
        best = i;
      }
    }
    current_pose_ = ik_sols_[best];
    ik_sols_index_ = best;
  } else {
    ik_sols_index_ = -1;
  }
}

void ViewModel::InvokeLayerInvalidated(Layer layer) {
  if (LayerInvalidated_) LayerInvalidated_(layer);
}

void ViewModel::OnPsgInvalidated(PassiveGripper::InvalidatedReason reason) {
  switch (reason) {
    case PassiveGripper::InvalidatedReason::kMesh:
      InvokeLayerInvalidated(Layer::kMesh);
      InvokeLayerInvalidated(Layer::kCenterOfMass);
      break;
    case PassiveGripper::InvalidatedReason::kContactPoints:
      InvokeLayerInvalidated(Layer::kContactPoints);
      is_init_params_valid_ = false;
      InvokeLayerInvalidated(Layer::kInitFingers);
      InvokeLayerInvalidated(Layer::kInitTrajectory);
      break;
    case PassiveGripper::InvalidatedReason::kFingers:
      InvokeLayerInvalidated(Layer::kFingers);
      InvokeLayerInvalidated(Layer::kSweptSurface);
      break;
    case PassiveGripper::InvalidatedReason::kTrajectory:
      InvokeLayerInvalidated(Layer::kTrajectory);
      InvokeLayerInvalidated(Layer::kSweptSurface);
      break;
    case PassiveGripper::InvalidatedReason::kTopoOptSettings:
      InvokeLayerInvalidated(Layer::kGripperBound);
      is_neg_valid_ = false;
      InvokeLayerInvalidated(Layer::kNegVol);
      break;
    case PassiveGripper::InvalidatedReason::kCost:
      InvokeLayerInvalidated(Layer::kGradient);
      break;
  }
}

bool ViewModel::ComputeInitParams() {
  Eigen::Vector3d effector_pos =
      robots::Forward(psg_.GetTrajectory().front()).translation();
  init_params_.fingers.resize(psg_.GetContactPoints().size());
  for (size_t i = 0; i < psg_.GetContactPoints().size(); i++) {
    init_params_.fingers[i] =
        InitializeFinger(psg_.GetContactPoints()[i],
                         psg_.GetMDR(),
                         effector_pos,
                         psg_.GetFingerSettings().n_finger_joints);
  }
  init_params_.trajectory =
      InitializeTrajectory(init_params_.fingers,
                           psg_.GetTrajectory().front(),
                           psg_.GetTrajectorySettings().n_keyframes);
  init_params_.contact_points = psg_.GetContactPoints();
  is_init_params_valid_ = true;

  InvokeLayerInvalidated(Layer::kInitFingers);
  InvokeLayerInvalidated(Layer::kInitTrajectory);
}

void ViewModel::PoseChanged() {
  InvokeLayerInvalidated(Layer::kFingers);
  InvokeLayerInvalidated(Layer::kRobot);
  InvokeLayerInvalidated(Layer::kGripperBound);
  InvokeLayerInvalidated(Layer::kNegVol);
  InvokeLayerInvalidated(Layer::kGripper);
  InvokeLayerInvalidated(Layer::kGradient);
}

}  // namespace ui
}  // namespace psg
