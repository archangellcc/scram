/// @file ccf_group.cc
/// Implementation of various common cause failure models.
#include "ccf_group.h"

#include <sstream>

#include <boost/algorithm/string.hpp>

namespace scram {

CcfGroup::CcfGroup(const std::string& name, const std::string& model,
                   const std::string& base_path, bool is_public)
    : Role::Role(is_public, base_path),
      name_(name),
      model_(model) {
  assert(name != "");
  id_ = is_public ? name : base_path + "." + name;  // Unique combination.
  boost::to_lower(id_);
}

void CcfGroup::AddMember(const BasicEventPtr& basic_event) {
  std::string name = basic_event->name();
  boost::to_lower(name);
  if (distribution_) {
    throw IllegalOperation("No more members accepted. The distribution for " +
                           name_ + " CCF group has already been defined.");
  }
  if (members_.count(name)) {
    throw DuplicateArgumentError("Duplicate member " + basic_event->name() +
                                 " in " + name_ + " CCF group.");
  }
  members_.insert(std::make_pair(name, basic_event));
}

void CcfGroup::AddDistribution(const ExpressionPtr& distr) {
  assert(!distribution_);
  distribution_ = distr;
  // Define probabilities of all basic events.
  std::map<std::string, BasicEventPtr>::iterator it;
  for (it = members_.begin(); it != members_.end(); ++it) {
    it->second->expression(distribution_);
  }
}

void CcfGroup::AddFactor(const ExpressionPtr& factor, int level) {
  assert(level > 0);
  if (level != factors_.size() + 1) {
    std::stringstream msg;
    msg << name_ << " " << model_ << " CCF group level expected "
        << factors_.size() + 1 << ". Instead was given " << level;
    throw ValidationError(msg.str());
  }
  factors_.push_back(std::make_pair(level, factor));
}

void CcfGroup::ValidateDistribution() {
  if (distribution_->Min() < 0 || distribution_->Max() > 1) {
    throw ValidationError("Distribution for " + name_ + " CCF group" +
                          " has illegal values.");
  }
}

void CcfGroup::Validate() {
  if (members_.size() < 2) {
    throw ValidationError(name_ + " CCF group must have at least 2 members.");
  }

  if (factors_.back().first > members_.size()) {
    throw ValidationError("The level of factors for " + name_ + " CCF group" +
                          " cannot be more than # of members.");
  }
  std::vector<std::pair<int, ExpressionPtr> >::iterator it;
  for (it = factors_.begin(); it != factors_.end(); ++it) {
    if (it->second->Max() > 1 || it->second->Min() < 0) {
      throw ValidationError("Factors for " + name_ + " CCF group" +
                            " have illegal values.");
    }
  }
}

void CcfGroup::ApplyModel() {
  // Construct replacement gates for member basic events.
  std::map<std::string, GatePtr> gates;
  std::map<std::string, BasicEventPtr>::const_iterator it_m;
  for (it_m = members_.begin(); it_m != members_.end(); ++it_m) {
    BasicEventPtr member = it_m->second;
    GatePtr new_gate(
        new Gate(member->name(), member->base_path(), member->is_public()));
    assert(member->id() == new_gate->id());
    new_gate->formula(FormulaPtr(new Formula("or")));
    gates.insert(std::make_pair(it_m->first, new_gate));
    member->ccf_gate(new_gate);
  }

  int max_level = factors_.back().first;  // Assumes that factors are
                                          // sequential.
  std::map<int, ExpressionPtr> probabilities;  // The level is position + 1.
  this->CalculateProb(max_level, &probabilities);

  // Mapping of new basic events and their parents.
  std::map<BasicEventPtr, std::set<std::string> > new_events;
  this->ConstructCcfBasicEvents(max_level, &new_events);

  assert(!new_events.empty());
  std::map<BasicEventPtr, std::set<std::string> >::iterator it;
  for (it = new_events.begin(); it != new_events.end(); ++it) {
    int level = it->second.size();
    ExpressionPtr prob = probabilities.find(level)->second;
    BasicEventPtr new_event = it->first;
    new_event->expression(prob);
    // Add this basic event to the parent gates.
    std::set<std::string>::iterator it_l;
    for (it_l = it->second.begin(); it_l != it->second.end(); ++it_l) {
      gates.find(*it_l)->second->formula()->AddArgument(new_event);
    }
  }
}

void CcfGroup::ConstructCcfBasicEvents(
    int max_level,
    std::map<BasicEventPtr, std::set<std::string> >* new_events) {
  typedef boost::shared_ptr<CcfEvent> CcfEventPtr;

  assert(max_level > 1);
  assert(members_.size() > 1);
  assert(max_level <= members_.size());
  assert(new_events->empty());

  std::set<std::set<std::string> > combinations;
  std::set<std::string> comb;  // One combination.
  combinations.insert(comb);  // Empty set is needed for iteration.

  for (int i = 0; i < max_level; ++i) {
    std::set<std::set<std::string> > next_level;
    std::set<std::set<std::string> >::iterator it;
    for (it = combinations.begin(); it != combinations.end(); ++it) {
      std::map<std::string, BasicEventPtr>::const_iterator it_m;
      for (it_m = members_.begin(); it_m != members_.end(); ++it_m) {
        if (!it->count(it_m->first)) {
          std::set<std::string> comb(*it);
          comb.insert(it_m->first);
          next_level.insert(comb);
        }
      }
    }
    for (it = next_level.begin(); it != next_level.end(); ++it) {
      std::string name = "[";
      std::vector<std::string> names;
      std::set<std::string>::const_iterator it_s;
      for (it_s = it->begin(); it_s != it->end();) {
        std::string member_name = members_.find(*it_s)->second->name();
        name += member_name;
        names.push_back(member_name);
        ++it_s;
        if (it_s != it->end()) name += " ";
      }
      name += "]";
      CcfEventPtr new_basic_event(new CcfEvent(name, this, names));
      new_events->insert(std::make_pair(new_basic_event, *it));
    }
    combinations = next_level;
  }
}

void BetaFactorModel::AddFactor(const ExpressionPtr& factor, int level) {
  if (!factors_.empty()) {
    throw ValidationError("Beta-Factor Model " + this->name() + " CCF group" +
                          " must have exactly one factor.");
  }
  if (level != members_.size()) {
    throw ValidationError("Beta-Factor Model " + this->name() + " CCF group" +
                          " must have the level matching the number of" +
                          " its members.");
  }
  CcfGroup::factors_.push_back(std::make_pair(level, factor));
}

void BetaFactorModel::ConstructCcfBasicEvents(
    int max_level,
    std::map<BasicEventPtr, std::set<std::string> >* new_events) {
  // This function is not optimized or efficient for beta-factor models.
  assert(CcfGroup::factors_.size() == 1);
  std::map<BasicEventPtr, std::set<std::string> > all_events;
  CcfGroup::ConstructCcfBasicEvents(max_level, &all_events);  // Standard case.

  std::map<BasicEventPtr, std::set<std::string> >::iterator it;
  for (it = all_events.begin(); it != all_events.end(); ++it) {
    int level = it->second.size();  // Filter out only relevant levels.
    if (level == 1 || level == max_level) new_events->insert(*it);
  }
}

void BetaFactorModel::CalculateProb(
    int max_level,
    std::map<int, ExpressionPtr>* probabilities) {
  assert(probabilities->empty());
  std::vector<ExpressionPtr> args;  // For expression arguments.
  ExpressionPtr one(new ConstantExpression(1.0));
  args.push_back(one);
  ExpressionPtr beta = CcfGroup::factors_.begin()->second;
  args.push_back(beta);
  ExpressionPtr indep_factor(new Sub(args));  // (1 - beta)
  args.clear();
  args.push_back(indep_factor);
  args.push_back(CcfGroup::distribution_);  // (1 - beta) * Q
  probabilities->insert(std::make_pair(1, ExpressionPtr(new Mul(args))));

  args.clear();
  args.push_back(beta);
  args.push_back(CcfGroup::distribution_);  // beta * Q
  probabilities->insert(std::make_pair(max_level,
                                       ExpressionPtr(new Mul(args))));
}

void MglModel::AddFactor(const ExpressionPtr& factor, int level) {
  assert(level > 0);
  if (level != factors_.size() + 2) {
    std::stringstream msg;
    msg << this->name() << " MGL model CCF group level expected "
        << factors_.size() + 2 << ". Instead was given " << level;
    throw ValidationError(msg.str());
  }
  CcfGroup::factors_.push_back(std::make_pair(level, factor));
}

void MglModel::CalculateProb(int max_level,
                             std::map<int, ExpressionPtr>* probabilities) {
  assert(factors_.size() == max_level - 1);

  for (int i = 0; i < max_level; ++i) {
    int num_members = members_.size();
    // (n - 1) choose (k - 1) element in the equation.
    int mult = CcfGroup::Factorial(num_members - 1) /
               CcfGroup::Factorial(num_members - i - 1) /
               CcfGroup::Factorial(i);

    ExpressionPtr k(new ConstantExpression(1.0 / mult));

    std::vector<ExpressionPtr> args;
    args.push_back(k);
    for (int j = 0; j < i; ++j) {
      args.push_back(factors_[j].second);
    }
    if (i < max_level - 1) {
      ExpressionPtr one(new ConstantExpression(1.0));
      std::vector<ExpressionPtr> sub_args;
      sub_args.push_back(one);
      sub_args.push_back(factors_[i].second);
      ExpressionPtr last_factor(new Sub(sub_args));
      args.push_back(last_factor);
    }
    args.push_back(distribution_);
    ExpressionPtr prob(new Mul(args));
    probabilities->insert(std::make_pair(i + 1, prob));
  }
  assert(probabilities->size() == max_level);
}

void AlphaFactorModel::CalculateProb(
    int max_level,
    std::map<int, ExpressionPtr>* probabilities) {
  assert(probabilities->empty());
  assert(factors_.size() == max_level);
  std::vector<ExpressionPtr> sum_args;
  std::vector<std::pair<int, ExpressionPtr> >::iterator it_f;
  for (it_f = factors_.begin(); it_f != factors_.end(); ++it_f) {
    sum_args.push_back(it_f->second);
  }
  ExpressionPtr sum(new Add(sum_args));

  for (int i = 0; i < max_level; ++i) {
    int num_members = members_.size();
    // (n - 1) choose (k - 1) element in the equation.
    int mult = CcfGroup::Factorial(num_members - 1) /
               CcfGroup::Factorial(num_members - i - 1) /
               CcfGroup::Factorial(i);

    ExpressionPtr k(new ConstantExpression(1.0 / mult));

    std::vector<ExpressionPtr> args;
    args.push_back(k);
    std::vector<ExpressionPtr> div_args;
    div_args.push_back(factors_[i].second);
    div_args.push_back(sum);
    ExpressionPtr fraction(new Div(div_args));
    args.push_back(fraction);
    args.push_back(distribution_);
    ExpressionPtr prob(new Mul(args));
    probabilities->insert(std::make_pair(i + 1, prob));
  }
  assert(probabilities->size() == max_level);
}

void PhiFactorModel::Validate() {
  CcfGroup::Validate();
  double sum = 0;
  double sum_min = 0;
  double sum_max = 0;
  /// @todo How to assure that the sum will be 1 in sampling.
  ///       Is it allowed to have a factor sampling for Uncertainty analysis.
  std::vector<std::pair<int, ExpressionPtr> >::iterator it;
  for (it = CcfGroup::factors_.begin(); it != CcfGroup::factors_.end(); ++it) {
    sum += it->second->Mean();
    sum_min += it->second->Min();
    sum_max += it->second->Max();
  }
  /// @todo Problems with floating point number comparison.
  double epsilon = 1e-4;
  double diff = std::abs(sum - 1);
  double diff_min = std::abs(sum_min - 1);
  double diff_max = std::abs(sum_max - 1);
  if (diff_min > epsilon || diff > epsilon || diff_max > epsilon) {
    throw ValidationError("The factors for Phi model " + CcfGroup::name() +
                          " CCF group must sum to 1.");
  }
}

void PhiFactorModel::CalculateProb(
    int max_level,
    std::map<int, ExpressionPtr>* probabilities) {
  assert(probabilities->empty());
  std::vector<std::pair<int, ExpressionPtr> >::iterator it_f;
  for (it_f = factors_.begin(); it_f != factors_.end(); ++it_f) {
    std::vector<ExpressionPtr> args;
    args.push_back(it_f->second);
    args.push_back(distribution_);
    ExpressionPtr prob(new Mul(args));
    probabilities->insert(std::make_pair(it_f->first, prob));
  }
  assert(probabilities->size() == max_level);
}

}  // namespace scram
