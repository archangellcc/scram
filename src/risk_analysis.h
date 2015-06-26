/// @file risk_analysis.h
/// Contains the main system for performing analysis.
#ifndef SCRAM_SRC_RISK_ANALYISIS_H_
#define SCRAM_SRC_RISK_ANALYISIS_H_

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "fault_tree_analysis.h"
#include "probability_analysis.h"
#include "settings.h"
#include "uncertainty_analysis.h"

class RiskAnalysisTest;
class PerformanceTest;

namespace scram {

class Gate;
class FaultTree;
class Model;

/// @class RiskAnalysis
/// Main system that performs analyses.
class RiskAnalysis {
  friend class ::RiskAnalysisTest;
  friend class ::PerformanceTest;

 public:
  typedef boost::shared_ptr<Model> ModelPtr;

  /// Constructs RiskAnalysis with a valid model and analysis settings.
  /// @param[in] model An analysis model with fault trees, events, etc.
  /// @param[in] settings Analysis settings for the given model.
  RiskAnalysis(const ModelPtr& model, const Settings& settings);

  /// Provides graphing instructions for each fault tree initialized in
  /// the analysis. All top events from fault trees are processed into output
  /// files named with fault tree and top event names.
  /// @throws IOError if any output file cannot be accessed for writing.
  /// @note This function must be called only after initialization of the tree.
  void GraphingInstructions();

  /// Performs the main analysis operations.
  /// Analyzes the fault tree and performs computations.
  /// This function must be called only after initializing the tree with or
  /// without its probabilities.
  void Analyze();

  /// Reports all results generated by all analyses into XML formatted stream.
  /// The report is appended to the stream.
  /// @param[out] out The output stream.
  /// @note This function must be called only after Analyze() function.
  void Report(std::ostream& out);

  /// Reports the results of analyses to a specified output destination.
  /// This function overwrites the file.
  /// @note This function must be called only after Analyze() function.
  /// param[out] output The output destination.
  /// @throws IOError if the output file is not accessible.
  void Report(std::string output);

 private:
  typedef boost::shared_ptr<Gate> GatePtr;
  typedef boost::shared_ptr<FaultTree> FaultTreePtr;
  typedef boost::shared_ptr<FaultTreeAnalysis> FaultTreeAnalysisPtr;
  typedef boost::shared_ptr<ProbabilityAnalysis> ProbabilityAnalysisPtr;
  typedef boost::shared_ptr<UncertaintyAnalysis> UncertaintyAnalysisPtr;

  ModelPtr model_;  ///< Analysis model with constructs.
  Settings settings_;  ///< Settings for analysis.

  /// Fault tree analyses that are performed on a specific fault tree.
  std::map<std::string, FaultTreeAnalysisPtr> ftas_;

  /// Probability analyses that are performed on a specific fault tree.
  std::map<std::string, ProbabilityAnalysisPtr> prob_analyses_;

  /// Uncertainty analyses that are performed on a specific fault tree.
  std::map<std::string, UncertaintyAnalysisPtr> uncertainty_analyses_;
};

}  // namespace scram

#endif  // SCRAM_SRC_RISK_ANALYSIS_H_
