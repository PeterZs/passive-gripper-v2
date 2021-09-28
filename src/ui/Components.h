#pragma once

namespace psg {
namespace ui {

bool MyInputDouble3(const char* name,
                    double* v,
                    double step = 0.001,
                    const char* fmt = "%.4f");

bool MyInputDouble3Convert(const char* name,
                           double* v,
                           double factor = 1,
                           double step = 0.001,
                           const char* fmt = "%.4f");

}  // namespace ui
}  // namespace psg