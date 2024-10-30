# Copyright (c) Stanford University, The Regents of the University of
#               California, and others.
#
# All Rights Reserved.
#
# See Copyright-SimVascular.txt for additional details.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject
# to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Build the model object files.

set(lib svzero_model_library)

set(CXXSRCS 
  Block.cpp
  BloodVessel.cpp
  BloodVesselJunction.cpp
  ChamberElastanceInductor.cpp
  ClosedLoopCoronaryBC.cpp
  ClosedLoopCoronaryLeftBC.cpp
  ClosedLoopCoronaryRightBC.cpp
  ClosedLoopHeartPulmonary.cpp
  ClosedLoopRCRBC.cpp
  DOFHandler.cpp
  FlowReferenceBC.cpp
  Junction.cpp
  Model.cpp
  Node.cpp
  OpenLoopCoronaryBC.cpp
  Parameter.cpp
  PressureReferenceBC.cpp
  ResistanceBC.cpp
  ResistiveJunction.cpp
  ValveTanh.cpp
  WindkesselBC.cpp
)

set(HDRS 
  Block.h
  BlockType.h
  BloodVessel.h
  BloodVesselJunction.h
  ChamberElastanceInductor.h
  ClosedLoopCoronaryBC.h
  ClosedLoopCoronaryLeftBC.h
  ClosedLoopCoronaryRightBC.h
  ClosedLoopHeartPulmonary.h
  ClosedLoopRCRBC.h
  DOFHandler.h
  FlowReferenceBC.h
  Junction.h
  Model.h
  Node.h
  OpenLoopCoronaryBC.h
  Parameter.h
  PressureReferenceBC.h
  ResistanceBC.h
  ResistiveJunction.h
  ValveTanh.h
  WindkesselBC.h
)

add_library(${lib} OBJECT ${CXXSRCS} ${HDRS})

target_include_directories( ${lib} PUBLIC
  ${CMAKE_SOURCE_DIR}/src/algebra
  ${CMAKE_SOURCE_DIR}/src/model
  ${CMAKE_SOURCE_DIR}/src/solve
  ${CMAKE_SOURCE_DIR}/src/ThirdParty
)

target_link_libraries( ${lib} Eigen3::Eigen )
target_link_libraries( ${lib} nlohmann_json::nlohmann_json )

