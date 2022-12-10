/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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

#include "modules/util/mod_util.h"

#include <memory>
#include <set>
#include <utility>
#include <vector>
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "modules/util/dump/dump_instance.h"
#include "modules/util/dump/dump_instance_options.h"
#include "modules/util/dump/dump_schemas.h"
#include "modules/util/dump/dump_schemas_options.h"
#include "modules/util/dump/dump_tables.h"
#include "modules/util/dump/dump_tables_options.h"
#include "modules/util/dump/export_table.h"
#include "modules/util/dump/export_table_options.h"
#include "modules/util/import_table/import_table.h"
#include "modules/util/import_table/import_table_options.h"
#include "modules/util/json_importer.h"
#include "modules/util/load/dump_loader.h"
#include "modules/util/load/load_dump_options.h"
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/document_parser.h"
#include "mysqlshdk/libs/utils/log_sql.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "mysqlshdk/libs/utils/ssl_keygen.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
REGISTER_HELP_GLOBAL_OBJECT(util, shellapi);
REGISTER_HELP(UTIL_GLOBAL_BRIEF,
              "Global object that groups miscellaneous tools like upgrade "
              "checker and JSON import.");
REGISTER_HELP(UTIL_BRIEF,
              "Global object that groups miscellaneous tools like upgrade "
              "checker and JSON import.");

Util::Util(shcore::IShell_core *owner)
    : Extensible_object("util", "util", true), _shell_core(*owner) {
  expose("checkForServerUpgrade", &Util::check_for_server_upgrade,
         "?connectionData", "?options")
      ->cli();
  expose("importJson", &Util::import_json, "path", "?options")->cli();
  expose("importTable", &Util::import_table_file, "path", "?options")
      ->cli(false);
  expose("importTable", &Util::import_table_files, "files", "?options")->cli();
  expose("dumpSchemas", &Util::dump_schemas, "schemas", "outputUrl", "?options")
      ->cli();
  expose("dumpTables", &Util::dump_tables, "schema", "tables", "outputUrl",
         "?options")
      ->cli();
  expose("dumpInstance", &Util::dump_instance, "outputUrl", "?options")->cli();
  expose("exportTable", &Util::export_table, "table", "outputUrl", "?options")
      ->cli();
  expose("loadDump", &Util::load_dump, "url", "?options")->cli();
}

REGISTER_HELP_FUNCTION(checkForServerUpgrade, util);
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_BRIEF,
              "Performs series of tests on specified MySQL server to check if "
              "the upgrade process will succeed.");
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_PARAM,
              "@param connectionData Optional the connection data to server to "
              "be checked");
REGISTER_HELP(
    UTIL_CHECKFORSERVERUPGRADE_PARAM1,
    "@param options Optional dictionary of options to modify tool behaviour.");
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL,
              "If no connectionData is specified tool will try to establish "
              "connection using data from current session.");
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL1,
              "Tool behaviour can be modified with following options:");

REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL2,
              "@li configPath - full path to MySQL server configuration file.");

REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL3,
              "@li outputFormat - value can be either TEXT (default) or JSON.");

REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL4,
              "@li targetVersion - version to which upgrade will be checked "
              "(default=" MYSH_VERSION ")");

REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL5,
              "@li password - password for connection.");

REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL6, "${TOPIC_CONNECTION_DATA}");

/**
 * \ingroup util
 * $(UTIL_CHECKFORSERVERUPGRADE_BRIEF)
 *
 * $(UTIL_CHECKFORSERVERUPGRADE_PARAM)
 * $(UTIL_CHECKFORSERVERUPGRADE_PARAM1)
 *
 * $(UTIL_CHECKFORSERVERUPGRADE_RETURNS)
 *
 * $(UTIL_CHECKFORSERVERUPGRADE_DETAIL)
 *
 * $(UTIL_CHECKFORSERVERUPGRADE_DETAIL1)
 * $(UTIL_CHECKFORSERVERUPGRADE_DETAIL2)
 * $(UTIL_CHECKFORSERVERUPGRADE_DETAIL3)
 * $(UTIL_CHECKFORSERVERUPGRADE_DETAIL4)
 * $(UTIL_CHECKFORSERVERUPGRADE_DETAIL5)
 *
 * \copydoc connection_options
 *
 * Detailed description of the connection data format is available at \ref
 * connection_data
 *
 */
#if DOXYGEN_JS
Undefined Util::checkForServerUpgrade(ConnectionData connectionData,
                                      Dictionary options);
Undefined Util::checkForServerUpgrade(Dictionary options);
#elif DOXYGEN_PY
None Util::check_for_server_upgrade(ConnectionData connectionData,
                                    dict options);
None Util::check_for_server_upgrade(dict options);
#endif

void Util::check_for_server_upgrade(
    const mysqlshdk::db::Connection_options &connection_options,
    const shcore::Option_pack_ref<Upgrade_check_options> &options) {
  auto connection = connection_options;
  if (connection.has_data()) {
    if (!options->password.is_null()) {
      if (connection.has_password()) connection.clear_password();
      connection.set_password(*options->password);
    }
  } else {
    if (!_shell_core.get_dev_session())
      throw shcore::Exception::argument_error(
          "Please connect the shell to the MySQL server to be checked or "
          "specify the server URI as a parameter.");
    connection = _shell_core.get_dev_session()->get_connection_options();
  }

  const auto session =
      establish_session(connection, current_shell_options()->get().wizards);
  mysqlshdk::mysql::Instance instance(session);
  std::unique_ptr<mysqlshdk::mysql::User_privileges> privileges;

  try {
    privileges = instance.get_current_user_privileges(true);
  } catch (const std::runtime_error &e) {
    log_error("Unable to check permissions: %s", e.what());
  } catch (const std::logic_error &e) {
    throw std::runtime_error("Unable to get information about a user");
  }

  Upgrade_check_config config{*options};
  config.set_session(session);
  config.set_user_privileges(privileges.get());

  check_for_upgrade(config);
}

REGISTER_HELP_FUNCTION(importJson, util);
REGISTER_HELP(UTIL_IMPORTJSON_BRIEF,
              "Import JSON documents from file to collection or table in MySQL "
              "Server using X Protocol session.");

REGISTER_HELP(UTIL_IMPORTJSON_PARAM, "@param file Path to JSON documents file");

REGISTER_HELP(UTIL_IMPORTJSON_PARAM1,
              "@param options Optional dictionary with import options");

REGISTER_HELP(UTIL_IMPORTJSON_DETAIL,
              "This function reads standard JSON documents from a file, "
              "however, it also supports converting BSON Data Types "
              "represented using the MongoDB Extended Json (strict mode) into "
              "MySQL values.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL1,
              "The options dictionary supports the following options:");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL2,
              "@li schema: string - name of target schema.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL3,
              "@li collection: string - name of collection where the data will "
              "be imported.");
REGISTER_HELP(
    UTIL_IMPORTJSON_DETAIL4,
    "@li table: string - name of table where the data will be imported.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL5,
              "@li tableColumn: string (default: \"doc\") - name of column in "
              "target table where the imported JSON documents will be stored.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL6,
              "@li convertBsonTypes: bool (default: false) - enables the BSON "
              "data type conversion.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL7,
              "@li convertBsonOid: bool (default: the value of "
              "convertBsonTypes) - enables conversion of the BSON ObjectId "
              "values.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL8,
              "@li extractOidTime: string (default: empty) - creates a new "
              "field based on the ObjectID timestamp. Only valid if "
              "convertBsonOid is enabled.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL9,
              "The following options are valid only when convertBsonTypes is "
              "enabled. They are all boolean flags. ignoreRegexOptions is "
              "enabled by default, rest are disabled by default.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL10,
              "@li ignoreDate: disables conversion of BSON Date values");
REGISTER_HELP(
    UTIL_IMPORTJSON_DETAIL11,
    "@li ignoreTimestamp: disables conversion of BSON Timestamp values");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL12,
              "@li ignoreRegex: disables conversion of BSON Regex values.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL15,
              "@li ignoreRegexOptions: causes regex options to be ignored when "
              "processing a Regex BSON value. This option is only valid if "
              "ignoreRegex is disabled.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL13,
              "@li ignoreBinary: disables conversion of BSON BinData values.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL14,
              "@li decimalAsDouble: causes BSON Decimal values to be imported "
              "as double values.");

REGISTER_HELP(UTIL_IMPORTJSON_DETAIL16,
              "If the schema is not provided, an active schema on the global "
              "session, if set, will be used.");

REGISTER_HELP(UTIL_IMPORTJSON_DETAIL17,
              "The collection and the table options cannot be combined. If "
              "they are not provided, the basename of the file without "
              "extension will be used as target collection name.");

REGISTER_HELP(
    UTIL_IMPORTJSON_DETAIL18,
    "If the target collection or table does not exist, they are created, "
    "otherwise the data is inserted into the existing collection or table.");

REGISTER_HELP(UTIL_IMPORTJSON_DETAIL19,
              "The tableColumn implies the use of the table option and cannot "
              "be combined "
              "with the collection option.");

REGISTER_HELP(UTIL_IMPORTJSON_DETAIL20, "<b>BSON Data Type Processing.</b>");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL21,
              "If only convertBsonOid is enabled, no conversion will be done "
              "on the rest of the BSON Data Types.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL22,
              "To use extractOidTime, it should be set to a name which will "
              "be used to insert an additional field into the main document. "
              "The value of the new field will be the timestamp obtained from "
              "the ObjectID value. Note that this will be done only for an "
              "ObjectID value associated to the '_id' field of the main "
              "document.");
REGISTER_HELP(
    UTIL_IMPORTJSON_DETAIL23,
    "NumberLong and NumberInt values will be converted to integer values.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL24,
              "NumberDecimal values are imported as strings, unless "
              "decimalAsDouble is enabled.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL25,
              "Regex values will be converted to strings containing the "
              "regular expression. The regular expression options are ignored "
              "unless ignoreRegexOptions is disabled. When ignoreRegexOptions "
              "is disabled the regular expression will be converted to the "
              "form: /@<regex@>/@<options@>.");

REGISTER_HELP(UTIL_IMPORTJSON_THROWS, "Throws ArgumentError when:");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS1, "@li Option name is invalid");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS2,
              "@li Required options are not set and cannot be deduced");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS3,
              "@li Shell is not connected to MySQL Server using X Protocol");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS4,
              "@li Schema is not provided and there is no active schema on the "
              "global session");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS5,
              "@li Both collection and table are specified");

REGISTER_HELP(UTIL_IMPORTJSON_THROWS6, "Throws LogicError when:");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS7,
              "@li Path to JSON document does not exists or is not a file");

REGISTER_HELP(UTIL_IMPORTJSON_THROWS8, "Throws RuntimeError when:");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS9, "@li The schema does not exists");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS10, "@li MySQL Server returns an error");

REGISTER_HELP(UTIL_IMPORTJSON_THROWS11, "Throws InvalidJson when:");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS12, "@li JSON document is ill-formed");

const shcore::Option_pack_def<Import_json_options>
    &Import_json_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Import_json_options>()
          .optional("schema", &Import_json_options::schema)
          .optional("collection", &Import_json_options::collection)
          .optional("table", &Import_json_options::table)
          .optional("tableColumn", &Import_json_options::table_column)
          .include(&Import_json_options::doc_reader);

  return opts;
}

/**
 * \ingroup util
 *
 * $(UTIL_IMPORTJSON_BRIEF)
 *
 * $(UTIL_IMPORTJSON_PARAM)
 * $(UTIL_IMPORTJSON_PARAM1)
 *
 * $(UTIL_IMPORTJSON_DETAIL)
 *
 * $(UTIL_IMPORTJSON_DETAIL1)
 * $(UTIL_IMPORTJSON_DETAIL2)
 * $(UTIL_IMPORTJSON_DETAIL3)
 * $(UTIL_IMPORTJSON_DETAIL4)
 * $(UTIL_IMPORTJSON_DETAIL5)
 * $(UTIL_IMPORTJSON_DETAIL6)
 * $(UTIL_IMPORTJSON_DETAIL7)
 * $(UTIL_IMPORTJSON_DETAIL8)
 *
 * $(UTIL_IMPORTJSON_DETAIL9)
 * $(UTIL_IMPORTJSON_DETAIL10)
 * $(UTIL_IMPORTJSON_DETAIL11)
 * $(UTIL_IMPORTJSON_DETAIL12)
 * $(UTIL_IMPORTJSON_DETAIL13)
 * $(UTIL_IMPORTJSON_DETAIL14)
 * $(UTIL_IMPORTJSON_DETAIL15)
 *
 * $(UTIL_IMPORTJSON_DETAIL16)
 *
 * $(UTIL_IMPORTJSON_DETAIL17)
 *
 * $(UTIL_IMPORTJSON_DETAIL18)
 *
 * $(UTIL_IMPORTJSON_DETAIL19)
 *
 * $(UTIL_IMPORTJSON_DETAIL20)
 *
 * $(UTIL_IMPORTJSON_DETAIL21)
 *
 * $(UTIL_IMPORTJSON_DETAIL22)
 *
 * $(UTIL_IMPORTJSON_DETAIL23)
 *
 * $(UTIL_IMPORTJSON_DETAIL24)
 *
 * $(UTIL_IMPORTJSON_DETAIL25)
 *
 * $(UTIL_IMPORTJSON_THROWS)
 * $(UTIL_IMPORTJSON_THROWS1)
 * $(UTIL_IMPORTJSON_THROWS2)
 * $(UTIL_IMPORTJSON_THROWS3)
 * $(UTIL_IMPORTJSON_THROWS4)
 * $(UTIL_IMPORTJSON_THROWS5)
 *
 * $(UTIL_IMPORTJSON_THROWS6)
 * $(UTIL_IMPORTJSON_THROWS7)
 *
 * $(UTIL_IMPORTJSON_THROWS8)
 * $(UTIL_IMPORTJSON_THROWS9)
 * $(UTIL_IMPORTJSON_THROWS10)
 *
 * $(UTIL_IMPORTJSON_THROWS11)
 * $(UTIL_IMPORTJSON_THROWS12)
 */
#if DOXYGEN_JS
Undefined Util::importJson(String file, Dictionary options);
#elif DOXYGEN_PY
None Util::import_json(str file, dict options);
#endif

void Util::import_json(
    const std::string &file,
    const shcore::Option_pack_ref<Import_json_options> &options) {
  auto shell_session = _shell_core.get_dev_session();

  if (!shell_session) {
    throw shcore::Exception::runtime_error(
        "Please connect the shell to the MySQL server.");
  }

  auto node_type = shell_session->get_node_type();
  if (node_type.compare("X") != 0) {
    throw shcore::Exception::runtime_error(
        "An X Protocol session is required for JSON import.");
  }

  Connection_options connection_options =
      shell_session->get_connection_options();

  std::shared_ptr<mysqlshdk::db::mysqlx::Session> xsession =
      mysqlshdk::db::mysqlx::Session::create();

  if (current_shell_options()->get().trace_protocol) {
    xsession->enable_protocol_trace(true);
  }
  xsession->connect(connection_options);

  Prepare_json_import prepare{xsession};

  if (!options->schema.empty()) {
    prepare.schema(options->schema);
  } else if (!shell_session->get_current_schema().empty()) {
    prepare.schema(shell_session->get_current_schema());
  } else {
    throw std::runtime_error(
        "There is no active schema on the current session, the target schema "
        "for the import operation must be provided in the options.");
  }

  prepare.path(file);

  if (!options->table.empty()) {
    prepare.table(options->table);
  }

  if (!options->table_column.empty()) {
    prepare.column(options->table_column);
  }

  if (!options->collection.empty()) {
    if (!options->table_column.empty()) {
      throw std::invalid_argument(
          "tableColumn cannot be used with collection.");
    }
    prepare.collection(options->collection);
  }

  // Validate provided parameters and build Json_importer object.
  auto importer = prepare.build();

  auto console = mysqlsh::current_console();
  console->print_info(
      prepare.to_string() + " in MySQL Server at " +
      connection_options.as_uri(mysqlshdk::db::uri::formats::only_transport()) +
      "\n");

  importer.set_print_callback([](const std::string &msg) -> void {
    mysqlsh::current_console()->print(msg);
  });

  try {
    importer.load_from(options->doc_reader);
  } catch (...) {
    importer.print_stats();
    throw;
  }
  importer.print_stats();
}

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_AWS_COMMON_OPTIONS, R"*(
@li <b>s3BucketName</b>: string (default: not set) - Name of the AWS S3 bucket
to use. The bucket must already exist.
@li <b>s3CredentialsFile</b>: string (default: not set) - Use the specified AWS
<b>credentials</b> file instead of the one at the default location.
@li <b>s3ConfigFile</b>: string (default: not set) - Use the specified AWS
<b>config</b> file instead of the one at the default location.
@li <b>s3Profile</b>: string (default: not set) - Use the specified AWS profile
instead of the <b>default</b> one.
@li <b>s3EndpointOverride</b>: string (default: not set) - Use the specified AWS
S3 API endpoint instead of the default one.)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_AWS_COMMON_OPTION_DETAILS, R"*(
If the <b>s3BucketName</b> option is used, the dump is stored in the specified
AWS S3 bucket. Connection is established using default local AWS configuration
paths and profiles, unless overridden. The directory structure is simulated
within the object name.

The <b>s3CredentialsFile</b>, <b>s3ConfigFile</b>, <b>s3Profile</b> and
<b>s3EndpointOverride</b> options cannot be used if the <b>s3BucketName</b>
option is not set or set to an empty string.

<b>Handling of the AWS settings</b>

-# The following settings are read from the <b>config</b> file for the
specified profile:
@li <b>aws_access_key_id</b>
@li <b>aws_secret_access_key</b>
@li <b>aws_session_token</b>
@li <b>region</b>
-# The following settings are read from the <b>credentials</b> file for the
specified profile:
@li <b>aws_access_key_id</b>
@li <b>aws_secret_access_key</b>
@li <b>aws_session_token</b>
-# If there are credentials in both <b>credentials</b> and <b>config</b> files
for the specified profile, the keys in the <b>credentials</b> file take
precedence.
-# If either <b>aws_access_key_id</b> or <b>aws_secret_access_key</b> is
missing, an exception is thrown.
-# If <b>aws_session_token</b> is missing, or is empty, it is not used to
authenticate the user.
-# If <b>region</b> is missing, or is empty, a default value of <b>us-east-1</b>
is used instead.)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_AWS_COMMON_OPTION_DETAILS, R"*(
<b>Dumping to a Bucket in the AWS S3 Object Storage</b>

${TOPIC_UTIL_AWS_COMMON_OPTION_DETAILS})*");

REGISTER_HELP_DETAIL_TEXT(IMPORT_EXPORT_URL_DETAIL, R"*(
@li <b>/path/to/file</b> - Path to a locally or remotely (e.g. in OCI Object
Storage) accessible file or directory
@li <b>file:///path/to/file</b> - Path to a locally accessible file or directory
@li <b>http[s]://host.domain[:port]/path/to/file</b> - Location of a remote file
accessible through HTTP(s) (<<<importTable>>>() only)

If the <b>osBucketName</b> option is given, the path argument must specify a
plain path in that OCI (Oracle Cloud Infrastructure) Object Storage bucket.

The OCI configuration profile is located through the oci.profile and
oci.configFile global shell options and can be overridden with ociProfile and
ociConfigFile, respectively.

If the <b>s3BucketName</b> option is given, the path argument must specify a
plain path in that AWS S3 bucket.)*");

REGISTER_HELP_DETAIL_TEXT(IMPORT_EXPORT_OCI_OPTIONS_DETAIL, R"*(
<b>OCI Object Storage Options</b>

@li <b>osBucketName</b>: string (default: not set) - Name of the OCI Object
Storage bucket to use. The bucket must already exist.
@li <b>osNamespace</b>: string (default: not set) - Specifies the namespace
where the bucket is located, if not given it will be obtained
using the tenancy id on the OCI configuration.
@li <b>ociConfigFile</b>: string (default: not set) - Override oci.configFile
shell option, to specify the path to the OCI configuration file.
@li <b>ociProfile</b>: string (default: not set) - Override oci.profile shell
option, to specify the name of the OCI profile to use.
)*");

REGISTER_HELP_FUNCTION(importTable, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_IMPORTTABLE, R"*(
Import table dump stored in files to target table using LOAD DATA LOCAL
INFILE calls in parallel connections.

@param files Path or list of paths to files with user data.
Path name can contain a glob pattern with wildcard '*' and/or '?'.
All selected files must be chunks of the same target table.
@param options Optional dictionary with import options

The scheme part of a filename contains infomation about the transport backend.
Supported transport backends are: file://, http://, https://.
If the scheme part of a filename is omitted, then file:// transport backend
will be chosen.

Supported filename formats:
${IMPORT_EXPORT_URL_DETAIL}

Options dictionary:
@li <b>schema</b>: string (default: current shell active schema) - Name of
target schema
@li <b>table</b>: string (default: filename without extension) - Name of target
table
@li <b>columns</b>: array of strings and/or integers (default: empty array) -
This option takes an array of column names as its value. The order of the column
names indicates how to match data file columns with table columns.
Use non-negative integer `i` to capture column value into user variable @@i.
With user variables, the decodeColumns option enables you to perform preprocessing
transformations on their values before assigning the result to columns.
@li <b>fieldsTerminatedBy</b>: string (default: "\t") - This option has the same
meaning as the corresponding clause for LOAD DATA INFILE.
@li <b>fieldsEnclosedBy</b>: char (default: '') - This option has the same meaning
as the corresponding clause for LOAD DATA INFILE.
@li <b>fieldsEscapedBy</b>: char (default: '\\') - This option has the same meaning
as the corresponding clause for LOAD DATA INFILE.
@li <b>fieldsOptionallyEnclosed</b>: bool (default: false) - Set to true if the
input values are not necessarily enclosed within quotation marks specified by
<b>fieldsEnclosedBy</b> option. Set to false if all fields are quoted by
character specified by <b>fieldsEnclosedBy</b> option.
@li <b>linesTerminatedBy</b>: string (default: "\n") - This option has the same
meaning as the corresponding clause for LOAD DATA INFILE. For example, to import
Windows files that have lines terminated with carriage return/linefeed pairs,
use --lines-terminated-by="\r\n". (You might have to double the backslashes,
depending on the escaping conventions of your command interpreter.)
See Section 13.2.7, "LOAD DATA INFILE Syntax".
@li <b>replaceDuplicates</b>: bool (default: false) - If true, input rows that
have the same value for a primary key or unique index as an existing row will be
replaced, otherwise input rows will be skipped.
@li <b>threads</b>: int (default: 8) - Use N threads to sent file chunks to the
server.
@li <b>bytesPerChunk</b>: string (minimum: "131072", default: "50M") - Send
bytesPerChunk (+ bytes to end of the row) in single LOAD DATA call. Unit
suffixes, k - for Kilobytes (n * 1'000 bytes), M - for Megabytes (n * 1'000'000
bytes), G - for Gigabytes (n * 1'000'000'000 bytes), bytesPerChunk="2k" - ~2
kilobyte data chunk will send to the MySQL Server. Not available for multiple
files import.
@li <b>maxBytesPerTransaction</b>: string (default: empty) - Specifies the
maximum number of bytes that can be loaded from a dump data file per single
LOAD DATA statement. If a content size of data file is bigger than this option
value, then multiple LOAD DATA statements will be executed per single file.
If this option is not specified explicitly, dump data file sub-chunking will be
disabled. Use this option with value less or equal to global variable
'max_binlog_cache_size' to mitigate "MySQL Error 1197 (HY000): Multi-statement
transaction required more than 'max_binlog_cache_size' bytes of storage".
Unit suffixes: k (Kilobytes), M (Megabytes), G (Gigabytes).
Minimum value: 4096.
@li <b>maxRate</b>: string (default: "0") - Limit data send throughput to
maxRate in bytes per second per thread.
maxRate="0" - no limit. Unit suffixes, k - for Kilobytes (n * 1'000 bytes),
M - for Megabytes (n * 1'000'000 bytes), G - for Gigabytes (n * 1'000'000'000
bytes), maxRate="2k" - limit to 2 kilobytes per second.
@li <b>showProgress</b>: bool (default: true if stdout is a tty, false
otherwise) - Enable or disable import progress information.
@li <b>skipRows</b>: int (default: 0) - Skip first n rows of the data in the
file. You can use this option to skip an initial header line containing column
names.
@li <b>dialect</b>: enum (default: "default") - Setup fields and lines options
that matches specific data file format. Can be used as base dialect and
customized with fieldsTerminatedBy, fieldsEnclosedBy, fieldsOptionallyEnclosed,
fieldsEscapedBy and linesTerminatedBy options. Must be one of the following
values: default, csv, tsv, json or csv-unix.
@li <b>decodeColumns</b>: map (default: not set) - a map between columns names
and SQL expressions to be applied on the loaded
data. Column value captured in 'columns' by integer is available as user
variable '@@i', where `i` is that integer. Requires 'columns' to be set.
@li <b>characterSet</b>: string (default: not set) -
Interpret the information in the input file using this character set
encoding. characterSet set to "binary" specifies "no conversion". If not set,
the server will use the character set indicated by the character_set_database
system variable to interpret the information in the file.
@li <b>sessionInitSql</b>: list of strings (default: []) - execute the given
list of SQL statements in each session about to load data.

${IMPORT_EXPORT_OCI_OPTIONS_DETAIL}

<b>AWS S3 Object Storage Options</b>

${TOPIC_UTIL_AWS_COMMON_OPTIONS}

${TOPIC_UTIL_AWS_COMMON_OPTION_DETAILS}

<b>dialect</b> predefines following set of options fieldsTerminatedBy (FT),
fieldsEnclosedBy (FE), fieldsOptionallyEnclosed (FOE), fieldsEscapedBy (FESC)
and linesTerminatedBy (LT) in following manner:
@li default: no quoting, tab-separated, lf line endings.
(LT=@<LF@>, FESC='\', FT=@<TAB@>, FE=@<empty@>, FOE=false)
@li csv: optionally quoted, comma-separated, crlf line endings.
(LT=@<CR@>@<LF@>, FESC='\', FT=",", FE='&quot;', FOE=true)
@li tsv: optionally quoted, tab-separated, crlf line endings.
(LT=@<CR@>@<LF@>, FESC='\', FT=@<TAB@>, FE='&quot;', FOE=true)
@li json: one JSON document per line.
(LT=@<LF@>, FESC=@<empty@>, FT=@<LF@>, FE=@<empty@>, FOE=false)
@li csv-unix: fully quoted, comma-separated, lf line endings.
(LT=@<LF@>, FESC='\', FT=",", FE='&quot;', FOE=false)

If the <b>schema</b> is not provided, an active schema on the global session, if
set, will be used.

If the input values are not necessarily enclosed within <b>fieldsEnclosedBy</b>,
set <b>fieldsOptionallyEnclosed</b> to true.

If you specify one separator that is the same as or a prefix of another, LOAD
DATA INFILE cannot interpret the input properly.

Connection options set in the global session, such as compression, ssl-mode, etc.
are used in parallel connections.

Each parallel connection sets the following session variables:
@li SET SQL_MODE = ''; -- Clear SQL Mode
@li SET NAMES ?; -- Set to characterSet option if provided by user.
@li SET unique_checks = 0
@li SET foreign_key_checks = 0
@li SET SESSION TRANSACTION ISOLATION LEVEL READ UNCOMMITTED
)*");
/**
 * \ingroup util
 *
 * $(UTIL_IMPORTTABLE_BRIEF)
 *
 * $(UTIL_IMPORTTABLE)
 *
 * Example input data for dialects:
 * @li default:
 * @code{.unparsed}
 * 1<TAB>20.1000<TAB>foo said: "Where is my bar?"<LF>
 * 2<TAB>-12.5000<TAB>baz said: "Where is my \<TAB> char?"<LF>
 * @endcode
 * @li csv:
 * @code{.unparsed}
 * 1,20.1000,"foo said: \"Where is my bar?\""<CR><LF>
 * 2,-12.5000,"baz said: \"Where is my <TAB> char?\""<CR><LF>
 * @endcode
 * @li tsv:
 * @code{.unparsed}
 * 1<TAB>20.1000<TAB>"foo said: \"Where is my bar?\""<CR><LF>
 * 2<TAB>-12.5000<TAB>"baz said: \"Where is my <TAB> char?\""<CR><LF>
 * @endcode
 * @li json:
 * @code{.unparsed}
 * {"id_int": 1, "value_float": 20.1000, "text_text": "foo said: \"Where is my
 * bar?\""}<LF>
 * {"id_int": 2, "value_float": -12.5000, "text_text": "baz said: \"Where is my
 * \u000b char?\""}<LF>
 * @endcode
 * @li csv-unix:
 * @code{.unparsed}
 * "1","20.1000","foo said: \"Where is my bar?\""<LF>
 * "2","-12.5000","baz said: \"Where is my <TAB> char?\""<LF>
 * @endcode
 *
 * Examples of <b>decodeColumns</b> usage:
 * @li Preprocess column2:
 * @code
 *     util.importTable('file.txt', {
 *       table: 't1',
 *       columns: ['column1', 1],
 *       decodeColumns: {'column2': '@1 / 100'}
 *     });
 * @endcode
 * is equivalent to:
 * @code
 *     LOAD DATA LOCAL INFILE 'file.txt'
 *     INTO TABLE `t1` (column1, @var1)
 *     SET `column2` = @var/100;
 * @endcode
 *
 * @li Skip columns:
 * @code
 *     util.importTable('file.txt', {
 *       table: 't1',
 *       columns: ['column1', 1, 'column2', 2, 'column3']
 *     });
 * @endcode
 * is equivalent to:
 * @code
 *     LOAD DATA LOCAL INFILE 'file.txt'
 *     INTO TABLE `t1` (column1, @1, column2, @2, column3);
 * @endcode
 *
 * @li Generate values for columns:
 * @code
 *     util.importTable('file.txt', {
 *       table: 't1',
 *       columns: [1, 2],
 *       decodeColumns: {
 *         'a': '@1',
 *         'b': '@2',
 *         'sum': '@1 + @2',
 *         'mul': '@1 * @2',
 *         'pow': 'POW(@1, @2)'
 *       }
 *     });
 * @endcode
 * is equivalent to:
 * @code
 *     LOAD DATA LOCAL INFILE 'file.txt'
 *     INTO TABLE `t1` (@1, @2)
 *     SET
 *       `a` = @1,
 *       `b` = @2,
 *       `sum` = @1 + @2,
 *       `mul` = @1 * @2,
 *       `pow` = POW(@1, @2);
 * @endcode
 */
#if DOXYGEN_JS
Undefined Util::importTable(List files, Dictionary options);
#elif DOXYGEN_PY
None Util::import_table(list files, dict options);
#endif
void Util::import_table_file(
    const std::string &filename,
    const shcore::Option_pack_ref<import_table::Import_table_option_pack>
        &options) {
  import_table_files({filename}, options);
}

void Util::import_table_files(
    const std::vector<std::string> &files,
    const shcore::Option_pack_ref<import_table::Import_table_option_pack>
        &options) {
  using import_table::Import_table;
  using mysqlshdk::utils::format_bytes;

  import_table::Import_table_options opt(*options);

  opt.set_filenames(files);

  auto shell_session = _shell_core.get_dev_session();
  if (!shell_session || !shell_session->is_open() ||
      shell_session->get_node_type().compare("mysql") != 0) {
    throw shcore::Exception::runtime_error(
        "A classic protocol session is required to perform this operation.");
  }

  opt.set_base_session(shell_session->get_core_session());

  opt.validate();

  volatile bool interrupt = false;
  shcore::Interrupt_handler intr_handler([&interrupt]() -> bool {
    mysqlsh::current_console()->print_note(
        "Interrupted by user. Cancelling...");
    interrupt = true;
    return false;
  });

  Import_table importer(opt);
  importer.interrupt(&interrupt);

  auto console = mysqlsh::current_console();
  console->print_info(opt.target_import_info());

  importer.import();

  const bool thread_thrown_exception = importer.any_exception();
  if (!thread_thrown_exception) {
    console->print_info(importer.import_summary());
  }

  console->print_info(importer.rows_affected_info());
  importer.rethrow_exceptions();
}

namespace {

std::shared_ptr<shcore::Log_sql> log_sql_for_dump_and_load() {
  // copy the storage
  auto storage = current_shell_options()->get();

  // log all errors
  if (shcore::Log_sql::parse_log_level(storage.log_sql) <
      shcore::Log_sql::Log_level::ERROR) {
    storage.log_sql = "error";
  }

  return std::make_shared<shcore::Log_sql>(storage);
}

}  // namespace

REGISTER_HELP_FUNCTION(loadDump, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_LOADDUMP, R"*(
Loads database dumps created by MySQL Shell.

@param url defines the location of the dump to be loaded
@param options Optional dictionary with load options

Depending on how the dump was created, the url identifies the location and in
some cases the access method to the dump, i.e. for dumps to be loaded using
pre-authenticated requests (PAR). Allowed values:

@li <b>/path/to/folder</b> - to load a dump from local storage
@li <b>/oci/bucket/path</b> - to load a dump from OCI Object Storage using an
OCI profile
@li <b>/aws/bucket/path</b> - to load a dump from AWS S3 Object Storage using
the AWS settings stored in the <b>credentials</b> and <b>config</b> files
@li <b>PAR to the dump manifest</b> - to load a dump from OCI Object Storage
created with the ociParManifest option
@li <b>PAR to the dump location</b> - to load a dump from OCI Object Storage
using a single PAR

<<<loadDump>>>() will load a dump from the specified path. It transparently
handles compressed files and directly streams data when loading from remote
storage (currently HTTP, OCI Object Storage and AWS S3 Object Storage). If the
'waitDumpTimeout' option is set, it will load a dump on-the-fly, loading table
data chunks as the dumper produces them.

Table data will be loaded in parallel using the configured number of threads
(4 by default). Multiple threads per table can be used if the dump was created
with table chunking enabled. Data loads are scheduled across threads in a way
that tries to maximize parallelism, while also minimizing lock contention from
concurrent loads to the same table. If there are more tables than threads,
different tables will be loaded per thread, larger tables first. If there are
more threads than tables, then chunks from larger tables will be proportionally
assigned more threads.

LOAD DATA LOCAL INFILE is used to load table data and thus, the 'local_infile'
MySQL global setting must be enabled.

<b>Resuming</b>

The load command will store progress information into a file for each step of
the loading process, including successfully completed and interrupted/failed
ones. If that file already exists, its contents will be used to skip steps that
have already been completed and retry those that failed or didn't start yet.

When resuming, table chunks that have started being loaded but didn't finish are
loaded again. Duplicate rows are discarded by the server. Tables that do not
have unique keys are truncated before the load is resumed.

IMPORTANT: Resuming assumes that no changes have been made to the partially
loaded data between the failure and the retry. Resuming after external changes
has undefined behavior and may lead to data loss.

The progress state file has a default name of load-progress.@<server_uuid@>.json
and is written to the same location as the dump. If 'progressFile' is specified,
progress will be written to either a local file at the given path, or, if the
HTTP(S) scheme is used, to a remote file using HTTP PUT requests. Setting it to
'' will disable progress tracking and resuming.

If the 'resetProgress' option is enabled, progress information from previous
load attempts of the dump to the destination server is discarded and the load
is restarted. You may use this option to retry loading the whole dump from the
beginning. However, changes made to the database are not reverted, so previously
loaded objects should be manually dropped first.

Options dictionary:

@li <b>analyzeTables</b>: "off", "on", "histogram" (default: off) - If 'on',
executes ANALYZE TABLE for all tables, once loaded. If set to 'histogram', only
tables that have histogram information stored in the dump will be analyzed. This
option can be used even if all 'load' options are disabled.
@li <b>backgroundThreads</b>: int (default not set) - Number of additional
threads to use to fetch contents of metadata and DDL files. If not set, loader
will use the value of the <b>threads</b> option in case of a local dump, or four
times that value in case on a non-local dump.
@li <b>characterSet</b>: string (default taken from dump) - Overrides
the character set to be used for loading dump data. By default, the same
character set used for dumping will be used (utf8mb4 if not set on dump).
@li <b>createInvisiblePKs</b>: bool (default taken from dump) - Automatically
create an invisible Primary Key for each table which does not have one. By
default, set to true if dump was created with <b>create_invisible_pks</b>
compatibility option, false otherwise. Requires server 8.0.24 or newer.
@li <b>deferTableIndexes</b>: "off", "fulltext", "all" (default: fulltext) -
If "all", creation of "all" indexes except PRIMARY is deferred until after
table data is loaded, which in many cases can reduce load times. If "fulltext",
only full-text indexes will be deferred.
@li <b>dryRun</b>: bool (default: false) - Scans the dump and prints everything
that would be performed, without actually doing so.
@li <b>excludeEvents</b>: array of strings (default not set) - Skip loading
specified events from the dump. Strings are in format <b>schema</b>.<b>event</b>,
quoted using backtick characters when required.
@li <b>excludeRoutines</b>: array of strings (default not set) - Skip loading
specified routines from the dump. Strings are in format <b>schema</b>.<b>routine</b>,
quoted using backtick characters when required.
@li <b>excludeSchemas</b>: array of strings (default not set) - Skip loading
specified schemas from the dump.
@li <b>excludeTables</b>: array of strings (default not set) - Skip loading
specified tables from the dump. Strings are in format <b>schema</b>.<b>table</b>,
quoted using backtick characters when required.
@li <b>excludeTriggers</b>: array of strings (default not set) - Skip loading
specified triggers from the dump. Strings are in format <b>schema</b>.<b>table</b>
(all triggers from the specified table) or <b>schema</b>.<b>table</b>.<b>trigger</b>
(the individual trigger), quoted using backtick characters when required.
@li <b>excludeUsers</b>: array of strings (default not set) - Skip loading
specified users from the dump. Each user is in the format of
'user_name'[@'host']. If the host is not specified, all the accounts with the
given user name are excluded.
@li <b>ignoreExistingObjects</b>: bool (default false) - Load the dump even if
it contains objects that already exist in the target database.
@li <b>ignoreVersion</b>: bool (default false) - Load the dump even if the
major version number of the server where it was created is different from where
it will be loaded.
@li <b>includeEvents</b>: array of strings (default not set) - Loads only the
specified events from the dump. Strings are in format <b>schema</b>.<b>event</b>,
quoted using backtick characters when required. By default, all events are
included.
@li <b>includeRoutines</b>: array of strings (default not set) - Loads only the
specified routines from the dump. Strings are in format <b>schema</b>.<b>routine</b>,
quoted using backtick characters when required. By default, all routines are
included.
@li <b>includeSchemas</b>: array of strings (default not set) - Loads only the
specified schemas from the dump. By default, all schemas are included.
@li <b>includeTables</b>: array of strings (default not set) - Loads only the
specified tables from the dump. Strings are in format <b>schema</b>.<b>table</b>,
quoted using backtick characters when required. By default, all tables from all
schemas are included.
@li <b>includeTriggers</b>: array of strings (default not set) - Loads only the
specified triggers from the dump. Strings are in format <b>schema</b>.<b>table</b>
(all triggers from the specified table) or <b>schema</b>.<b>table</b>.<b>trigger</b>
(the individual trigger), quoted using backtick characters when required. By
default, all triggers are included.
@li <b>includeUsers</b>: array of strings (default not set) - Load only the
specified users from the dump. Each user is in the format of
'user_name'[@'host']. If the host is not specified, all the accounts with the
given user name are included. By default, all users are included.
@li <b>loadData</b>: bool (default: true) - Loads table data from the dump.
@li <b>loadDdl</b>: bool (default: true) - Executes DDL/SQL scripts in the
dump.
@li <b>loadIndexes</b>: bool (default: true) - use together with
‘deferTableIndexes’ to control whether secondary indexes should be recreated
at the end of the load. Useful when loading DDL and data separately.
@li <b>loadUsers</b>: bool (default: false) - Executes SQL scripts for user
accounts, roles and grants contained in the dump. Note: statements for the
current user will be skipped.
@li <b>maxBytesPerTransaction</b>: string (default taken from dump) - Specifies
the maximum number of bytes that can be loaded from a dump data file per single
LOAD DATA statement. Supports unit suffixes: k (kilobytes), M (Megabytes), G
(Gigabytes). Minimum value: 4096. If this option is not specified explicitly,
the value of the <b>bytesPerChunk</b> dump option is used, but only in case of
the files with data size greater than <b>1.5 * bytesPerChunk</b>.
@li <b>progressFile</b>: path (default: load-progress.@<server_uuid@>.progress)
- Stores load progress information in the given local file path.
@li <b>resetProgress</b>: bool (default: false) - Discards progress information
of previous load attempts to the destination server and loads the whole dump
again.
@li <b>schema</b>: string (default not set) - Load the dump into the given
schema. This option can only be used when loading just one schema, (either only
one schema was dumped, or schema filters result in only one schema).
@li <b>sessionInitSql</b>: list of strings (default: []) - execute the given
list of SQL statements in each session about to load data.
@li <b>showMetadata</b>: bool (default: false) - Displays the metadata
information stored in the dump files, i.e. binary log file name and position.
@li <b>showProgress</b>: bool (default: true if stdout is a tty, false
otherwise) - Enable or disable import progress information.
@li <b>skipBinlog</b>: bool (default: false) - Disables the binary log
for the MySQL sessions used by the loader (set sql_log_bin=0).
@li <b>threads</b>: int (default: 4) - Number of threads to use to import table
data.
@li <b>updateGtidSet</b>: "off", "replace", "append" (default: off) - if set to
a value other than 'off' updates GTID_PURGED by either replacing its contents
or appending to it the gtid set present in the dump.
@li <b>waitDumpTimeout</b>: float (default: 0) - Loads a dump while it's still
being created. Once all uploaded tables are processed the command will either
wait for more data, the dump is marked as completed or the given timeout (in
seconds) passes.
<= 0 disables waiting.
@li <b>sessionInitSql</b>: list of strings (default: []) - execute the given
list of SQL statements in each session about to load data.
${TOPIC_UTIL_DUMP_OCI_COMMON_OPTIONS}
${TOPIC_UTIL_AWS_COMMON_OPTIONS}

Connection options set in the global session, such as compression, ssl-mode, etc.
are inherited by load sessions.

Examples:
<br>
@code
util.<<<loadDump>>>('sakila_dump')

util.<<<loadDump>>>('mysql/sales', {
    'osBucketName': 'mybucket',    // OCI Object Storage bucket
    'waitDumpTimeout': 1800        // wait for new data for up to 30mins
})
@endcode


<b>Loading a dump using Pre-authenticated Requests (PAR)</b>

When a dump is created in OCI Object Storage, it is possible to load it using a
single pre-authenticated request which gives access to the location of the dump.
The requirements for this PAR include:

@li Permits object reads
@li Enables object listing

Given a dump located at a bucket root and a PAR created for the bucket, the
dump can be loaded by providing the PAR as the url parameter:

Example:
<br>
@code
Dump Location: root of 'test' bucket

util.<<<loadDump>>>(
  'https://objectstorage.*.oraclecloud.com/p/*/n/main/b/test/o/', {
    'progressFile': 'load_progress.txt'
  }
)
@endcode

Given a dump located at some folder within a bucket and a PAR created for the
given folder, the dump can be loaded by providing the PAR and the prefix as the
url parameter:

Example:
<br>
@code
Dump Location: folder 'dump' at the 'test' bucket
PAR created using the 'dump/' prefix.

util.<<<loadDump>>>(
  'https://objectstorage.*.oraclecloud.com/p/*/n/main/b/test/o/dump/', {
    'progressFile': 'load_progress.txt'
  }
)
@endcode

In both of the above cases the load is done using pure HTTP GET requests and the
progressFile option is mandatory.

A legacy method to create a dump loadable through PAR is still supported, this
is done by using the ociParManifest option when creating the dump. When this is
enabled, a manifest file "@.manifest.json" will be generated, to be used as the
entry point to load the dump using a PAR to this file.

When using a Manifest PAR to load a dump, the progressFile option is mandatory.

To store the progress on dump location, create an ObjectReadWrite PAR to the
desired progress file (it does not need to exist), it should be located on
the same location of the "@.manifest.json" file. Finally specify the PAR URL
on the progressFile option.

Example:
<br>
@code
Dump Location: root of 'test' bucket:

util.<<<loadDump>>>(
  'https://objectstorage.*.oraclecloud.com/p/*/n/main/b/test/o/@.manifest.json',
  { 'progressFile': 'load_progress.txt' }
)
@endcode
)*");
/**
 * \ingroup util
 *
 * $(UTIL_LOADDUMP_BRIEF)
 *
 * $(UTIL_LOADDUMP)
 */
#if DOXYGEN_JS
Undefined Util::loadDump(String url, Dictionary options) {}
#elif DOXYGEN_PY
None Util::load_dump(str url, dict options) {}
#endif
void Util::load_dump(
    const std::string &url,
    const shcore::Option_pack_ref<Load_dump_options> &options) {
  auto session = _shell_core.get_dev_session();
  if (!session || !session->is_open()) {
    throw std::runtime_error(
        "An open session is required to perform this operation.");
  }

  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.loadDump()"};

  Load_dump_options opt = *options;
  opt.set_url(url);
  opt.set_session(session->get_core_session(), session->get_current_schema());
  opt.validate();

  Dump_loader loader(opt);

  shcore::Interrupt_handler intr_handler([&loader]() -> bool {
    loader.interrupt();
    return false;
  });

  auto console = mysqlsh::current_console();
  console->print_info(opt.target_import_info());

  loader.run();
}

REGISTER_HELP_TOPIC_TEXT(TOPIC_UTIL_DUMP_COMPATIBILITY_OPTION, R"*(
<b>MySQL Database Service Compatibility</b>

The MySQL Database Service has a few security related restrictions that
are not present in a regular, on-premise instance of MySQL. In order to make it
easier to load existing databases into the Service, the dump commands in the
MySQL Shell has options to detect potential issues and in some cases, to
automatically adjust your schema definition to be compliant.

The <b>ocimds</b> option, when set to true, will perform schema checks for
most of these issues and abort the dump if any are found. The <<<loadDump>>>()
command will also only allow loading dumps that have been created with the
"ocimds" option enabled.

Some issues found by the <b>ocimds</b> option may require you to manually
make changes to your database schema before it can be loaded into the MySQL
Database Service. However, the <b>compatibility</b> option can be used to
automatically modify the dumped schema SQL scripts, resolving some of these
compatibility issues. You may pass one or more of the following values to
the "compatibility" option.

<b>create_invisible_pks</b> - Each table which does not have a Primary Key will
have one created when the dump is loaded. The following Primary Key is added
to the table:
@code
`my_row_id` BIGINT UNSIGNED AUTO_INCREMENT INVISIBLE PRIMARY KEY
@endcode
At the time of the release of MySQL Shell 8.0.24, dumps created with this value
cannot be used with Inbound Replication into an MySQL Database Service instance
with High Availability. Mutually exclusive with the <b>ignore_missing_pks</b>
value.

<b>force_innodb</b> - The MySQL Database Service requires use of the InnoDB
storage engine. This option will modify the ENGINE= clause of CREATE TABLE
statements that use incompatible storage engines and replace them with InnoDB.
It will also remove the ROW_FORMAT=FIXED option, as it is not supported by the
InnoDB storage engine.

<b>ignore_missing_pks</b> - Ignore errors caused by tables which do not have
Primary Keys. Dumps created with this value cannot be used in MySQL Database
Service instance with High Availability. Mutually exclusive with the
<b>create_invisible_pks</b> value.

<b>skip_invalid_accounts</b> - Skips accounts which do not have a password or
use authentication methods (plugins) not supported by the MySQL Database Service.

<b>strip_definers</b> - Strips the "DEFINER=account" clause from views, routines,
events and triggers. The MySQL Database Service requires special privileges to
create these objects with a definer other than the user loading the schema.
By stripping the DEFINER clause, these objects will be created with that default
definer. Views and routines will additionally have their SQL SECURITY clause
changed from DEFINER to INVOKER. If this characteristic is missing, SQL SECURITY
INVOKER clause will be added. This ensures that the access permissions of the
account querying or calling these are applied, instead of the user that created
them. This should be sufficient for most users, but if your database security
model requires that views and routines have more privileges than their invoker,
you will need to manually modify the schema before loading it.

Please refer to the MySQL manual for details about DEFINER and SQL SECURITY.

<b>strip_restricted_grants</b> - Certain privileges are restricted in the MySQL
Database Service. Attempting to create users granting these privileges would
fail, so this option allows dumped GRANT statements to be stripped of these
privileges.

<b>strip_tablespaces</b> - Tablespaces have some restrictions in the MySQL
Database Service. If you'd like to have tables created in their default
tablespaces, this option will strip the TABLESPACE= option from CREATE TABLE
statements.

Additionally, the following changes will always be made to DDL scripts
when the <b>ocimds</b> option is enabled:

@li <b>DATA DIRECTORY</b>, <b>INDEX DIRECTORY</b> and <b>ENCRYPTION</b> options
in <b>CREATE TABLE</b> statements will be commented out.

At the time of the release of MySQL Shell 8.0.24, in order to use Inbound
Replication into an MySQL Database Service instance with High Availability, all
tables at the source server need to have Primary Keys. This needs to be fixed
manually before running the dump. Starting with MySQL 8.0.23 invisible columns
may be used to add Primary Keys without changing the schema compatibility, for
more information see: https://dev.mysql.com/doc/refman/en/invisible-columns.html.

In order to use MySQL Database Service instance with High Availability, all
tables at the MDS server need to have Primary Keys. This can be fixed
automatically using the <b>create_invisible_pks</b> compatibility value.

Please refer to the MySQL Database Service documentation for more information
about restrictions and compatibility.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_DDL_COMMON_PARAMETERS, R"*(
The <b>outputUrl</b> specifies where the dump is going to be stored.

By default, a local directory is used, and in this case <b>outputUrl</b> can be
prefixed with <b>file://</b> scheme. If a relative path is given, the absolute
path is computed as relative to the current working directory. If the output
directory does not exist but its parent does, it is created. If the output
directory exists, it must be empty. All directories are created with the
following access rights (on operating systems which support them):
<b>rwxr-x---</b>. All files are created with the following access rights (on
operating systems which support them): <b>rw-r-----</b>.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_EXPORT_COMMON_OPTIONS, R"*(
@li <b>maxRate</b>: string (default: "0") - Limit data read throughput to
maximum rate, measured in bytes per second per thread. Use maxRate="0" to set no
limit.
@li <b>showProgress</b>: bool (default: true if stdout is a TTY device, false
otherwise) - Enable or disable dump progress information.
@li <b>defaultCharacterSet</b>: string (default: "utf8mb4") - Character set used
for the dump.)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_OCI_COMMON_OPTIONS, R"*(
@li <b>osBucketName</b>: string (default: not set) - Use specified OCI bucket
for the location of the dump.
@li <b>osNamespace</b>: string (default: not set) - Specifies the namespace
where the bucket is located, if not given it will be obtained
using the tenancy id on the OCI configuration.
@li <b>ociConfigFile</b>: string (default: not set) - Use the specified OCI
configuration file instead of the one at the default location.
@li <b>ociProfile</b>: string (default: not set) - Use the specified OCI profile
instead of the default one.)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_OCI_PAR_COMMON_OPTIONS, R"*(
@li <b>ociParManifest</b>: bool (default: not set) - Enables the generation of
the PAR manifest while the dump operation is being executed.
@li <b>ociParExpireTime</b>: string (default: not set) - Allows defining the
expiration time for the PARs generated when ociParManifest is enabled.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_DDL_COMMON_OPTIONS, R"*(
@li <b>triggers</b>: bool (default: true) - Include triggers for each dumped
table.
@li <b>excludeTriggers</b>: list of strings (default: empty) - List of triggers
to be excluded from the dump in the format of <b>schema</b>.<b>table</b>
(all triggers from the specified table) or
<b>schema</b>.<b>table</b>.<b>trigger</b> (the individual trigger).
@li <b>includeTriggers</b>: list of strings (default: empty) - List of triggers
to be included in the dump in the format of <b>schema</b>.<b>table</b>
(all triggers from the specified table) or
<b>schema</b>.<b>table</b>.<b>trigger</b> (the individual trigger).

@li <b>tzUtc</b>: bool (default: true) - Convert TIMESTAMP data to UTC.

@li <b>consistent</b>: bool (default: true) - Enable or disable consistent data
dumps.
@li <b>ddlOnly</b>: bool (default: false) - Only dump Data Definition Language
(DDL) from the database.
@li <b>dataOnly</b>: bool (default: false) - Only dump data from the database.
@li <b>dryRun</b>: bool (default: false) - Print information about what would be
dumped, but do not dump anything.

@li <b>chunking</b>: bool (default: true) - Enable chunking of the tables.
@li <b>bytesPerChunk</b>: string (default: "64M") - Sets average estimated
number of bytes to be written to each chunk file, enables <b>chunking</b>.
@li <b>threads</b>: int (default: 4) - Use N threads to dump data chunks from
the server.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_DDL_COMPRESSION, R"*(
@li <b>compression</b>: string (default: "zstd") - Compression used when writing
the data dump files, one of: "none", "gzip", "zstd".
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_MDS_COMMON_OPTIONS, R"*(
@li <b>ocimds</b>: bool (default: false) - Enable checks for compatibility with
MySQL Database Service (MDS)
@li <b>compatibility</b>: list of strings (default: empty) - Apply MySQL
Database Service compatibility modifications when writing dump files. Supported
values: "create_invisible_pks", "force_innodb", "ignore_missing_pks",
"skip_invalid_accounts", "strip_definers", "strip_restricted_grants",
"strip_tablespaces".)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_SCHEMAS_COMMON_OPTIONS, R"*(
@li <b>excludeTables</b>: list of strings (default: empty) - List of tables or
views to be excluded from the dump in the format of <b>schema</b>.<b>table</b>.
@li <b>includeTables</b>: list of strings (default: empty) - List of tables or
views to be included in the dump in the format of <b>schema</b>.<b>table</b>.

${TOPIC_UTIL_DUMP_MDS_COMMON_OPTIONS}

@li <b>events</b>: bool (default: true) - Include events from each dumped
schema.
@li <b>excludeEvents</b>: list of strings (default: empty) - List of events
to be excluded from the dump in the format of <b>schema</b>.<b>event</b>.
@li <b>includeEvents</b>: list of strings (default: empty) - List of events
to be included in the dump in the format of <b>schema</b>.<b>event</b>.

@li <b>routines</b>: bool (default: true) - Include functions and stored
procedures for each dumped schema.
@li <b>excludeRoutines</b>: list of strings (default: empty) - List of routines
to be excluded from the dump in the format of <b>schema</b>.<b>routine</b>.
@li <b>includeRoutines</b>: list of strings (default: empty) - List of routines
to be included in the dump in the format of <b>schema</b>.<b>routine</b>.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_SESSION_DETAILS, R"*(
Requires an open, global Shell session, and uses its connection options, such as
compression, ssl-mode, etc., to establish additional connections.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_EXPORT_COMMON_REQUIREMENTS, R"*(
<b>Requirements</b>
@li MySQL Server 5.7 or newer is required.
@li Size limit for individual files uploaded to the OCI or AWS S3 bucket is 1.2 TiB.
@li Columns with data types which are not safe to be stored in text form (i.e.
BLOB) are converted to Base64, hence the size of such columns cannot exceed
approximately 0.74 * <b>max_allowed_packet</b> bytes, as configured through that
system variable at the target server.)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_DDL_COMMON_REQUIREMENTS, R"*(
${TOPIC_UTIL_DUMP_EXPORT_COMMON_REQUIREMENTS}
@li Schema object names must use latin1 or utf8 character set.
@li Only tables which use the InnoDB storage engine are guaranteed to be dumped
with consistent data.)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_SCHEMAS_COMMON_DETAILS, R"*(
${TOPIC_UTIL_DUMP_DDL_COMMON_REQUIREMENTS}

<b>Details</b>

This operation writes SQL files per each schema, table and view dumped, along
with some global SQL files.

Table data dumps are written to TSV files, optionally splitting them into
multiple chunk files.

${TOPIC_UTIL_DUMP_SESSION_DETAILS}

Data dumps cannot be created for the following tables:
@li mysql.apply_status
@li mysql.general_log
@li mysql.schema
@li mysql.slow_log
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_DDL_COMMON_OPTION_DETAILS, R"*(
The names given in the <b>exclude{object}</b> or <b>include{object}</b> options
should be valid MySQL identifiers, quoted using backtick characters when
required.

If the <b>exclude{object}</b> or <b>include{object}</b> options contain an
object which does not exist, or an object which belongs to a schema which does
not exist, it is ignored.

The <b>tzUtc</b> option allows dumping TIMESTAMP data when a server has data in
different time zones or data is being moved between servers with different time
zones.

If the <b>consistent</b> option is set to true, a global read lock is set using
the <b>FLUSH TABLES WITH READ LOCK</b> statement, all threads establish
connections with the server and start transactions using:
@li SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ
@li START TRANSACTION WITH CONSISTENT SNAPSHOT

Once all the threads start transactions, the instance is locked for backup and
the global read lock is released.

If the account used for the dump does not have enough privileges to execute
FLUSH TABLES, LOCK TABLES will be used as a fallback instead. All tables being
dumped, in addition to DDL and GRANT related tables in the mysql schema will
be temporarily locked.

The <b>ddlOnly</b> and <b>dataOnly</b> options cannot both be set to true at
the same time.

The <b>chunking</b> option causes the the data from each table to be split and
written to multiple chunk files. If this option is set to false, table data is
written to a single file.

If the <b>chunking</b> option is set to <b>true</b>, but a table to be dumped
cannot be chunked (for example if it does not contain a primary key or a unique
index), a warning is displayed and chunking is disabled for this table.

The value of the <b>threads</b> option must be a positive number.

Both the <b>bytesPerChunk</b> and <b>maxRate</b> options support unit suffixes:
@li k - for kilobytes,
@li M - for Megabytes,
@li G - for Gigabytes,

i.e. maxRate="2k" - limit throughput to 2000 bytes per second.

The value of the <b>bytesPerChunk</b> option cannot be smaller than "128k".
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_OCI_COMMON_OPTION_DETAILS, R"*(
<b>Dumping to a Bucket in the OCI Object Storage</b>

If the <b>osBucketName</b> option is used, the dump is stored in the specified
OCI bucket, connection is established using the local OCI profile. The directory
structure is simulated within the object name.

The <b>osNamespace</b>, <b>ociConfigFile</b> and <b>ociProfile</b> options
cannot be used if the <b>osBucketName</b> option is set to an empty string.

The <b>osNamespace</b> option overrides the OCI namespace obtained based on the
tenancy ID from the local OCI profile.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_OCI_PAR_OPTION_DETAILS, R"*(
<b>Enabling dump loading using pre-authenticated requests</b>

The <<<loadDump>>> utility supports loading a dump using a pre-authenticated
request (PAR). The simplest way to do this is by providing a PAR to the
location of the dump in a bucket, the PAR must be created with the following
permissions:

@li Permits object reads
@li Enables object listing

The generated URL can be used to load the dump, see \? <<<loadDump>>> for more
details.


Another way to enable loading a dump without requiring an OCI Profile, is to
execute the dump operations enabling the ociParManifest option which will
cause the dump operation automatically generates a PAR for every file
in the dump, and will store them as part of the dump in a file named
"@.manifest.json". The manifest is updated as the dump operation progresses.

Using a PAR with permissions to read the manifest is another option to load
the dump using PAR.

The <b>ociParManifest</b> option cannot be used if <b>osBucketName</b> is not
set.

When creating PARs, an expiration time is required, it can be defined through
the <b>ociParExpireTime</b> option. If the option is not used, a predefined
expiration time will be used equivalent to a week after the dump operation
started. The values assigned to this option should be conformant to RFC3339.

The <b>ociParExpireTime</b> option cannot be used if the <b>ociParManifest</b>
option is not enabled.
)*");

REGISTER_HELP_FUNCTION(exportTable, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_EXPORTTABLE, R"*(
Exports the specified table to the data dump file.

@param table Name of the table to be exported.
@param outputUrl Target file to store the data.
@param options Optional dictionary with the export options.

The value of <b>table</b> parameter should be in form of <b>table</b> or
<b>schema</b>.<b>table</b>, quoted using backtick characters when required. If
schema is omitted, an active schema on the global Shell session is used. If
there is none, an exception is raised.

The <b>outputUrl</b> specifies where the dump is going to be stored.

By default, a local file is used, and in this case <b>outputUrl</b> can be
prefixed with <b>file://</b> scheme. If a relative path is given, the absolute
path is computed as relative to the current working directory. The parent
directory of the output file must exist. If the output file exists, it is going
to be overwritten. The output file is created with the following access rights
(on operating systems which support them): <b>rw-r-----</b>.

<b>The following options are supported:</b>
@li <b>fieldsTerminatedBy</b>: string (default: "\t"), <b>fieldsEnclosedBy</b>:
char (default: ''), <b>fieldsEscapedBy</b>: char (default: '\'),
<b>linesTerminatedBy</b>: string (default: "\n") - These options have the same
meaning as the corresponding clauses for SELECT ... INTO OUTFILE. For more
information use <b>\? SQL Syntax/SELECT</b>, (a session is required).
@li <b>fieldsOptionallyEnclosed</b>: bool (default: false) - Set to true if the
input values are not necessarily enclosed within quotation marks specified by
<b>fieldsEnclosedBy</b> option. Set to false if all fields are quoted by
character specified by <b>fieldsEnclosedBy</b> option.
@li <b>dialect</b>: enum (default: "default") - Setup fields and lines options
that matches specific data file format. Can be used as base dialect and
customized with <b>fieldsTerminatedBy</b>, <b>fieldsEnclosedBy</b>,
<b>fieldsOptionallyEnclosed</b>, <b>fieldsEscapedBy</b> and
<b>linesTerminatedBy</b> options. Must be one of the following values: default,
csv, tsv or csv-unix.

${TOPIC_UTIL_DUMP_EXPORT_COMMON_OPTIONS}
@li <b>compression</b>: string (default: "none") - Compression used when writing
the data dump files, one of: "none", "gzip", "zstd".

${TOPIC_UTIL_DUMP_OCI_COMMON_OPTIONS}

${TOPIC_UTIL_AWS_COMMON_OPTIONS}

${TOPIC_UTIL_DUMP_EXPORT_COMMON_REQUIREMENTS}

<b>Details</b>

This operation writes table data dump to the specified by the user files.

${TOPIC_UTIL_DUMP_SESSION_DETAILS}

<b>Options</b>

The <b>dialect</b> option predefines the set of options fieldsTerminatedBy (FT),
fieldsEnclosedBy (FE), fieldsOptionallyEnclosed (FOE), fieldsEscapedBy (FESC)
and linesTerminatedBy (LT) in the following manner:
@li default: no quoting, tab-separated, LF line endings.
(LT=@<LF@>, FESC='\', FT=@<TAB@>, FE=@<empty@>, FOE=false)
@li csv: optionally quoted, comma-separated, CRLF line endings.
(LT=@<CR@>@<LF@>, FESC='\', FT=",", FE='&quot;', FOE=true)
@li tsv: optionally quoted, tab-separated, CRLF line endings.
(LT=@<CR@>@<LF@>, FESC='\', FT=@<TAB@>, FE='&quot;', FOE=true)
@li csv-unix: fully quoted, comma-separated, LF line endings.
(LT=@<LF@>, FESC='\', FT=",", FE='&quot;', FOE=false)

The <b>maxRate</b> option supports unit suffixes:
@li k - for kilobytes,
@li M - for Megabytes,
@li G - for Gigabytes,

i.e. maxRate="2k" - limit throughput to 2000 bytes per second.

${TOPIC_UTIL_DUMP_OCI_COMMON_OPTION_DETAILS}

${TOPIC_UTIL_DUMP_AWS_COMMON_OPTION_DETAILS}

@throws ArgumentError in the following scenarios:
@li If any of the input arguments contains an invalid value.

@throws RuntimeError in the following scenarios:
@li If there is no open global session.
@li If creating or writing to the output file fails.
)*");

/**
 * \ingroup util
 *
 * $(UTIL_EXPORTTABLE_BRIEF)
 *
 * $(UTIL_EXPORTTABLE)
 */
#if DOXYGEN_JS
Undefined Util::exportTable(String table, String outputUrl, Dictionary options);
#elif DOXYGEN_PY
None Util::export_table(str table, str outputUrl, dict options);
#endif
void Util::export_table(
    const std::string &table, const std::string &file,
    const shcore::Option_pack_ref<dump::Export_table_options> &options) {
  const auto session = _shell_core.get_dev_session();

  if (!session || !session->is_open()) {
    throw std::runtime_error(
        "An open session is required to perform this operation.");
  }

  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.exportTable()"};

  using mysqlsh::dump::Export_table;

  mysqlsh::dump::Export_table_options opts = *options;
  opts.set_table(table);
  opts.set_output_url(file);
  opts.set_session(session->get_core_session());

  Export_table{opts}.run();
}

REGISTER_HELP_FUNCTION(dumpTables, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_DUMPTABLES, R"*(
Dumps the specified tables or views from the given schema to the files in the
target directory.

@param schema Name of the schema that contains tables/views to be dumped.
@param tables List of tables/views to be dumped.
@param outputUrl Target directory to store the dump files.
@param options Optional dictionary with the dump options.

The <b>tables</b> parameter cannot be an empty list.

${TOPIC_UTIL_DUMP_DDL_COMMON_PARAMETERS}

<b>The following options are supported:</b>
@li <b>all</b>: bool (default: false) - Dump all views and tables from the
specified schema.

${TOPIC_UTIL_DUMP_MDS_COMMON_OPTIONS}

${TOPIC_UTIL_DUMP_DDL_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_EXPORT_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_DDL_COMPRESSION}
${TOPIC_UTIL_DUMP_OCI_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_OCI_PAR_COMMON_OPTIONS}

${TOPIC_UTIL_AWS_COMMON_OPTIONS}

${TOPIC_UTIL_DUMP_DDL_COMMON_REQUIREMENTS}
@li Views and triggers to be dumped must not use qualified names to reference
other views or tables.
@li Since util.<<<dumpTables>>>() function does not dump routines, any routines
referenced by the dumped objects are expected to already exist when the dump is
loaded.

<b>Details</b>

This operation writes SQL files per each table and view dumped, along with some
global SQL files. The information about the source schema is also saved, meaning
that when using the util.<<<loadDump>>>() function to load the dump, it is
automatically recreated. Alternatively, dump can be loaded into another existing
schema using the <b>schema</b> option.

Table data dumps are written to TSV files, optionally splitting them into
multiple chunk files.

${TOPIC_UTIL_DUMP_SESSION_DETAILS}

<b>Options</b>

If the <b>all</b> option is set to true and the <b>tables</b> parameter is set
to an empty array, all views and tables from the specified schema are going to
be dumped. If the <b>tables</b> parameter is not set to an empty array, an
exception is thrown.

${TOPIC_UTIL_DUMP_DDL_COMMON_OPTION_DETAILS}
${TOPIC_UTIL_DUMP_COMPATIBILITY_OPTION}
${TOPIC_UTIL_DUMP_OCI_COMMON_OPTION_DETAILS}
${TOPIC_UTIL_DUMP_OCI_PAR_OPTION_DETAILS}

${TOPIC_UTIL_DUMP_AWS_COMMON_OPTION_DETAILS}

@throws ArgumentError in the following scenarios:
@li If any of the input arguments contains an invalid value.

@throws RuntimeError in the following scenarios:
@li If there is no open global session.
@li If creating the output directory fails.
@li If creating or writing to the output file fails.
)*");

/**
 * \ingroup util
 *
 * $(UTIL_DUMPTABLES_BRIEF)
 *
 * $(UTIL_DUMPTABLES)
 */
#if DOXYGEN_JS
Undefined Util::dumpTables(String schema, List tables, String outputUrl,
                           Dictionary options);
#elif DOXYGEN_PY
None Util::dump_tables(str schema, list tables, str outputUrl, dict options);
#endif
void Util::dump_tables(
    const std::string &schema, const std::vector<std::string> &tables,
    const std::string &directory,
    const shcore::Option_pack_ref<dump::Dump_tables_options> &options) {
  const auto session = _shell_core.get_dev_session();

  if (!session || !session->is_open()) {
    throw std::runtime_error(
        "An open session is required to perform this operation.");
  }

  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.dumpTables()"};

  using mysqlsh::dump::Dump_tables;

  mysqlsh::dump::Dump_tables_options opts = *options;
  opts.set_schema(schema);
  opts.set_tables(tables);
  opts.set_output_url(directory);
  opts.set_session(session->get_core_session());

  Dump_tables{opts}.run();
}

REGISTER_HELP_FUNCTION(dumpSchemas, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_DUMPSCHEMAS, R"*(
Dumps the specified schemas to the files in the output directory.

@param schemas List of schemas to be dumped.
@param outputUrl Target directory to store the dump files.
@param options Optional dictionary with the dump options.

The <b>schemas</b> parameter cannot be an empty list.

${TOPIC_UTIL_DUMP_DDL_COMMON_PARAMETERS}

<b>The following options are supported:</b>
${TOPIC_UTIL_DUMP_SCHEMAS_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_DDL_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_EXPORT_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_DDL_COMPRESSION}
${TOPIC_UTIL_DUMP_OCI_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_OCI_PAR_COMMON_OPTIONS}

${TOPIC_UTIL_AWS_COMMON_OPTIONS}

${TOPIC_UTIL_DUMP_SCHEMAS_COMMON_DETAILS}

<b>Options</b>

${TOPIC_UTIL_DUMP_DDL_COMMON_OPTION_DETAILS}
${TOPIC_UTIL_DUMP_COMPATIBILITY_OPTION}
${TOPIC_UTIL_DUMP_OCI_COMMON_OPTION_DETAILS}
${TOPIC_UTIL_DUMP_OCI_PAR_OPTION_DETAILS}

${TOPIC_UTIL_DUMP_AWS_COMMON_OPTION_DETAILS}

@throws ArgumentError in the following scenarios:
@li If any of the input arguments contains an invalid value.

@throws RuntimeError in the following scenarios:
@li If there is no open global session.
@li If creating the output directory fails.
@li If creating or writing to the output file fails.
)*");

/**
 * \ingroup util
 *
 * $(UTIL_DUMPSCHEMAS_BRIEF)
 *
 * $(UTIL_DUMPSCHEMAS)
 */
#if DOXYGEN_JS
Undefined Util::dumpSchemas(List schemas, String outputUrl, Dictionary options);
#elif DOXYGEN_PY
None Util::dump_schemas(list schemas, str outputUrl, dict options);
#endif
void Util::dump_schemas(
    const std::vector<std::string> &schemas, const std::string &directory,
    const shcore::Option_pack_ref<dump::Dump_schemas_options> &options) {
  const auto session = _shell_core.get_dev_session();

  if (!session || !session->is_open()) {
    throw std::runtime_error(
        "An open session is required to perform this operation.");
  }

  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.dumpSchemas()"};

  using mysqlsh::dump::Dump_schemas;

  mysqlsh::dump::Dump_schemas_options opts = *options;
  opts.set_schemas(schemas);
  opts.set_output_url(directory);
  opts.set_session(session->get_core_session());

  Dump_schemas{opts}.run();
}

REGISTER_HELP_FUNCTION(dumpInstance, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_DUMPINSTANCE, R"*(
Dumps the whole database to files in the output directory.

@param outputUrl Target directory to store the dump files.
@param options Optional dictionary with the dump options.

${TOPIC_UTIL_DUMP_DDL_COMMON_PARAMETERS}

<b>The following options are supported:</b>
@li <b>excludeSchemas</b>: list of strings (default: empty) - List of schemas to
be excluded from the dump.
@li <b>includeSchemas</b>: list of strings (default: empty) - List of schemas to
be included in the dump.
${TOPIC_UTIL_DUMP_SCHEMAS_COMMON_OPTIONS}
@li <b>users</b>: bool (default: true) - Include users, roles and grants in the
dump file.
@li <b>excludeUsers</b>: array of strings (default not set) - Skip dumping the
specified users. Each user is in the format of 'user_name'[@'host']. If the host
is not specified, all the accounts with the given user name are excluded.
@li <b>includeUsers</b>: array of strings (default not set) - Dump only the
specified users. Each user is in the format of 'user_name'[@'host']. If the host
is not specified, all the accounts with the given user name are included. By
default, all users are included.

${TOPIC_UTIL_DUMP_DDL_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_EXPORT_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_DDL_COMPRESSION}
${TOPIC_UTIL_DUMP_OCI_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_OCI_PAR_COMMON_OPTIONS}

${TOPIC_UTIL_AWS_COMMON_OPTIONS}

${TOPIC_UTIL_DUMP_SCHEMAS_COMMON_DETAILS}

Dumps cannot be created for the following schemas:
@li information_schema,
@li mysql,
@li ndbinfo,
@li performance_schema,
@li sys.

<b>Options</b>

If the <b>excludeSchemas</b> or <b>includeSchemas</b> options contain a schema
which is not included in the dump or does not exist, it is ignored.

${TOPIC_UTIL_DUMP_DDL_COMMON_OPTION_DETAILS}
${TOPIC_UTIL_DUMP_COMPATIBILITY_OPTION}
${TOPIC_UTIL_DUMP_OCI_COMMON_OPTION_DETAILS}
${TOPIC_UTIL_DUMP_OCI_PAR_OPTION_DETAILS}

${TOPIC_UTIL_DUMP_AWS_COMMON_OPTION_DETAILS}

@throws ArgumentError in the following scenarios:
@li If any of the input arguments contains an invalid value.

@throws RuntimeError in the following scenarios:
@li If there is no open global session.
@li If creating the output directory fails.
@li If creating or writing to the output file fails.
)*");

/**
 * \ingroup util
 *
 * $(UTIL_DUMPINSTANCE_BRIEF)
 *
 * $(UTIL_DUMPINSTANCE)
 */
#if DOXYGEN_JS
Undefined Util::dumpInstance(String outputUrl, Dictionary options);
#elif DOXYGEN_PY
None Util::dump_instance(str outputUrl, dict options);
#endif
void Util::dump_instance(
    const std::string &directory,
    const shcore::Option_pack_ref<dump::Dump_instance_options> &options) {
  const auto session = _shell_core.get_dev_session();

  if (!session || !session->is_open()) {
    throw std::runtime_error(
        "An open session is required to perform this operation.");
  }

  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.dumpInstance()"};

  using mysqlsh::dump::Dump_instance;

  mysqlsh::dump::Dump_instance_options opts = *options;
  opts.set_output_url(directory);
  opts.set_session(session->get_core_session());

  Dump_instance{opts}.run();
}

}  // namespace mysqlsh
