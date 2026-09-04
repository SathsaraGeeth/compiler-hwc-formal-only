/*
 * compiler/btor2/include/Lowering/Formal/system.h
 *
 * Copyright (C) 2026 Sathsara Geeth
 */

/*
 * Version 1.0
 *
 * Version History
 *
 * Version | Description
 * --------+-----------------------------------------
 * 1.0     | Initial implementation
 */

/*
 * Comments:
 *
 * Defines the BTOR2 transition system and its visible signals
 */

#pragma once
#include "Lowering/TwoStateEIRLowering.h"

namespace emul::formal {
using TransitionSystem = btor2::TwoStateTransitionSystem;
using Btor2Value = btor2::Btor2Value;
}
