/*
Copyright (c) 2014 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#include <string>
#include "util/sstream.h"
#include "kernel/environment.h"
#include "kernel/type_checker.h"
#include "library/kernel_serializer.h"
#include "library/scoped_ext.h"
#include "library/reducible.h"
#include "library/attribute_manager.h"

namespace lean {
struct reducible_entry {
    reducible_status m_status;
    name             m_name;
    reducible_entry():m_status(reducible_status::Semireducible) {}
    reducible_entry(reducible_status s, name const & n):m_status(s), m_name(n) {}
};

typedef name_map<reducible_status> reducible_state;

static reducible_status get_status(reducible_state const & s, name const & n) {
    if (auto it = s.find(n))
        return *it;
    else
        return reducible_status::Semireducible;
}

static name * g_class_name = nullptr;
static std::string * g_key = nullptr;

struct reducible_config {
    typedef reducible_state  state;
    typedef reducible_entry  entry;
    static void add_entry(environment const &, io_state const &, state & s, entry const & e) {
        s.insert(e.m_name, e.m_status);
    }
    static name const & get_class_name() {
         return *g_class_name;
    }
    static std::string const & get_serialization_key() {
        return *g_key;
    }
    static void  write_entry(serializer & s, entry const & e) {
        s << static_cast<char>(e.m_status) << e.m_name;
    }
    static entry read_entry(deserializer & d) {
        entry e; char s;
        d >> s >> e.m_name;
        e.m_status = static_cast<reducible_status>(s);
        return e;
    }
    static optional<unsigned> get_fingerprint(entry const & e) {
        return some(hash(static_cast<unsigned>(e.m_status), e.m_name.hash()));
    }
};

template class scoped_ext<reducible_config>;
typedef scoped_ext<reducible_config> reducible_ext;

static name * g_tmp_prefix = nullptr;

void initialize_reducible() {
    g_class_name = new name("reducible");
    g_key        = new std::string("REDU");
    g_tmp_prefix = new name(name::mk_internal_unique_name());
    reducible_ext::initialize();

    register_attribute("reducible", "reducible",
                       [](environment const & env, io_state const &, name const & d, name const & ns, bool persistent) {
                           return set_reducible(env, d, reducible_status::Reducible, ns, persistent);
                       },
                       [](environment const & env, name const & d) { return get_reducible_status(env, d) == reducible_status::Reducible; });

    register_attribute("quasireducible", "quasireducible",
                       [](environment const & env, io_state const &, name const & d, name const & ns, bool persistent) {
                           return set_reducible(env, d, reducible_status::Quasireducible, ns, persistent);
                       },
                       [](environment const & env, name const & d) { return get_reducible_status(env, d) == reducible_status::Quasireducible; });

    register_attribute("semireducible", "semireducible",
                       [](environment const & env, io_state const &, name const & d, name const & ns, bool persistent) {
                           return set_reducible(env, d, reducible_status::Semireducible, ns, persistent);
                       },
                       [](environment const & env, name const & d) { return get_reducible_status(env, d) == reducible_status::Semireducible; });

    register_attribute("irreducible", "irreducible",
                       [](environment const & env, io_state const &, name const & d, name const & ns, bool persistent) {
                           return set_reducible(env, d, reducible_status::Irreducible, ns, persistent);
                       },
                       [](environment const & env, name const & d) { return get_reducible_status(env, d) == reducible_status::Irreducible; });

    register_incompatible("reducible", "quasireducible");
    register_incompatible("reducible", "semireducible");
    register_incompatible("reducible", "irreducible");
    register_incompatible("quasireducible", "semireducible");
    register_incompatible("quasireducible", "irreducible");
    register_incompatible("semireducible", "irreducible");
}

void finalize_reducible() {
    reducible_ext::finalize();
    delete g_tmp_prefix;
    delete g_key;
    delete g_class_name;
}

void for_each_reducible(environment const & env, std::function<void(name const &, reducible_status)> const & fn) {
    reducible_state m_state = reducible_ext::get_state(env);
    m_state.for_each(fn);
}

static void check_declaration(environment const & env, name const & n) {
    declaration const & d = env.get(n);
    if (!d.is_definition())
        throw exception(sstream() << "invalid reducible command, '" << n << "' is not a definition");
}

environment set_reducible(environment const & env, name const & n, reducible_status s, name const & ns, bool persistent) {
    check_declaration(env, n);
    return reducible_ext::add_entry(env, get_dummy_ios(), reducible_entry(s, n), ns, persistent);
}

reducible_status get_reducible_status(environment const & env, name const & n) {
    reducible_state const & s = reducible_ext::get_state(env);
    return get_status(s, n);
}

bool is_at_least_quasireducible(environment const & env, name const & n) {
    reducible_status r = get_reducible_status(env, n);
    return r == reducible_status::Reducible || r == reducible_status::Quasireducible;
}

name_predicate mk_not_reducible_pred(environment const & env) {
    reducible_state m_state = reducible_ext::get_state(env);
    return [=](name const & n) { // NOLINT
        return get_status(m_state, n) != reducible_status::Reducible;
    };
}

name_predicate mk_not_quasireducible_pred(environment const & env) {
    reducible_state m_state = reducible_ext::get_state(env);
    return [=](name const & n) { // NOLINT
        auto r = get_status(m_state, n);
        return r != reducible_status::Reducible && r != reducible_status::Quasireducible;
    };
}

name_predicate mk_irreducible_pred(environment const & env) {
    reducible_state m_state = reducible_ext::get_state(env);
    return [=](name const & n) { // NOLINT
        return get_status(m_state, n) == reducible_status::Irreducible;
    };
}

type_checker_ptr mk_type_checker(environment const & env, name_generator && ngen, reducible_behavior rb) {
    switch (rb) {
    case UnfoldReducible:
        return mk_type_checker(env, std::move(ngen), mk_not_reducible_pred(env));
    case UnfoldQuasireducible:
        return mk_type_checker(env, std::move(ngen), mk_not_quasireducible_pred(env));
    case UnfoldSemireducible:
        return mk_type_checker(env, std::move(ngen), mk_irreducible_pred(env));
    }
    lean_unreachable();
}

type_checker_ptr mk_type_checker(environment const & env, reducible_behavior rb) {
    return mk_type_checker(env, name_generator(*g_tmp_prefix), rb);
}

type_checker_ptr mk_opaque_type_checker(environment const & env, name_generator && ngen) {
    return mk_type_checker(env, std::move(ngen), [](name const &) { return true; });
}
}
