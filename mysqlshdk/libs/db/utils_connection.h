/*
 * Copyright (c) 2014, 2021, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_DB_UTILS_CONNECTION_H_
#define MYSQLSHDK_LIBS_DB_UTILS_CONNECTION_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "mysqlshdk/include/mysqlshdk_export.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#ifdef _WIN32
#include <string.h>
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

namespace mysqlshdk {
namespace db {

/**
 * Bidirectional map of modes values:
 * "DISABLED" <-> 1
 * "PREFERRED" <-> 2
 * "REQUIRED" <-> 3
 * "VERIFY_CA" <-> 4
 * "VERIFY_IDENTITY" <-> 5
 */
class SHCORE_PUBLIC MapSslModeNameToValue {
 private:
  MapSslModeNameToValue() {}

 public:
  static int get_value(const std::string &value);
  static const std::string &get_value(int value);
};

// common keys for dictionary connection data
constexpr const char kHost[] = "host";
constexpr const char kPort[] = "port";
constexpr const char kSocket[] = "socket";
constexpr const char kScheme[] = "scheme";
// schema in mysql uri is actually a path in generic URI
// so this two will point to the exact same place then
constexpr const char kPath[] = "schema";
constexpr const char kSchema[] = "schema";
constexpr const char kUser[] = "user";
constexpr const char kDbUser[] = "dbUser";
constexpr const char kPassword[] = "password";
constexpr const char kDbPassword[] = "dbPassword";
constexpr const char kSshRemoteHost[] = "ssh-remote-host";
constexpr const char kSshPassword[] = "ssh-password";
constexpr const char kSshIdentityFile[] = "ssh-identity-file";
constexpr const char kSshIdentityFilePassword[] = "ssh-identity-file-password";
constexpr const char kSshConfigFile[] = "ssh-config-file";
constexpr const char kSslCa[] = "ssl-ca";
constexpr const char kSslCaPath[] = "ssl-capath";
constexpr const char kSslCert[] = "ssl-cert";
constexpr const char kSslKey[] = "ssl-key";
constexpr const char kSslCrl[] = "ssl-crl";
constexpr const char kSslCrlPath[] = "ssl-crlpath";
constexpr const char kSslCipher[] = "ssl-cipher";
constexpr const char kSslTlsVersion[] = "tls-version";
constexpr const char kSslTlsVersions[] = "tls-versions";
constexpr const char kSslTlsCiphersuites[] = "tls-ciphersuites";
constexpr const char kSslMode[] = "ssl-mode";
constexpr const char kAuthMethod[] = "auth-method";
constexpr const char kGetServerPublicKey[] = "get-server-public-key";
constexpr const char kServerPublicKeyPath[] = "server-public-key-path";
constexpr const char kConnectTimeout[] = "connect-timeout";
constexpr const char kNetReadTimeout[] = "net-read-timeout";
constexpr const char kNetWriteTimeout[] = "net-write-timeout";
constexpr const char kCompression[] = "compression";
constexpr const char kCompressionAlgorithms[] = "compression-algorithms";
constexpr const char kCompressionLevel[] = "compression-level";
constexpr const char kLocalInfile[] = "local-infile";
constexpr const char kNetBufferLength[] = "net-buffer-length";
constexpr const char kMaxAllowedPacket[] = "max-allowed-packet";
constexpr const char kMysqlPluginDir[] = "mysql-plugin-dir";
constexpr const char kFidoRegisterFactor[] = "fido-register-factor";
constexpr const char kConnectionAttributes[] = "connection-attributes";
constexpr const char kUri[] = "uri";
constexpr const char kSsh[] = "ssh";

constexpr const char kSslModeDisabled[] = "disabled";
constexpr const char kSslModePreferred[] = "preferred";
constexpr const char kSslModeRequired[] = "required";
constexpr const char kSslModeVerifyCA[] = "verify_ca";
constexpr const char kSslModeVerifyIdentity[] = "verify_identity";

constexpr const char kCompressionRequired[] = "REQUIRED";
constexpr const char kCompressionPreferred[] = "PREFERRED";
constexpr const char kCompressionDisabled[] = "DISABLED";

constexpr char kAuthMethodClearPassword[] = "mysql_clear_password";
constexpr char kAuthMethodLdapSasl[] = "authentication_ldap_sasl_client";
constexpr char kAuthMethodKerberos[] = "authentication_kerberos_client";
constexpr char kAuthMethodOci[] = "authentication_oci_client";

const std::set<std::string> db_connection_attributes = {kUri,
                                                        kHost,
                                                        kPort,
                                                        kSocket,
                                                        kScheme,
                                                        kSchema,
                                                        kUser,
                                                        kDbUser,
                                                        kPassword,
                                                        kDbPassword,
                                                        kSslCa,
                                                        kSslCaPath,
                                                        kSslCert,
                                                        kSslKey,
                                                        kSslCrl,
                                                        kSslCrlPath,
                                                        kSslCipher,
                                                        kSslTlsVersion,
                                                        kSslTlsCiphersuites,
                                                        kSslMode,
                                                        kAuthMethod,
                                                        kGetServerPublicKey,
                                                        kServerPublicKeyPath,
                                                        kConnectTimeout,
                                                        kNetReadTimeout,
                                                        kNetWriteTimeout,
                                                        kCompression,
                                                        kCompressionAlgorithms,
                                                        kCompressionLevel,
                                                        kConnectionAttributes};

const std::set<std::string> ssh_uri_connection_attributes = {
    kSsh, kSshConfigFile, kSshIdentityFile, kSshIdentityFilePassword,
    kSshPassword};

std::set<std::string> connection_attributes();

const std::set<std::string> uri_connection_attributes = {kSslCa,
                                                         kSslCaPath,
                                                         kSslCert,
                                                         kSslKey,
                                                         kSslCrl,
                                                         kSslCrlPath,
                                                         kSslCipher,
                                                         kSslTlsVersion,
                                                         kSslTlsCiphersuites,
                                                         kSslMode,
                                                         kAuthMethod,
                                                         kGetServerPublicKey,
                                                         kServerPublicKeyPath,
                                                         kConnectTimeout,
                                                         kCompression,
                                                         kCompressionAlgorithms,
                                                         kCompressionLevel,
                                                         kConnectionAttributes};

const std::set<std::string> uri_extra_options = {
    kAuthMethod,      kGetServerPublicKey,    kServerPublicKeyPath,
    kConnectTimeout,  kNetReadTimeout,        kNetWriteTimeout,
    kCompression,     kCompressionAlgorithms, kLocalInfile,
    kNetBufferLength, kMaxAllowedPacket,      kMysqlPluginDir};

const std::vector<std::string> ssl_modes = {"",
                                            kSslModeDisabled,
                                            kSslModePreferred,
                                            kSslModeRequired,
                                            kSslModeVerifyCA,
                                            kSslModeVerifyIdentity};

const std::map<std::string, std::string, shcore::Case_insensitive_comparator>
    deprecated_connection_attributes = {{kDbUser, kUser},
                                        {kDbPassword, kPassword}};

}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_UTILS_CONNECTION_H_
