/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// @file model.h
/// Representation for a model container for risk analysis.

#ifndef SCRAM_SRC_MODEL_H_
#define SCRAM_SRC_MODEL_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "ccf_group.h"
#include "element.h"
#include "event.h"
#include "expression.h"
#include "fault_tree.h"

namespace scram {

/// @class Model
/// This class represents a risk analysis model.
class Model : public Element {
 public:
  /// Creates a model container.
  ///
  /// @param[in] name  The optional name for the model.
  explicit Model(const std::string& name = "");

  Model(const Model&) = delete;
  Model& operator=(const Model&) = delete;

  /// @returns The name of the model.
  const std::string& name() const { return name_; }

  /// @returns Defined constructs in the model.
  /// @{
  const std::unordered_map<std::string, FaultTreePtr>& fault_trees() const {
    return fault_trees_;
  }
  const std::unordered_map<std::string, ParameterPtr>& parameters() const {
    return parameters_;
  }
  const std::unordered_map<std::string, HouseEventPtr>& house_events() const {
    return house_events_;
  }
  const std::unordered_map<std::string, BasicEventPtr>& basic_events() const {
    return basic_events_;
  }
  const std::unordered_map<std::string, GatePtr>& gates() const {
    return gates_;
  }
  const std::unordered_map<std::string, CcfGroupPtr>& ccf_groups() const {
    return ccf_groups_;
  }
  /// @}

  /// Adds a fault tree into the model container.
  /// Fault trees are uniquely owned by this model.
  ///
  /// @param[in] fault_tree  A fault tree defined in this model.
  ///
  /// @throws RedefinitionError  The model has a container with the same name.
  void AddFaultTree(FaultTreePtr fault_tree);

  /// Adds a parameter that is used in this model's expressions.
  ///
  /// @param[in] parameter  A parameter defined in this model.
  ///
  /// @throws RedefinitionError  The model has a parameter with the same name.
  void AddParameter(const ParameterPtr& parameter);

  /// Finds a parameter from a reference.
  /// The reference is not case sensitive
  /// and can contain the identifier, full path, or local path.
  ///
  /// @param[in] reference  Reference string to the parameter.
  /// @param[in] base_path  The series of containers indicating the scope.
  ///
  /// @returns Pointer to the parameter found by following the given reference.
  ///
  /// @throws std::out_of_range  The entity cannot be found.
  ParameterPtr GetParameter(const std::string& reference,
                            const std::string& base_path);

  /// Adds a house event that is used in this model.
  ///
  /// @param[in] house_event  A house event defined in this model.
  ///
  /// @throws RedefinitionError  An event with the same name already exists.
  void AddHouseEvent(const HouseEventPtr& house_event);

  /// Finds a house event from a reference.
  /// The reference is not case sensitive
  /// and can contain the identifier, full path, or local path.
  ///
  /// @param[in] reference  Reference string to the house event.
  /// @param[in] base_path  The series of containers indicating the scope.
  ///
  /// @returns Pointer to the house event found by following the reference.
  ///
  /// @throws std::out_of_range  The entity cannot be found.
  HouseEventPtr GetHouseEvent(const std::string& reference,
                              const std::string& base_path);

  /// Adds a basic event that is used in this model.
  ///
  /// @param[in] basic_event  A basic event defined in this model.
  ///
  /// @throws RedefinitionError  An event with the same name already exists.
  void AddBasicEvent(const BasicEventPtr& basic_event);

  /// Finds a basic event from a reference.
  /// The reference is not case sensitive
  /// and can contain the identifier, full path, or local path.
  ///
  /// @param[in] reference  Reference string to the basic event.
  /// @param[in] base_path  The series of containers indicating the scope.
  ///
  /// @returns Pointer to the basic event found by following the reference.
  ///
  /// @throws std::out_of_range  The entity cannot be found.
  BasicEventPtr GetBasicEvent(const std::string& reference,
                              const std::string& base_path);

  /// Adds a gate that is used in this model's fault trees or components.
  ///
  /// @param[in] gate  A gate defined in this model.
  ///
  /// @throws RedefinitionError  An event with the same name already exists.
  void AddGate(const GatePtr& gate);

  /// Finds a gate from a reference.
  /// The reference is not case sensitive
  /// and can contain the identifier, full path, or local path.
  ///
  /// @param[in] reference  Reference string to the gate.
  /// @param[in] base_path  The series of containers indicating the scope.
  ///
  /// @returns Pointer to the gate found by following the reference.
  ///
  /// @throws std::out_of_range  The entity cannot be found.
  GatePtr GetGate(const std::string& reference, const std::string& base_path);

  /// Adds a CCF group that is used in this model's fault trees.
  ///
  /// @param[in] ccf_group  A CCF group defined in this model.
  ///
  /// @throws RedefinitionError  The model has a CCF group with the same name.
  void AddCcfGroup(const CcfGroupPtr& ccf_group);

 private:
  /// Helper function to find the container for references.
  ///
  /// @param[in] path  The ancestor container names in lower case.
  ///
  /// @returns A fault tree or component from the base path if any.
  ///
  /// @throws std::out_of_range  There's missing container in the path.
  const Component& GetContainer(const std::vector<std::string>& path);

  std::string name_;  ///< The name of the model.

  /// A collection of defined constructs in the model.
  /// @{
  std::unordered_map<std::string, FaultTreePtr> fault_trees_;
  std::unordered_map<std::string, GatePtr> gates_;
  std::unordered_map<std::string, HouseEventPtr> house_events_;
  std::unordered_map<std::string, BasicEventPtr> basic_events_;
  std::unordered_map<std::string, ParameterPtr> parameters_;
  std::unordered_map<std::string, CcfGroupPtr> ccf_groups_;
  /// @}
  std::unordered_set<std::string> event_ids_;  ///< For faster lookup.
};

}  // namespace scram

#endif  // SCRAM_SRC_MODEL_H_
