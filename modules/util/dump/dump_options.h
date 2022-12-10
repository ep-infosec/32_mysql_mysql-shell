/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MODULES_UTIL_DUMP_DUMP_OPTIONS_H_
#define MODULES_UTIL_DUMP_DUMP_OPTIONS_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/storage/config.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/version.h"

#include "modules/util/dump/compatibility_option.h"
#include "modules/util/dump/instance_cache.h"
#include "modules/util/import_table/dialect.h"

namespace mysqlsh {
namespace dump {

class Dump_options {
 public:
  Dump_options();

  Dump_options(const Dump_options &) = default;
  Dump_options(Dump_options &&) = default;

  Dump_options &operator=(const Dump_options &) = default;
  Dump_options &operator=(Dump_options &&) = default;

  virtual ~Dump_options() = default;

  static const shcore::Option_pack_def<Dump_options> &options();

  void validate() const;

  // setters
  void set_session(const std::shared_ptr<mysqlshdk::db::ISession> &session);

  // getters
  const std::string &output_url() const { return m_output_url; }

  void set_output_url(const std::string &url) { m_output_url = url; }

  const shcore::Dictionary_t &original_options() const { return m_options; }

  bool use_base64() const { return m_use_base64; }

  int64_t max_rate() const { return m_max_rate; }

  bool show_progress() const { return m_show_progress; }

  mysqlshdk::storage::Compression compression() const { return m_compression; }

  const std::shared_ptr<mysqlshdk::db::ISession> &session() const {
    return m_session;
  }

  const import_table::Dialect &dialect() const { return m_dialect; }

  const mysqlshdk::storage::Config_ptr &storage_config() const {
    return m_storage_config;
  }

  const std::string &character_set() const { return m_character_set; }

  const mysqlshdk::utils::nullable<mysqlshdk::utils::Version>
      &mds_compatibility() const {
    return m_mds;
  }

  const Compatibility_options &compatibility_options() const {
    return m_compatibility_options;
  }

  const std::vector<shcore::Account> &excluded_users() const {
    return m_excluded_users;
  }

  const std::vector<shcore::Account> &included_users() const {
    return m_included_users;
  }

  const Instance_cache_builder::Filter &included_schemas() const {
    return m_included_schemas;
  }

  const Instance_cache_builder::Filter &excluded_schemas() const {
    return m_excluded_schemas;
  }

  const Instance_cache_builder::Object_filters &included_tables() const {
    return m_included_tables;
  }

  const Instance_cache_builder::Object_filters &excluded_tables() const {
    return m_excluded_tables;
  }

  const Instance_cache_builder::Object_filters &included_events() const {
    return m_included_events;
  }

  const Instance_cache_builder::Object_filters &excluded_events() const {
    return m_excluded_events;
  }

  const Instance_cache_builder::Object_filters &included_routines() const {
    return m_included_routines;
  }

  const Instance_cache_builder::Object_filters &excluded_routines() const {
    return m_excluded_routines;
  }

  const Instance_cache_builder::Trigger_filters &included_triggers() const {
    return m_included_triggers;
  }

  const Instance_cache_builder::Trigger_filters &excluded_triggers() const {
    return m_excluded_triggers;
  }

  virtual bool split() const = 0;

  virtual uint64_t bytes_per_chunk() const = 0;

  virtual std::size_t threads() const = 0;

  virtual bool is_export_only() const = 0;

  virtual bool use_single_file() const = 0;

  virtual bool dump_ddl() const = 0;

  virtual bool dump_data() const = 0;

  virtual bool is_dry_run() const = 0;

  virtual bool consistent_dump() const = 0;

  virtual bool dump_events() const = 0;

  virtual bool dump_routines() const = 0;

  virtual bool dump_triggers() const = 0;

  virtual bool dump_users() const = 0;

  virtual bool use_timezone_utc() const = 0;

  virtual bool dump_binlog_info() const = 0;

  virtual bool par_manifest() const = 0;

 protected:
  void set_dialect(const import_table::Dialect &dialect) {
    m_dialect = dialect;
  }

  void set_compression(mysqlshdk::storage::Compression compression) {
    m_compression = compression;
  }

  void set_mds_compatibility(
      const mysqlshdk::utils::nullable<mysqlshdk::utils::Version> &mds) {
    m_mds = mds;
  }

  void set_compatibility_option(Compatibility_option c) {
    m_compatibility_options |= c;
  }

  template <typename C>
  void add_excluded_users(const C &excluded_users) {
    try {
      auto accounts = shcore::to_accounts(excluded_users);
      std::move(accounts.begin(), accounts.end(),
                std::back_inserter(m_excluded_users));
    } catch (const std::runtime_error &e) {
      throw std::invalid_argument(e.what());
    }
  }

  template <typename C>
  void set_included_users(const C &included_users) {
    try {
      m_included_users = shcore::to_accounts(included_users);
    } catch (const std::runtime_error &e) {
      throw std::invalid_argument(e.what());
    }
  }

  template <typename C, typename M>
  void add_object_filters(const C &data, const std::string &object_type,
                          const std::string &action, M *map) {
    std::string schema;
    std::string object;

    for (const auto &t : data) {
      schema.clear();
      object.clear();

      try {
        shcore::split_schema_and_table(t, &schema, &object);
      } catch (const std::runtime_error &e) {
        throw std::invalid_argument("Failed to parse " + object_type +
                                    " to be " + action + "d '" + t +
                                    "': " + e.what());
      }

      if (schema.empty()) {
        throw std::invalid_argument(
            "The " + object_type + " to be " + action +
            "d must be in the following form: schema." + object_type +
            ", with optional backtick quotes, wrong value: '" + t + "'.");
      }

      (*map)[schema].emplace(std::move(object));
    }
  }

  template <typename C>
  void add_trigger_filters(const C &data, const std::string &action,
                           Instance_cache_builder::Trigger_filters *target) {
    std::string schema;
    std::string table;
    std::string object;

    for (const auto &t : data) {
      schema.clear();
      table.clear();
      object.clear();

      try {
        shcore::split_schema_table_and_object(t, &schema, &table, &object);
      } catch (const std::runtime_error &e) {
        throw std::invalid_argument("Failed to parse trigger to be " + action +
                                    "d '" + t + "': " + e.what());
      }

      if (table.empty()) {
        throw std::invalid_argument(
            "The trigger to be " + action +
            "d must be in the following form: schema.table or "
            "schema.table.trigger, with optional backtick quotes, wrong value: "
            "'" +
            t + "'.");
      }

      if (schema.empty()) {
        // we got schema.table, need to move names around
        std::swap(schema, table);
        std::swap(table, object);
      }

      // Object name can be empty, in this case all triggers in a table are
      // included/excluded. We add it as is, so later we can detect cases like
      // 'schema.table' and 'schema.table.trigger' given by the user at the same
      // time.
      (*target)[schema][table].emplace(std::move(object));
    }
  }

  bool exists(const std::string &schema) const;

  bool exists(const std::string &schema, const std::string &table) const;

  std::set<std::string> find_missing(
      const std::unordered_set<std::string> &schemas) const;

  std::set<std::string> find_missing(
      const std::string &schema,
      const std::unordered_set<std::string> &tables) const;

  Instance_cache_builder::Filter m_included_schemas;
  Instance_cache_builder::Filter m_excluded_schemas;

  Instance_cache_builder::Object_filters m_included_tables;
  Instance_cache_builder::Object_filters m_excluded_tables;

  Instance_cache_builder::Object_filters m_included_events;
  Instance_cache_builder::Object_filters m_excluded_events;

  Instance_cache_builder::Object_filters m_included_routines;
  Instance_cache_builder::Object_filters m_excluded_routines;

  Instance_cache_builder::Trigger_filters m_included_triggers;
  Instance_cache_builder::Trigger_filters m_excluded_triggers;

  mutable bool m_filter_conflicts = false;

 protected:
  void on_start_unpack(const shcore::Dictionary_t &options);
  void set_storage_config(
      const std::shared_ptr<mysqlshdk::storage::Config> &storage_config);

  // This function should be implemented when the validation process requires
  // data NOT coming on the user options, i.e. a session
  virtual void validate_options() const {}

  void on_log_options(const char *msg) const;

  void error_on_schema_filters_conflicts() const;

  void error_on_table_filters_conflicts() const;

  void error_on_event_filters_conflicts() const;

  void error_on_routine_filters_conflicts() const;

  void error_on_trigger_filters_conflicts() const;

 private:
  virtual void on_set_session(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) = 0;

  void set_string_option(const std::string &option, const std::string &value);

  std::set<std::string> find_missing_impl(
      const std::string &subquery,
      const std::unordered_set<std::string> &objects) const;

  void error_on_object_filters_conflicts(
      const Instance_cache_builder::Object_filters &included,
      const Instance_cache_builder::Object_filters &excluded,
      const std::string &object_label, const std::string &option_suffix) const;

  void error_on_object_filters_conflicts(
      const Instance_cache_builder::Filter &included,
      const Instance_cache_builder::Filter &excluded,
      const std::string &object_label, const std::string &option_suffix,
      const std::string &schema_name) const;

  template <typename C>
  void error_on_schema_cross_filters_conflicts(
      const C &included, const C &excluded, const std::string &object_label,
      const std::string &option_suffix) const;

  void error_on_table_cross_filters_conflicts() const;

  // global session
  std::shared_ptr<mysqlshdk::db::ISession> m_session;

  // input arguments
  std::string m_output_url;
  shcore::Dictionary_t m_options;

  // not configurable
  bool m_use_base64 = true;

  // common options
  int64_t m_max_rate = 0;
  bool m_show_progress;
  mysqlshdk::storage::Compression m_compression =
      mysqlshdk::storage::Compression::ZSTD;
  mysqlshdk::storage::Config_ptr m_storage_config;

  std::string m_character_set = "utf8mb4";

  // these options are unpacked elsewhere, but are here 'cause we're returning
  // a reference
  // currently used by exportTable()
  import_table::Dialect m_dialect;

  // currently used by dumpTables(), dumpSchemas() and dumpInstance()
  mysqlshdk::utils::nullable<mysqlshdk::utils::Version> m_mds;
  Compatibility_options m_compatibility_options;

  // currently used by dumpInstance()
  std::vector<shcore::Account> m_excluded_users;
  std::vector<shcore::Account> m_included_users;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_OPTIONS_H_
