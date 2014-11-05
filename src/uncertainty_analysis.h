/// @file uncertainty_analysis.h
/// Provides functionality for uncertainty analysis with Monte Carlo method.
#ifndef SCRAM_SRC_UNCERTAINTY_ANALYSIS_H_
#define SCRAM_SRC_UNCERTAINTY_ANALYSIS_H_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include <event.h>
#include <probability_analysis.h>

class UncertaintyAnalysisTest;

namespace scram {

class Reporter;

/// @class UncertaintyAnalysis
/// Uncertainty analysis and statistics for top event or gate probabilities
/// from minimal cut sets and probability distributions of basic events.
class UncertaintyAnalysis : private ProbabilityAnalysis {
friend class ::UncertaintyAnalysisTest;
friend class Reporter;

 public:
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;

  /// The main constructor of Uncertainty Analysis.
  /// @param[in] nsums The number of sums in the probability series.
  /// @param[in] cut_off The cut-off probability for cut sets.
  /// @param[in] num_trials The number of trials to perform.
  /// @throws InvalidArgument if any of the parameters is invalid.
  explicit UncertaintyAnalysis(int nsums = 7, double cut_off = 1e-8,
                               int num_trials = 1e3);

  /// Set the databases of primary events with probabilities.
  /// Resets the main primary events database and clears the
  /// previous information. This information is the main source for
  /// calculations.
  /// Updates internal indexes for events.
  /// @param[in] basic_events The database of basic event in cut sets.
  void UpdateDatabase(const boost::unordered_map<std::string, BasicEventPtr>&
                      basic_events);

  /// Performs quantitative analysis on minimal cut sets containing primary
  /// events provided in the databases.
  /// @param[in] min_cut_sets Minimal cut sets with string ids of events.
  ///                         Negative event is indicated by "'not' + id"
  void Analyze(const std::set< std::set<std::string> >& min_cut_sets);

  /// @returns Mean of the final distribution.
  inline double mean() const { return mean_; }

  /// @returns Standard deviation of the final distribution.
  inline double sigma() const { return sigma_; }

  /// @returns 95% confidence interval. Normal distribution is assumed.
  inline const std::pair<double, double>& confidence_interval() const {
    return confidence_interval_;
  }

  /// @returns The distribution histogram.
  const std::vector<std::pair<double, double> >& distribution() const {
    return distribution_;
  }

 private:
  /// Performs Monte Carlo Simulation by sampling the probability distributions
  /// and providing the final sampled values of the final probability.
  void Sample();

  /// Calculates statistical values from the final distribution.
  void CalculateStatistics();

  std::vector<double> sampled_results_;  ///< Storage for sampled values.

  int num_trials_;  ///< The number of trials to perform.

  double p_time_;  ///< Time for probability calculations.

  double mean_;  ///< The mean of the final distribution.
  double sigma_;  ///< The standard deviation of the final distribution.
  /// The confidence interval of the distribution.
  std::pair<double, double> confidence_interval_;
  /// The histogram density of the distribution with lower bounds and values.
  std::vector<std::pair<double, double> > distribution_;
};

}  // namespace scram

#endif  // SCRAM_SRC_UNCERTAINTY_ANALYSIS_H_
