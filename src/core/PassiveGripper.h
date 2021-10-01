#pragma once

#include <Eigen/Core>
#include <functional>
#include <vector>

#include "models/ContactPoint.h"
#include "models/GripperParams.h"
#include "models/GripperSettings.h"
#include "models/MeshDependentResource.h"

namespace psg {
namespace core {

using namespace models;

class PassiveGripper {
 public:
  enum class InvalidatedReason { kMesh, kContactPoints, kFingers, kTrajectory };
  typedef std::function<void(InvalidatedReason)> InvalidatedDelegate;

  PassiveGripper();

  inline void RegisterInvalidatedDelegate(const InvalidatedDelegate& d) {
    Invalidated_ = d;
  }

  // Mesh
  void SetMesh(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F);
  inline void GetMesh(Eigen::MatrixXd& V, Eigen::MatrixXi& F) const {
    V = mdr_.V;
    F = mdr_.F;
  }
  void SetMeshTrans(const Eigen::Affine3d& trans);
  void TransformMesh(const Eigen::Affine3d& trans);

  // Contact Point
  void AddContactPoint(const ContactPoint& contact_point);
  void RemoveContactPoint(size_t index);
  void ClearContactPoint();

  // Trajectory
  void AddKeyframe(const Pose& pose);
  void EditKeyframe(size_t index, const Pose& pose);
  void RemoveKeyframe(size_t index);
  void ClearKeyframe();
  inline const Trajectory& GetTrajectory() const { return params_.trajectory; }

  // Settings
  void SetContactSettings(const ContactSettings& settings);
  void SetFingerSettings(const FingerSettings& finger_settings);
  void SetTrajectorySettings(const TrajectorySettings& finger_settings);
  void SetOptSettings(const OptSettings& finger_settings);
  void SetTopoOptSettings(const TopoOptSettings& finger_settings);
  void SetCostSettings(const CostSettings& finger_settings);

  bool reinit_trajectory = true;

 private:
  GripperParams params_;
  GripperSettings settings_;
  MeshDependentResource mdr_;
  std::vector<ContactPoint> contact_cones_;
  Eigen::Affine3d mesh_trans_;

  bool mesh_loaded_ = false;

  bool is_force_closure_;
  bool is_partial_closure_;
  double min_wrench_;
  double partial_min_wrench_;
  double cost_;

  // state dependency
  bool mesh_changed_ = false;
  bool contact_settings_changed_ = false;
  bool finger_settings_changed_ = false;
  bool trajectory_settings_changed_ = false;
  bool opt_settings_changed_ = false;
  bool topo_opt_settings_changed_ = false;
  bool cost_settings_changed_ = false;
  bool contact_changed_ = false;
  bool finger_changed_ = false;
  bool trajectory_changed_ = false;
  bool quality_changed_ = false;
  bool cost_changed_ = false;

  InvalidatedDelegate Invalidated_;

  void Invalidate();
  void ForceInvalidateAll();
  void InvokeInvalidated(InvalidatedReason reason);

  // To be called by Invalidate()
  void InvalidateMesh();
  void InvalidateContactSettings();
  void InvalidateFingerSettings();
  void InvalidateTrajectorySettings();
  void InvalidateCostSettings();
  void InvalidateContact();
  void InvalidateFinger();
  void InvalidateTrajectory();
  void InvalidateQuality();
  void InvalidateCost();

 public:
  DECLARE_GETTER(GetCenterOfMass, mdr_.center_of_mass)
  DECLARE_GETTER(GetMeshTrans, mesh_trans_)
  DECLARE_GETTER(IsMeshLoaded, mesh_loaded_)
  DECLARE_GETTER(GetContactPoints, params_.contact_points)
  DECLARE_GETTER(GetContactCones, contact_cones_)
  DECLARE_GETTER(GetFingers, params_.fingers)
  DECLARE_GETTER(GetContactSettings, settings_.contact)
  DECLARE_GETTER(GetFingerSettings, settings_.finger)
  DECLARE_GETTER(GetTrajectorySettings, settings_.trajectory)
  DECLARE_GETTER(GetOptSettings, settings_.opt)
  DECLARE_GETTER(GetTopoOptSettings, settings_.topo_opt)
  DECLARE_GETTER(GetCostSettings, settings_.cost)
  DECLARE_GETTER(GetIsForceClosure, is_force_closure_)
  DECLARE_GETTER(GetIsPartialClosure, is_partial_closure_)
  DECLARE_GETTER(GetMinWrench, min_wrench_)
  DECLARE_GETTER(GetPartialMinWrench, partial_min_wrench_)
  DECLARE_GETTER(GetCost, cost_)
  DECLARE_GETTER(GetParams, params_)
  DECLARE_GETTER(GetSettings, settings_)
  DECLARE_GETTER(GetMDR, mdr_)
};

}  // namespace core
}  // namespace psg