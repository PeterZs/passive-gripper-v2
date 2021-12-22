#pragma once

#include "../Constants.h"
#include "models/GripperParams.h"
#include "models/GripperSettings.h"
#include "models/MeshDependentResource.h"

namespace psg {
namespace core {

using namespace models;

double EvalAt(const Eigen::Vector3d& p,
              const CostSettings& settings,
              const MeshDependentResource& mdr,
              Eigen::RowVector3d& out_dc_dp);

double ComputeCost(const GripperParams& params,
                   const GripperSettings& settings,
                   const MeshDependentResource& mdr,
                   std::vector<Eigen::MatrixXd>& out_dCost_dFinger);

double MinDistance(const GripperParams& params,
                   const GripperSettings& settings,
                   const MeshDependentResource& mdr);

bool Intersects(const GripperParams& params,
                const GripperSettings& settings,
                const MeshDependentResource& mdr);

}  // namespace core
}  // namespace psg
