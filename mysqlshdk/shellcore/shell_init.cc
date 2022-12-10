/*
 * Copyright (c) 2018, 2020 Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/shell_init.h"
#include <curl/curl.h>
#include <mysql.h>
#include <stdlib.h>
#include <stdexcept>

#ifdef HAVE_V8
namespace shcore {
extern void JScript_context_init();
extern void JScript_context_fini();
}  // namespace shcore
#endif

namespace mysqlsh {

void thread_init() { mysql_thread_init(); }

void thread_end() { mysql_thread_end(); }

void global_init() {
#ifdef HAVE_V8
  shcore::JScript_context_init();
#endif

  mysql_library_init(0, nullptr, nullptr);

  thread_init();

  srand(time(0));
  curl_global_init(CURL_GLOBAL_ALL);
}

void global_end() {
  thread_end();
  mysql_library_end();

#ifdef HAVE_V8
  shcore::JScript_context_fini();
#endif
}

Mysql_thread::Mysql_thread() {
  if (mysql_thread_init()) {
    throw std::runtime_error(
        "Cannot allocate specific memory for the MySQL thread.");
  }
}

Mysql_thread::~Mysql_thread() { mysql_thread_end(); }

}  // namespace mysqlsh
