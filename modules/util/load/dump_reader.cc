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

#include "modules/util/load/dump_reader.h"

#include <algorithm>
#include <atomic>
#include <numeric>
#include <utility>

#include "modules/util/dump/dump_utils.h"
#include "modules/util/dump/schema_dumper.h"
#include "modules/util/load/load_errors.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {

namespace {

using mysqlshdk::utils::Version;

std::string fetch_file(mysqlshdk::storage::IDirectory *dir,
                       const std::string &fn) {
  auto file = dir->file(fn);
  file->open(mysqlshdk::storage::Mode::READ);

  std::string data = mysqlshdk::storage::read_file(file.get());

  file->close();

  return data;
}

shcore::Dictionary_t parse_metadata(const std::string &data,
                                    const std::string &fn) {
  try {
    auto metadata = shcore::Value::parse(data);
    if (metadata.type != shcore::Map) {
      THROW_ERROR(SHERR_LOAD_INVALID_METADATA_FILE, fn.c_str());
    }
    return metadata.as_map();
  } catch (const shcore::Exception &e) {
    THROW_ERROR(SHERR_LOAD_PARSING_METADATA_FILE_FAILED, fn.c_str(),
                e.format().c_str());
  }
}

shcore::Dictionary_t fetch_metadata(mysqlshdk::storage::IDirectory *dir,
                                    const std::string &fn) {
  return parse_metadata(fetch_file(dir, fn), fn);
}

auto to_vector_of_strings(const shcore::Array_t &arr) {
  std::vector<std::string> res;
  res.reserve(arr->size());

  for (const auto &s : *arr) {
    res.emplace_back(s.as_string());
  }

  return res;
}

}  // namespace

Dump_reader::Dump_reader(
    std::unique_ptr<mysqlshdk::storage::IDirectory> dump_dir,
    const Load_dump_options &options)
    : m_dir(std::move(dump_dir)), m_options(options) {}

Dump_reader::Status Dump_reader::open() {
  shcore::Dictionary_t md(fetch_metadata(m_dir.get(), "@.json"));

  shcore::Dictionary_t basenames(md->get_map("basenames"));

  for (const auto &schema : *md->get_array("schemas")) {
    if (include_schema(schema.as_string())) {
      auto info = std::make_shared<Schema_info>();

      info->schema = schema.get_string();
      if (basenames->has_key(info->schema)) {
        info->basename = basenames->get_string(info->schema);
      } else {
        info->basename = info->schema;
      }

      m_contents.schemas.emplace(info->schema, info);
    } else {
      log_debug("Skipping schema '%s'", schema.as_string().c_str());
    }
  }

  if (md->has_key("version"))
    m_contents.dump_version = Version(md->get_string("version"));

  if (md->has_key("serverVersion"))
    m_contents.server_version = Version(md->get_string("serverVersion"));

  if (md->has_key("origin")) m_contents.origin = md->get_string("origin");

  if (md->has_key("defaultCharacterSet"))
    m_contents.default_charset = md->get_string("defaultCharacterSet");

  if (md->has_key("binlogFile"))
    m_contents.binlog_file = md->get_string("binlogFile");

  if (md->has_key("binlogPosition"))
    m_contents.binlog_position = md->get_uint("binlogPosition");

  if (md->has_key("gtidExecuted"))
    m_contents.gtid_executed = md->get_string("gtidExecuted");

  if (md->has_key("gtidExecutedInconsistent")) {
    m_contents.gtid_executed_inconsistent =
        md->get_bool("gtidExecutedInconsistent");
  }

  if (md->has_key("tzUtc")) m_contents.tz_utc = md->get_bool("tzUtc");

  if (md->has_key("mdsCompatibility"))
    m_contents.mds_compatibility = md->get_bool("mdsCompatibility");

  if (md->has_key("compatibilityOptions")) {
    const auto options = md->at("compatibilityOptions")
                             .to_string_container<std::vector<std::string>>();

    m_contents.create_invisible_pks =
        std::find(options.begin(), options.end(), "create_invisible_pks") !=
        options.end();
  }

  if (md->has_key("tableOnly"))
    m_contents.table_only = md->get_bool("tableOnly");

  if (md->has_key("bytesPerChunk"))
    m_contents.bytes_per_chunk = md->get_uint("bytesPerChunk");

  m_contents.has_users = md->has_key("users");

  if (md->has_key("capabilities")) {
    const auto capabilities = md->at("capabilities").as_array();

    for (const auto &entry : *capabilities) {
      const auto capability = entry.as_map();

      m_contents.capabilities.emplace_back(Capability_info{
          capability->at("id").as_string(),
          capability->at("description").as_string(),
          Version(capability->at("versionRequired").as_string())});
    }
  }

  try {
    m_contents.parse_done_metadata(m_dir.get());
    m_dump_status = Status::COMPLETE;
  } catch (const std::exception &e) {
    log_info("@.done.json: %s", e.what());
    m_dump_status = Status::DUMPING;
  }

  return m_dump_status;
}

std::string Dump_reader::begin_script() const {
  return m_contents.sql ? *m_contents.sql : "";
}

std::string Dump_reader::end_script() const {
  return m_contents.post_sql ? *m_contents.post_sql : "";
}

std::string Dump_reader::users_script() const {
  return m_contents.users_sql ? *m_contents.users_sql : "";
}

bool Dump_reader::next_schema_and_tables(
    std::string *out_schema, std::list<Name_and_file> *out_tables,
    std::list<Name_and_file> *out_view_placeholders) {
  out_tables->clear();
  out_view_placeholders->clear();

  // find the first schema that is ready
  for (auto &it : m_contents.schemas) {
    auto &s = it.second;
    if (!s->table_sql_done && s->ready()) {
      *out_schema = s->schema;

      for (const auto &t : s->tables) {
        out_tables->emplace_back(
            t.first,
            t.second->has_sql ? m_dir->file(t.second->script_name()) : nullptr);
      }

      if (s->has_view_sql) {
        for (const auto &v : s->views) {
          out_view_placeholders->emplace_back(v.table,
                                              m_dir->file(v.pre_script_name()));
        }
      }

      s->table_sql_done = true;

      return true;
    }
  }

  return false;
}

bool Dump_reader::next_schema_and_views(std::string *out_schema,
                                        std::list<Name_and_file> *out_views) {
  out_views->clear();

  // find the first schema that is ready
  for (auto &it : m_contents.schemas) {
    auto &s = it.second;

    // always return true for every schema, even if there are no views
    if (!s->view_sql_done && s->ready()) {
      *out_schema = s->schema;

      if (s->has_view_sql) {
        for (const auto &v : s->views) {
          out_views->emplace_back(v.table, m_dir->file(v.script_name()));
        }
      }

      s->view_sql_done = true;

      return true;
    }
  }

  return false;
}

std::vector<shcore::Account> Dump_reader::accounts() const {
  std::vector<shcore::Account> account_list;
  std::string script = users_script();

  // parse the script to extract user list
  dump::Schema_dumper::preprocess_users_script(
      script, [&account_list](const std::string &account) {
        account_list.emplace_back(shcore::split_account(account));
        return true;
      });

  return account_list;
}

std::vector<std::string> Dump_reader::schemas() const {
  std::vector<std::string> slist;

  for (const auto &s : m_contents.schemas) {
    slist.push_back(s.second->schema);
  }

  return slist;
}

bool Dump_reader::schema_objects(const std::string &schema,
                                 std::vector<std::string> *out_tables,
                                 std::vector<std::string> *out_views,
                                 std::vector<std::string> *out_triggers,
                                 std::vector<std::string> *out_functions,
                                 std::vector<std::string> *out_procedures,
                                 std::vector<std::string> *out_events) {
  auto schema_it = m_contents.schemas.find(schema);
  if (schema_it == m_contents.schemas.end()) return false;
  auto schema_info = schema_it->second;

  out_tables->clear();
  for (const auto &t : schema_info->tables) {
    out_tables->push_back(t.first);
  }
  out_views->clear();
  for (const auto &v : schema_info->views) {
    out_views->push_back(v.table);
  }
  *out_triggers = schema_info->trigger_names;
  *out_functions = schema_info->function_names;
  *out_procedures = schema_info->procedure_names;
  *out_events = schema_info->event_names;

  return true;
}

void Dump_reader::schema_table_triggers(
    const std::string &schema, std::list<Name_and_file> *out_table_triggers) {
  out_table_triggers->clear();

  const auto &s = m_contents.schemas.at(schema);
  for (const auto &t : s->tables) {
    if (t.second->has_triggers)
      out_table_triggers->emplace_back(
          t.first, m_dir->file(t.second->triggers_script_name()));
  }
}

const std::vector<std::string> &Dump_reader::deferred_schema_fks(
    const std::string &schema) const {
  const auto &s = m_contents.schemas.at(schema);
  return s->foreign_key_queries;
}

const std::vector<std::string> &Dump_reader::queries_on_schema_end(
    const std::string &schema) const {
  const auto &s = m_contents.schemas.at(schema);
  return s->queries_on_schema_end;
}

const std::map<std::string, std::vector<std::string>>
Dump_reader::tables_without_pk() const {
  std::map<std::string, std::vector<std::string>> res;
  for (const auto &s : m_contents.schemas) {
    std::vector<std::string> tables;
    for (const auto &t : s.second->tables)
      if (t.second->primary_index.empty())
        tables.emplace_back(shcore::quote_identifier(t.first));
    if (!tables.empty()) {
      std::sort(tables.begin(), tables.end());
      res.emplace(s.first, tables);
    }
  }
  return res;
}

bool Dump_reader::has_tables_without_pk() const {
  for (const auto &s : m_contents.schemas) {
    for (const auto &t : s.second->tables) {
      if (t.second->primary_index.empty()) {
        return true;
      }
    }
  }

  return false;
}

bool Dump_reader::has_primary_key(const std::string &schema,
                                  const std::string &table) const {
  return !m_contents.schemas.at(schema)
              ->tables.at(table)
              ->primary_index.empty();
}

std::string Dump_reader::fetch_schema_script(const std::string &schema) const {
  std::string script;

  const auto &s = m_contents.schemas.at(schema);

  if (s->has_sql) {
    // Get the base script for the schema
    script = fetch_file(m_dir.get(), s->script_name());
  }

  return script;
}

// Proportional chunk scheduling
//
// Multiple sessions writing to the same table mean they will be competing for
// locks to be able to update indexes.
// So to optimize performance, we try to have as many different tables loaded
// at the same time as possible.
// If there are more tables than threads, that means there will be a single
// session per table.
// But if there are more threads than tables, we need to distribute them
// among tables. If we distribute the threads equally, then smaller tables
// will finish sooner and bigger tables will take extra longer because
// it will have more threads working. So our optimization goal must be to keep
// as many different tables loading concurrently for as long as possible, while
// ensuring that tables that are currently being loaded have preference over
// others, which could otherwise trigger tables to get flushed out from the
// buffer pool.
// Thus, smaller tables must get fewer threads allocated so they take longer
// to load, while bigger threads get more, with the hope that the total time
// to load all tables is minimized.
Dump_reader::Candidate Dump_reader::schedule_chunk_proportionally(
    const std::unordered_multimap<std::string, size_t> &tables_being_loaded,
    std::unordered_set<Dump_reader::Table_data_info *> *tables_with_data,
    uint64_t max_concurrent_tables) {
  if (tables_with_data->empty()) return tables_with_data->end();

  std::vector<Candidate> tables_in_progress;

  // first check if there's any table that's not being loaded
  {
    const auto end = tables_with_data->end();
    auto best = end;

    for (auto it = tables_with_data->begin(); it != end; ++it) {
      if ((*it)->chunks_consumed) {
        tables_in_progress.emplace_back(it);
      }

      if (tables_being_loaded.find((*it)->key()) == tables_being_loaded.end()) {
        // table is better if it's bigger and in the same state as the current
        // best, or if it was previously scheduled and current best was not
        if (best == end ||
            ((*it)->bytes_available() > (*best)->bytes_available() &&
             !(*it)->chunks_consumed == !(*best)->chunks_consumed) ||
            ((*it)->chunks_consumed && !(*best)->chunks_consumed))
          best = it;
      }
    }

    // schedule a new table only if we're not exceeding the maximum number of
    // concurrent tables that can be loaded at the same time
    if (best != end && (tables_in_progress.size() < max_concurrent_tables ||
                        (*best)->chunks_consumed)) {
      return best;
    }
  }

  // if all available tables are already loaded, then schedule proportionally
  std::unordered_map<std::string, double> worker_weights;

  // calc ratio of data being loaded per table / total data being loaded
  double total_bytes_loading = std::accumulate(
      tables_being_loaded.begin(), tables_being_loaded.end(),
      static_cast<size_t>(0),
      [&worker_weights](size_t size,
                        const std::pair<std::string, size_t> &table_size) {
        worker_weights[table_size.first] += table_size.second;
        return size + table_size.second;
      });

  if (total_bytes_loading > 0) {
    for (auto &it : worker_weights) {
      it.second = it.second / total_bytes_loading;
    }
  }

  std::vector<std::pair<Candidate, double>> candidate_weights;

  // calc ratio of data available per table / total data available
  double total_bytes_available = std::accumulate(
      tables_in_progress.begin(), tables_in_progress.end(),
      static_cast<size_t>(0),
      [](size_t size, auto it) { return size + (*it)->bytes_available(); });
  if (total_bytes_available > 0) {
    for (auto it = tables_in_progress.begin(); it != tables_in_progress.end();
         ++it) {
      candidate_weights.emplace_back(
          *it, static_cast<double>((**it)->bytes_available()) /
                   total_bytes_available);
    }
  } else {
    assert(0);
    return tables_in_progress.front();
  }

  // pick a chunk from the table that has the biggest difference between both
  double best_diff = 0;
  Candidate best = tables_in_progress.front();

  for (const auto &cand : candidate_weights) {
    const auto it = worker_weights.find((*cand.first)->key());
    const auto weight = it == worker_weights.end() ? 0.0 : it->second;
    const auto d = cand.second - weight;

    if (d > best_diff) {
      best_diff = d;
      best = cand.first;
    }
  }

  return best;
}

bool Dump_reader::next_table_chunk(
    const std::unordered_multimap<std::string, size_t> &tables_being_loaded,
    std::string *out_schema, std::string *out_table, std::string *out_partition,
    bool *out_chunked, size_t *out_chunk_index, size_t *out_chunks_total,
    std::unique_ptr<mysqlshdk::storage::IFile> *out_file,
    size_t *out_chunk_size, shcore::Dictionary_t *out_options) {
  auto iter = schedule_chunk_proportionally(
      tables_being_loaded, &m_tables_with_data, m_options.threads_count());

  if (iter != m_tables_with_data.end()) {
    *out_schema = (*iter)->owner->schema;
    *out_table = (*iter)->owner->table;
    *out_partition = (*iter)->partition;
    *out_chunked = (*iter)->chunked;
    *out_chunk_index = (*iter)->chunks_consumed;

    if ((*iter)->last_chunk_seen) {
      *out_chunks_total = (*iter)->available_chunks.size();
    } else {
      *out_chunks_total = 0;
    }

    const auto &info = (*iter)->available_chunks[*out_chunk_index];
    *out_file = m_dir->file(info->name());
    *out_chunk_size = info->size();
    *out_options = (*iter)->owner->options;

    (*iter)->chunks_consumed++;
    if (!(*iter)->has_data_available()) m_tables_with_data.erase(iter);

    return true;
  }

  return false;
}

bool Dump_reader::next_deferred_index(
    std::string *out_schema, std::string *out_table,
    compatibility::Deferred_statements::Index_info **out_indexes) {
  for (auto &schema : m_contents.schemas) {
    for (auto &table : schema.second->tables) {
      if ((!m_options.load_data() || table.second->all_data_loaded()) &&
          !table.second->indexes_scheduled) {
        table.second->indexes_scheduled = true;
        *out_schema = schema.second->schema;
        *out_table = table.second->table;
        *out_indexes = &table.second->indexes;
        return true;
      }
    }
  }
  return false;
}

bool Dump_reader::next_table_analyze(std::string *out_schema,
                                     std::string *out_table,
                                     std::vector<Histogram> *out_histograms) {
  for (auto &schema : m_contents.schemas) {
    for (auto &table : schema.second->tables) {
      if ((!m_options.load_data() || table.second->all_data_loaded()) &&
          table.second->indexes_created && !table.second->analyze_scheduled) {
        table.second->analyze_scheduled = true;
        *out_schema = schema.second->schema;
        *out_table = table.second->table;
        *out_histograms = table.second->histograms;
        return true;
      }
    }
  }
  return false;
}

bool Dump_reader::data_available() const { return !m_tables_with_data.empty(); }

bool Dump_reader::work_available() const {
  for (auto &schema : m_contents.schemas) {
    for (auto &table : schema.second->tables) {
      if ((m_options.load_data() && !table.second->all_data_scheduled()) ||
          !table.second->indexes_scheduled ||
          !table.second->analyze_scheduled) {
        return true;
      }
    }
  }
  return false;
}

size_t Dump_reader::filtered_data_size() const {
  if (m_dump_status == Status::COMPLETE) {
    return m_filtered_data_size;
  } else {
    return total_data_size();
  }
}

size_t Dump_reader::table_data_size(const std::string &schema,
                                    const std::string &table) const {
  if (const auto s = m_contents.table_data_size.find(schema);
      s != m_contents.table_data_size.end()) {
    if (const auto t = s->second.find(table); t != s->second.end()) {
      return t->second;
    }
  }

  return 0;
}

void Dump_reader::compute_filtered_data_size() {
  m_filtered_data_size = 0;

  for (const auto &schema : m_contents.schemas) {
    const auto s = m_contents.table_data_size.find(schema.first);

    if (s != m_contents.table_data_size.end()) {
      for (const auto &table : schema.second->tables) {
        const auto t = s->second.find(table.second->table);

        if (t != s->second.end()) {
          m_filtered_data_size += t->second;
        }
      }
    }
  }
}

// Scan directory for new files and adds them to the pending file list
void Dump_reader::rescan(dump::Progress_thread *progress_thread) {
  Files files;

  {
    dump::Progress_thread::Stage *stage = nullptr;
    shcore::on_leave_scope finish_stage([&stage]() {
      if (stage) {
        stage->finish();
      }
    });

    if (!m_dir->is_local()) {
      current_console()->print_status(
          "Fetching dump data from remote location...");

      if (progress_thread) {
        stage = progress_thread->start_stage("Listing files");
      }
    }

    files = m_dir->list_files();
  }

  log_debug("Finished listing files, starting rescan");

  m_contents.rescan(m_dir.get(), files, this, progress_thread);

  log_debug("Rescan done");

  if (files.find({"@.done.json"}) != files.end() &&
      m_dump_status != Status::COMPLETE) {
    m_contents.parse_done_metadata(m_dir.get());
    m_dump_status = Status::COMPLETE;
  }

  compute_filtered_data_size();
}

uint64_t Dump_reader::add_deferred_statements(
    const std::string &schema, const std::string &table,
    compatibility::Deferred_statements &&stmts) {
  const auto s = m_contents.schemas.find(schema);

  if (s == m_contents.schemas.end()) {
    throw std::logic_error("Unable to find schema " + schema +
                           " for adding index");
  }

  const auto t = s->second->tables.find(table);

  if (t == s->second->tables.end()) {
    throw std::logic_error("Unable to find table " + table + " in schema " +
                           schema + " for adding index");
  }

  // if indexes are not going to be recreated, we're marking them as already
  // created
  t->second->indexes_scheduled = t->second->indexes_created =
      !m_options.load_deferred_indexes() || stmts.index_info.empty();
  t->second->indexes = std::move(stmts.index_info);

  std::move(stmts.foreign_keys.begin(), stmts.foreign_keys.end(),
            std::back_inserter(s->second->foreign_key_queries));

  if (!stmts.secondary_engine.empty()) {
    s->second->queries_on_schema_end.emplace_back(
        std::move(stmts.secondary_engine));
  }

  return t->second->indexes.size();
}

void Dump_reader::replace_target_schema(const std::string &schema) {
  if (1 != m_contents.schemas.size()) {
    current_console()->print_error(
        "The 'schema' option can only be used when loading a single schema, "
        "but " +
        std::to_string(m_contents.schemas.size()) + " will be loaded.");

    throw std::invalid_argument("Invalid option: schema.");
  }

  const auto info = m_contents.schemas.begin()->second;
  m_contents.schemas.clear();
  m_contents.schemas.emplace(schema, info);

  m_schema_override = {schema, info->schema};
  info->schema = schema;

  for (const auto &table : info->tables) {
    table.second->schema = schema;
  }

  for (auto &view : info->views) {
    view.schema = schema;
  }

  for (const auto &table : m_tables_with_data) {
    table->owner->schema = schema;
    table->owner->options->set("schema", shcore::Value(schema));
  }
}

void Dump_reader::validate_options() {
  if (m_options.load_users() && !m_contents.has_users) {
    current_console()->print_warning(
        "The 'loadUsers' option is set to true, but the dump does not contain "
        "the user data.");
  }

  if (table_only()) {
    // old version of dumpTables() - no schema SQL is available
    if (m_options.target_schema().empty()) {
      // user didn't provide the new schema name, we cannot proceed
      current_console()->print_error(
          "The dump was created by an older version of the util." +
          shcore::get_member_name("dumpTables",
                                  shcore::current_naming_style()) +
          "() function and needs to be loaded into an existing schema. "
          "Please set the target schema using the 'schema' option.");

      throw std::invalid_argument("The target schema was not specified.");
    } else {
      const auto result = m_options.base_session()->queryf(
          "SELECT SCHEMA_NAME FROM information_schema.schemata WHERE "
          "SCHEMA_NAME=?",
          m_options.target_schema());

      if (nullptr == result->fetch_one()) {
        throw std::invalid_argument("The specified schema does not exist.");
      }
    }
  }

  if (should_create_pks() && !m_options.load_ddl()) {
    current_console()->print_warning(
        "The 'createInvisiblePKs' option is set to true, but the 'loadDdl' "
        "option is false, Primary Keys are not going to be created.");
  }
}

std::string Dump_reader::Table_info::script_name() const {
  return dump::get_table_filename(basename);
}

std::string Dump_reader::Table_info::triggers_script_name() const {
  return dump::get_table_data_filename(basename, "triggers.sql");
}

bool Dump_reader::Table_info::ready() const {
  return md_done && (!has_sql || sql_seen);
}

std::string Dump_reader::Table_info::metadata_name() const {
  // schema@table.json
  return dump::get_table_data_filename(basename, "json");
}

bool Dump_reader::Table_info::should_fetch_metadata_file(
    const Files &files) const {
  if (!md_done && files.find(metadata_name()) != files.end()) {
    return true;
  }

  return false;
}

void Dump_reader::Table_info::update_metadata(const std::string &data,
                                              Dump_reader *reader) {
  auto md = parse_metadata(data, metadata_name());

  Table_data_info di;
  di.owner = this;

  has_sql = md->get_bool("includesDdl", true);
  di.has_data = md->get_bool("includesData", true);

  options = md->get_map("options");

  if (options) {
    // "compression" and "primaryIndex" are not used by the chunk importer
    // and need to be removed
    // these were misplaced in the options dictionary, code is kept for
    // backward compatibility
    options->erase("compression");

    if (options->has_key("primaryIndex")) {
      auto index = options->get_string("primaryIndex");

      if (!index.empty()) {
        primary_index.emplace_back(std::move(index));
      }

      options->erase("primaryIndex");
    }

    // chunk importer uses characterSet instead of defaultCharacterSet
    if (options->has_key("defaultCharacterSet")) {
      options->set("characterSet", options->at("defaultCharacterSet"));
      options->erase("defaultCharacterSet");
    } else {
      // By default, we use the character set from the source DB
      options->set("characterSet",
                   shcore::Value(reader->default_character_set()));
    }
  }

  di.extension = md->get_string("extension", "tsv");
  di.chunked = md->get_bool("chunking", false);

  if (md->has_key("primaryIndex")) {
    primary_index = to_vector_of_strings(md->get_array("primaryIndex"));
  }

  {
    const auto trigger_list = md->get_array("triggers");
    has_triggers = trigger_list && !trigger_list->empty();
  }

  auto histogram_list = md->get_array("histograms");

  if (histogram_list) {
    for (const auto &h : *histogram_list) {
      auto histogram = h.as_map();
      if (histogram) {
        histograms.emplace_back(
            Histogram{histogram->get_string("column"),
                      static_cast<size_t>(histogram->get_int("buckets"))});
      }
    }
  }

  {
    // maps partition names to basenames
    const auto basenames = md->get_map("basenames");

    if (basenames && !basenames->empty()) {
      for (const auto &p : *basenames) {
        auto copy = di;

        copy.partition = p.first;
        copy.basename = p.second.as_string();

        data_info.emplace_back(std::move(copy));
      }
    } else {
      di.basename = basename;
      data_info.emplace_back(std::move(di));
    }
  }

  reader->on_table_metadata_parsed(*this);
  md_done = true;
}

void Dump_reader::Table_info::rescan(const Files &files) {
  // MD not included for tables if data is not dumped

  // check for the sql file for the schema
  if (!sql_seen) {
    if (files.find(script_name()) != files.end()) {
      sql_seen = true;
    }
  }

  if (!has_triggers) {
    if (files.find(triggers_script_name()) != files.end()) {
      has_triggers = true;
    }
  }
}

bool Dump_reader::Table_info::all_data_scheduled() const {
  for (const auto &di : data_info) {
    if (!di.data_scheduled()) return false;
  }

  return true;
}

bool Dump_reader::Table_info::all_data_loaded() const {
  for (const auto &di : data_info) {
    if (!di.data_loaded()) return false;
  }

  return true;
}

void Dump_reader::Table_data_info::rescan_data(const Files &files,
                                               Dump_reader *reader) {
  bool found_data = false;

  const auto try_to_add_chunk = [&files, reader, &found_data,
                                 this](auto &&... params) {
    static_assert(sizeof...(params) == 0 || sizeof...(params) == 2);

    // default values for non-chunked case
    size_t idx = 0;
    bool last_chunk = true;

    if constexpr (sizeof...(params) == 2) {
      std::tie(idx, last_chunk) = std::forward_as_tuple(params...);
    }

    const auto it = files.find(
        dump::get_table_data_filename(basename, extension, params...));

    if (it == files.end()) {
      return false;
    }

    if (idx >= available_chunks.size()) {
      available_chunks.resize(idx + 1, {});
    }

    available_chunks[idx] = *it;
    reader->m_contents.dump_size += it->size();
    ++chunks_seen;
    found_data = true;

    if (last_chunk) {
      last_chunk_seen = true;
    }

    return true;
  };

  // in older versions of Shell, empty tables would create a single data file
  // using the non-chunked name format even if file was supposed to be chunked
  try_to_add_chunk();

  if (chunked) {
    if (chunks_seen < available_chunks.size()) {
      // search for chunks that we're still missing
      for (size_t i = 0, s = available_chunks.size(); i < s; ++i) {
        // If we have found the last chunk, then the last element in the array
        // will already be set. If we didn't find the last chunk, then the last
        // element in the array is going to be a regular chunk.
        if (!available_chunks[i]) {
          try_to_add_chunk(i, false);
        }
      }
    }

    if (!last_chunk_seen) {
      // We don't know how many chunks there are, try to find a consecutive
      // sequence of chunk IDs. If sequence stops, check if the final chunk
      // follows, then always break as we don't want to scan too many files.
      for (size_t i = available_chunks.size(); i < files.size(); ++i) {
        if (!try_to_add_chunk(i, false)) {
          try_to_add_chunk(i, true);
          break;
        }
      }
    }
  }

  if (found_data) reader->m_tables_with_data.insert(this);
}

std::string Dump_reader::View_info::script_name() const {
  return dump::get_table_filename(basename);
}

std::string Dump_reader::View_info::pre_script_name() const {
  return dump::get_table_data_filename(basename, "pre.sql");
}

void Dump_reader::View_info::rescan(const Files &files) {
  // check for the sql file for the schema
  if (!sql_seen) {
    if (files.find(script_name()) != files.end()) {
      sql_seen = true;
    }
  }

  if (!sql_pre_seen) {
    if (files.find(pre_script_name()) != files.end()) {
      sql_pre_seen = true;
    }
  }
}

bool Dump_reader::Schema_info::ready() const {
  if (md_done && (!has_sql || sql_seen)) {
    return true;
  }

  return false;
}

std::string Dump_reader::Schema_info::script_name() const {
  return dump::get_schema_filename(basename);
}

std::string Dump_reader::Schema_info::metadata_name() const {
  // schema.json
  return dump::get_schema_filename(basename, "json");
}

bool Dump_reader::Schema_info::should_fetch_metadata_file(
    const Files &files) const {
  if (!md_loaded && files.find(metadata_name()) != files.end()) {
    return true;
  }

  return false;
}

void Dump_reader::Schema_info::update_metadata(const std::string &data,
                                               Dump_reader *reader) {
  auto md = parse_metadata(data, metadata_name());

  has_sql = md->get_bool("includesDdl", true);
  has_view_sql = md->get_bool("includesViewsDdl", has_sql);
  has_data = md->get_bool("includesData", true);

  shcore::Dictionary_t basenames = md->get_map("basenames");

  if (md->has_key("tables")) {
    for (const auto &t : *md->get_array("tables")) {
      if (reader->include_table(schema, t.as_string())) {
        auto info = std::make_shared<Table_info>();

        info->schema = schema;
        info->table = t.as_string();

        if (basenames->has_key(info->table))
          info->basename = basenames->get_string(info->table);
        else
          info->basename = basename + "@" + info->table;

        // if tables are not going to be analysed, we're marking them as already
        // analysed
        info->analyze_scheduled = info->analyze_finished =
            reader->m_options.analyze_tables() ==
            Load_dump_options::Analyze_table_mode::OFF;

        tables.emplace(info->table, std::move(info));
      }
    }
    log_debug("%s has %zi tables", schema.c_str(), tables.size());
  }

  if (md->has_key("views")) {
    for (const auto &v : *md->get_array("views")) {
      if (reader->include_table(schema, v.as_string())) {
        View_info info;

        info.schema = schema;
        info.table = v.as_string();
        if (basenames->has_key(info.table))
          info.basename = basenames->get_string(info.table);
        else
          info.basename = basename + "@" + info.table;

        views.emplace_back(std::move(info));
      }
    }
    log_debug("%s has %zi views", schema.c_str(), views.size());
  }

  if (md->has_key("functions"))
    function_names = to_vector_of_strings(md->get_array("functions"));

  if (md->has_key("procedures"))
    procedure_names = to_vector_of_strings(md->get_array("procedures"));

  if (md->has_key("events"))
    event_names = to_vector_of_strings(md->get_array("events"));

  md_loaded = true;
  reader->on_metadata_parsed();
}

void Dump_reader::Schema_info::rescan(mysqlshdk::storage::IDirectory *dir,
                                      const Files &files, Dump_reader *reader,
                                      shcore::Thread_pool *pool) {
  log_debug("Scanning contents of schema '%s'", schema.c_str());

  if (md_loaded && !md_done) {
    // we have the list of tables, so check for their metadata and data files
    std::size_t files_to_fetch = 0;
    for (auto &t : tables) {
      if (!t.second->ready()) {
        if (t.second->should_fetch_metadata_file(files)) {
          // metadata available, fetch it, parse it, then rescan asynchronously
          reader->on_metadata_available();
          ++files_to_fetch;

          pool->add_task(
              [dir, mdpath = t.second->metadata_name()]() {
                return fetch_file(dir, mdpath);
              },
              [table = t.second.get(), &files, reader](std::string &&data) {
                table->update_metadata(data, reader);
                table->rescan(files);
              });
        } else {
          // metadata already parsed, scan synchronously
          t.second->rescan(files);
        }
      }
    }

    if (files_to_fetch > 0 && !dir->is_local()) {
      log_info("Fetching %zu table metadata files for schema `%s`...",
               files_to_fetch, schema.c_str());
    }

    for (auto &v : views) {
      if (!v.ready()) {
        v.rescan(files);
      }
    }
  }

  // check for the sql file for the schema
  if (!sql_seen) {
    if (files.find(script_name()) != files.end()) {
      sql_seen = true;
    }
  }
}

void Dump_reader::Schema_info::check_if_ready() {
  if (md_loaded && !md_done) {
    bool children_done = true;

    for (const auto &t : tables) {
      if (!t.second->ready()) {
        children_done = false;
        break;
      }
    }

    if (children_done) {
      for (const auto &v : views) {
        if (!v.ready()) {
          children_done = false;
          break;
        }
      }
    }

    md_done = children_done;

    if (md_done) {
      log_debug("All metadata for schema `%s` was scanned", schema.c_str());
    }
  }
}

void Dump_reader::Schema_info::rescan_data(const Files &files,
                                           Dump_reader *reader) {
  for (auto &t : tables) {
    for (auto &di : t.second->data_info) {
      if (!di.data_dumped()) {
        di.rescan_data(files, reader);
      }
    }
  }
}

bool Dump_reader::Dump_info::ready() const {
  return sql && post_sql && has_users == !!users_sql && md_done;
}

void Dump_reader::Dump_info::rescan(mysqlshdk::storage::IDirectory *dir,
                                    const Files &files, Dump_reader *reader,
                                    dump::Progress_thread *progress_thread) {
  if (!sql && files.find({"@.sql"}) != files.end()) {
    sql = std::make_unique<std::string>(fetch_file(dir, "@.sql"));
  }
  if (!post_sql && files.find({"@.post.sql"}) != files.end()) {
    post_sql = std::make_unique<std::string>(fetch_file(dir, "@.post.sql"));
  }
  if (has_users && !users_sql && files.find({"@.users.sql"}) != files.end()) {
    users_sql = std::make_unique<std::string>(fetch_file(dir, "@.users.sql"));
  }

  if (!md_done) {
    rescan_metadata(dir, files, reader, progress_thread);
  }

  rescan_data(files, reader);
}

void Dump_reader::Dump_info::rescan_metadata(
    mysqlshdk::storage::IDirectory *dir, const Files &files,
    Dump_reader *reader, dump::Progress_thread *progress_thread) {
  const auto thread_pool_ptr = reader->create_thread_pool();
  const auto pool = thread_pool_ptr.get();

  std::atomic<uint64_t> task_producers{0};

  const auto maybe_shutdown = [&task_producers, pool]() {
    if (0 == --task_producers) {
      pool->tasks_done();
    }
  };

  ++task_producers;

  dump::Progress_thread::Stage *stage = nullptr;
  shcore::on_leave_scope finish_stage([&stage]() {
    if (stage) {
      stage->finish();
    }
  });

  if (progress_thread) {
    dump::Progress_thread::Progress_config config;
    config.current = [reader]() { return reader->metadata_parsed(); };
    config.total = [reader]() { return reader->metadata_available(); };
    config.is_total_known = [&task_producers]() { return 0 == task_producers; };

    stage =
        progress_thread->start_stage("Scanning metadata", std::move(config));
  }

  pool->start_threads();

  for (const auto &s : schemas) {
    if (!s.second->ready()) {
      if (s.second->should_fetch_metadata_file(files)) {
        // metadata available, fetch it, parse it, then rescan asynchronously
        reader->on_metadata_available();
        ++task_producers;

        pool->add_task(
            [dir, mdpath = s.second->metadata_name()]() {
              return fetch_file(dir, mdpath);
            },
            [&maybe_shutdown, schema = s.second.get(), dir, &files, reader,
             pool](std::string &&data) {
              shcore::on_leave_scope cleanup(
                  [&maybe_shutdown]() { maybe_shutdown(); });

              schema->update_metadata(data, reader);
              schema->rescan(dir, files, reader, pool);
            });
      } else {
        // metadata already parsed, scan synchronously
        s.second->rescan(dir, files, reader, pool);
      }
    }
  }

  maybe_shutdown();
  pool->process();

  check_if_ready();
}

void Dump_reader::Dump_info::check_if_ready() {
  if (!md_done) {
    bool children_done = true;

    for (const auto &s : schemas) {
      s.second->check_if_ready();

      if (!s.second->ready()) {
        children_done = false;
      }
    }

    md_done = children_done;

    if (md_done) log_debug("All metadata for dump was scanned");
  }
}

void Dump_reader::Dump_info::rescan_data(const Files &files,
                                         Dump_reader *reader) {
  for (const auto &s : schemas) {
    if (s.second->ready()) {
      log_debug("Scanning data of schema '%s'", s.second->schema.c_str());

      s.second->rescan_data(files, reader);
    }
  }
}

void Dump_reader::Dump_info::parse_done_metadata(
    mysqlshdk::storage::IDirectory *dir) {
  shcore::Dictionary_t metadata = fetch_metadata(dir, "@.done.json");
  log_info("Dump %s is complete", dir->full_path().masked().c_str());

  if (metadata) {
    if (metadata->has_key("dataBytes")) {
      data_size = metadata->get_uint("dataBytes");
    } else {
      log_warning(
          "Dump metadata file @.done.json does not contain dataBytes "
          "information");
    }

    if (metadata->has_key("tableDataBytes")) {
      for (const auto &schema : *metadata->get_map("tableDataBytes")) {
        for (const auto &table : *schema.second.as_map()) {
          table_data_size[schema.first][table.first] = table.second.as_uint();
        }
      }
    } else {
      log_warning(
          "Dump metadata file @.done.json does not contain tableDataBytes "
          "information");
    }

    // only exists in 1.0.1+
    if (metadata->has_key("chunkFileBytes")) {
      for (const auto &file : *metadata->get_map("chunkFileBytes")) {
        chunk_sizes[file.first] = file.second.as_uint();
      }
    }
  } else {
    log_warning("Dump metadata file @.done.json is invalid");
  }
}

std::unique_ptr<mysqlshdk::storage::IFile>
Dump_reader::create_progress_file_handle() const {
  return m_options.create_progress_file_handle();
}

void Dump_reader::show_metadata() const {
  if (m_options.show_metadata()) {
    const auto metadata = shcore::make_dict(
        "Dump_metadata",
        shcore::make_dict("Binlog_file", binlog_file(), "Binlog_position",
                          binlog_position(), "Executed_GTID_set",
                          gtid_executed()));

    const auto console = current_console();

    console->println();
    console->println(shcore::Value(metadata).yaml());
  }
}

bool Dump_reader::should_create_pks() const {
  return m_options.should_create_pks(m_contents.create_invisible_pks);
}

std::unique_ptr<shcore::Thread_pool> Dump_reader::create_thread_pool() const {
  auto threads = m_options.threads_count();

  if (!m_dir->is_local()) {
    // in case of remote dumps we're using more threads, in order to compensate
    // for download times
    threads *= 4;
  }

  return std::make_unique<shcore::Thread_pool>(
      m_options.background_threads_count(threads));
}

void Dump_reader::on_table_metadata_parsed(const Table_info &info) {
  const auto partitions = info.data_info.size();
  assert(partitions > 0);

  if (info.data_info[0].has_data) {
    ++m_tables_to_load;
    m_tables_and_partitions_to_load += partitions;

    if (partitions > 1) {
      m_dump_has_partitions = true;
    }
  }

  on_metadata_parsed();
}

bool Dump_reader::include_schema(const std::string &schema) const {
  return m_options.include_schema(override_schema(schema));
}

bool Dump_reader::include_table(const std::string &schema,
                                const std::string &table) const {
  return m_options.include_table(override_schema(schema), table);
}

bool Dump_reader::include_event(const std::string &schema,
                                const std::string &event) const {
  return m_options.include_event(override_schema(schema), event);
}

bool Dump_reader::include_routine(const std::string &schema,
                                  const std::string &routine) const {
  return m_options.include_routine(override_schema(schema), routine);
}

bool Dump_reader::include_trigger(const std::string &schema,
                                  const std::string &table,
                                  const std::string &trigger) const {
  return m_options.include_trigger(override_schema(schema), table, trigger);
}

const std::string &Dump_reader::override_schema(const std::string &s) const {
  if (m_schema_override) {
    const auto &value = m_schema_override.value();
    return value.first == s ? value.second : s;
  } else {
    return s;
  }
}

void Dump_reader::on_chunk_loaded(const std::string &schema,
                                  const std::string &table,
                                  const std::string &partition) {
  const auto t = find_table(schema, table, "chunk was loaded");

  for (auto &tdi : t->data_info) {
    if (tdi.partition == partition) {
      ++tdi.chunks_loaded;
      return;
    }
  }

  throw std::logic_error("Unable to find partition " + partition +
                         " of table " + table + " in schema " + schema +
                         " whose chunk was loaded");
}

void Dump_reader::on_index_end(const std::string &schema,
                               const std::string &table) {
  find_table(schema, table, "indexes were created")->indexes_created = true;
}

void Dump_reader::on_analyze_end(const std::string &schema,
                                 const std::string &table) {
  find_table(schema, table, "analysis was finished")->analyze_finished = true;
}

Dump_reader::Table_info *Dump_reader::find_table(const std::string &schema,
                                                 const std::string &table,
                                                 const char *context) {
  const auto s = m_contents.schemas.find(schema);

  if (s == m_contents.schemas.end()) {
    throw std::logic_error("Unable to find schema " + schema + " whose " +
                           context);
  }

  const auto t = s->second->tables.find(table);

  if (t == s->second->tables.end()) {
    throw std::logic_error("Unable to find table " + table + " in schema " +
                           schema + " whose " + context);
  }

  return t->second.get();
}

}  // namespace mysqlsh
