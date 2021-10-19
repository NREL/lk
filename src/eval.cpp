/***********************************************************************************************************************
*  LK, Copyright (c) 2008-2017, Alliance for Sustainable Energy, LLC. All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
*  following conditions are met:
*
*  (1) Redistributions of source code must retain the above copyright notice, this list of conditions and the following
*  disclaimer.
*
*  (2) Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
*  following disclaimer in the documentation and/or other materials provided with the distribution.
*
*  (3) Neither the name of the copyright holder nor the names of any contributors may be used to endorse or promote
*  products derived from this software without specific prior written permission from the respective party.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
*  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER, THE UNITED STATES GOVERNMENT, OR ANY CONTRIBUTORS BE LIABLE FOR
*  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
*  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
*  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************************************************************/

#include <cstdio>
#include <cstring>
#include <cmath>
#include <limits>

#include <lk/eval.h>
#include <lk/invoke.h>


char *replace_char(const char *input, char find, char replace) {

    char *output = (char *) malloc(strlen(input));

    for (size_t i = 0; i < strlen(input); i++) {
        if (input[i] == find) output[i] = replace;
        else output[i] = input[i];
    }

    output[strlen(input)] = '\0';

    return output;
}

static lk_string make_error(lk::node_t *n, const char *fmt, ...) {
    char buf[512];
    memset(buf, 0, sizeof(buf));

//	char *ffmt = replace_char(fmt, '%', '_');

    fmt = replace_char(fmt, '%', '_');

    sprintf(buf, "[%d]: ", n->line());
    char *p = buf + strlen(buf);
    va_list list;
    va_start(list, fmt);
//	if (strlen((const char *)list) != 0)
    {
#ifdef _WIN32
        _vsnprintf(p, 480, fmt, list);
#else
        vsnprintf(p, 480, fmt, list);
#endif
    }
    va_end(list);

    return lk_string(buf);
}

#define ENV_MUTABLE 0x0001

lk::eval::eval(lk::node_t *tree)
        : m_tree(tree), m_env(&m_localEnv) {
}

lk::eval::eval(lk::node_t *tree, lk::env_t *env)
        : m_tree(tree), m_env(env) {
}

lk::eval::~eval() {
    /* nothing to do */
}

bool lk::eval::run() {
    unsigned int ctl = CTL_NONE;
    return interpret(m_tree, m_env, m_result, 0, ctl);
}

bool lk::eval::special_set(const lk_string &name, vardata_t &) {
    throw error_t(lk_tr("no defined mechanism to set special variable") + " '" + name + "'");
}

bool lk::eval::special_get(const lk_string &name, vardata_t &) {
    throw error_t(lk_tr("no defined mechanism to get special variable") + " '" + name + "'");
}

static void do_plus_eq(lk::vardata_t &l, lk::vardata_t &r) {
    if (l.deref().type() == lk::vardata_t::STRING)
        l.deref().assign(l.deref().as_string() + r.deref().as_string());
    else if (l.deref().type() == lk::vardata_t::VECTOR) {
        if (r.deref().type() == lk::vardata_t::VECTOR) {
            for (size_t i = 0; i < r.deref().length(); i++)
                l.deref().vec()->push_back(*r.deref().index(i));
        } else
            // append to the vector
            l.deref().vec()->push_back(r.deref());
    } else
        l.deref().assign(l.deref().num() + r.deref().as_number());
}

static void do_minus_eq(lk::vardata_t &l, lk::vardata_t &r) {
    l.deref().assign(l.deref().num() - r.deref().num());
}

static void do_mult_eq(lk::vardata_t &l, lk::vardata_t &r) {
    l.deref().assign(l.deref().num() * r.deref().num());
}

static void do_div_eq(lk::vardata_t &l, lk::vardata_t &r) {
    l.deref().assign(l.deref().num() / r.deref().num());
}

bool lk::eval::interpret(node_t *root,
                         env_t *cur_env,
                         vardata_t &result,
                         unsigned int flags,
                         unsigned int &ctl_id) {
    if (!root) return true;

    if (!on_run(root->line())) {
        return false;
    } /* abort script execution */

    if (list_t *n1 = dynamic_cast<list_t *>(root)) {
        ctl_id = CTL_NONE;
        bool ok = true;
        for (size_t i = 0; i < n1->items.size() && ctl_id == CTL_NONE; i++) {
            ok = interpret(n1->items[i], cur_env, result, flags, ctl_id);
            if (!ok) {
                m_errors.push_back(
                        make_error(n1, (const char *) lk_string(lk_tr("eval error in statement list") + "\n").c_str()));
                return false;
            }
        }

        return ok;
    } else if (iter_t *n2 = dynamic_cast<iter_t *>(root)) {
        if (!interpret(n2->init, cur_env, result, flags, ctl_id)) {
            return false;
        }

        while (1) {
            // test the condition
            vardata_t outcome;
            outcome.assign(0.0);

            if (!interpret(n2->test, cur_env, outcome, flags, ctl_id)) {
                return false;
            }

            if (!outcome.as_boolean())
                break;

            if (!interpret(n2->block, cur_env, result, flags, ctl_id)) {
                return false;
            }

            switch (ctl_id) {
                case CTL_BREAK:
                    ctl_id = CTL_NONE;
                case CTL_RETURN:
                case CTL_EXIT:
                    return true;

                case CTL_CONTINUE:
                default:
                    ctl_id = CTL_NONE;
            }

            if (!interpret(n2->adv, cur_env, result, flags, ctl_id)) {
                return false;
            }
        }

        return true;
    } else if (cond_t *n3 = dynamic_cast<cond_t *>(root)) {
        vardata_t outcome;
        outcome.assign(0.0);
        if (!interpret(n3->test, cur_env, outcome, flags, ctl_id)) {
            return false;
        }

        if (outcome.as_boolean())
            return interpret(n3->on_true, cur_env, result, flags, ctl_id);
        else
            return interpret(n3->on_false, cur_env, result, flags, ctl_id);
    } else if (expr_t *n4 = dynamic_cast<expr_t *>(root)) {
        try {
            bool ok = true;
            vardata_t l, r;
            double newval;

            switch (n4->oper) {
                case expr_t::PLUS:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    if (l.deref().type() == vardata_t::STRING
                        || r.deref().type() == vardata_t::STRING) {
                        result.assign(l.deref().as_string() + r.deref().as_string());
                    } else
                        result.assign(l.deref().num() + r.deref().num());
                    return ok;
                case expr_t::MINUS:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    result.assign(l.deref().num() - r.deref().num());
                    return ok;
                case expr_t::MULT:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    result.assign(l.deref().num() * r.deref().num());
                    return ok;
                case expr_t::DIV:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    if (r.deref().num() == 0)
                        result.assign(std::numeric_limits<double>::quiet_NaN());
                    else
                        result.assign(l.deref().num() / r.deref().num());
                    return ok;

                case expr_t::PLUSEQ:
                    do_op_eq(do_plus_eq, n4, cur_env, flags, ctl_id, result, l, r);
                    return ok;
                case expr_t::MINUSEQ:
                    do_op_eq(do_minus_eq, n4, cur_env, flags, ctl_id, result, l, r);
                    return ok;
                case expr_t::MULTEQ:
                    do_op_eq(do_mult_eq, n4, cur_env, flags, ctl_id, result, l, r);
                    return ok;
                case expr_t::DIVEQ:
                    do_op_eq(do_div_eq, n4, cur_env, flags, ctl_id, result, l, r);
                    return ok;

                case expr_t::MINUSAT:
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);

                    if (l.deref().type() == vardata_t::HASH) {
                        lk::varhash_t *hh = l.deref().hash();
                        lk::varhash_t::iterator it = hh->find(r.deref().as_string());
                        if (it != hh->end())
                            hh->erase(it);
                    } else if (l.deref().type() == vardata_t::VECTOR) {
                        std::vector<lk::vardata_t> *vv = l.deref().vec();
                        size_t idx = r.deref().as_unsigned();
                        if (idx < vv->size())
                            vv->erase(vv->begin() + idx);
                    } else {
                        m_errors.push_back(make_error(n4, (const char *) lk_string("-@ " +
                                                                                   lk_tr("operator requires a hash or vector left hand side")).c_str()));
                        return false;
                    }

                    return true;

                case expr_t::INCR:
                    ok = ok && interpret(n4->left, cur_env, l, flags | ENV_MUTABLE, ctl_id);
                    newval = l.deref().num() + 1;
                    l.deref().assign(newval);
                    result.assign(newval);
                    return ok;
                case expr_t::DECR:
                    ok = ok && interpret(n4->left, cur_env, l, flags | ENV_MUTABLE, ctl_id);
                    newval = l.deref().num() - 1;
                    l.deref().assign(newval);
                    result.assign(newval);
                    return ok;
                case expr_t::DEFINE:
                    result.assign(n4);
                    return ok;
                case expr_t::ASSIGN:
                    // evaluate expression before the lhs identifier
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);

                    // if on the LHS of the assignment we have a special variable i.e. ${xy}, use a
                    // hack to assign the value to the storage location
                    if (lk::iden_t *iden = dynamic_cast<lk::iden_t *>(n4->left))
                        if (iden->special)
                            return ok &&
                                   special_set(iden->name, r.deref()); // don't bother to copy rhs to result either.

                    // otherwise evaluate the LHS in a mutable context, as normal.
                    ok = ok && interpret(n4->left, cur_env, l, flags | ENV_MUTABLE, ctl_id);
                    l.deref().copy(r.deref());
                    result.copy(r.deref());
                    return ok;
                case expr_t::LOGIOR:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    if (((int) l.deref().num()) != 0) // short circuit evaluation
                    {
                        result.assign(1.0);
                        return ok;
                    }
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    result.assign((((int) l.deref().num()) || ((int) r.deref().num())) ? 1 : 0);
                    return ok;
                case expr_t::LOGIAND:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    if (((int) l.deref().num()) == 0) // short circuit evaluation
                    {
                        result.assign(0.0);
                        return ok;
                    }
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    result.assign((((int) l.deref().num()) && ((int) r.deref().num())) ? 1 : 0);
                    return ok;
                case expr_t::NOT:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    result.assign(((int) l.deref().num()) ? 0 : 1);
                    return ok;
                case expr_t::EQ:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    result.assign(l.deref().equals(r.deref()) ? 1 : 0);
                    return ok;
                case expr_t::NE:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    result.assign(l.deref().equals(r.deref()) ? 0 : 1);
                    return ok;
                case expr_t::LT:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    result.assign(l.deref().lessthan(r.deref()) ? 1 : 0);
                    return ok;
                case expr_t::LE:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    result.assign(l.deref().lessthan(r.deref()) || l.deref().equals(r.deref()) ? 1 : 0);
                    return ok;
                case expr_t::GT:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    result.assign(!l.deref().lessthan(r.deref()) && !l.deref().equals(r.deref()) ? 1 : 0);
                    return ok;
                case expr_t::GE:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    result.assign(!l.deref().lessthan(r.deref()) ? 1 : 0);
                    return ok;
                case expr_t::EXP:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    result.assign(pow(l.deref().num(), r.deref().num()));
                    return ok;
                case expr_t::NEG:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    result.assign(0 - l.deref().num());
                    return ok;
                case expr_t::WHEREAT:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    ok = ok && interpret(n4->right, cur_env, r, flags, ctl_id);
                    if (l.deref().type() == vardata_t::HASH) {
                        lk::varhash_t *hh = l.deref().hash();
                        result.assign(hh->find(r.deref().as_string()) != hh->end() ? 1.0 : 0.0);
                    } else if (l.deref().type() == vardata_t::VECTOR) {
                        std::vector<lk::vardata_t> *vv = l.deref().vec();
                        for (size_t i = 0; i < vv->size(); i++) {
                            if ((*vv)[i].equals(r.deref())) {
                                result.assign((double) i);
                                return ok;
                            }
                        }

                        result.assign(-1.0);
                        return ok;
                    } else if (l.deref().type() == vardata_t::STRING) {
                        lk_string::size_type pos = l.deref().str().find(r.deref().as_string());
                        result.assign(pos != lk_string::npos ? (int) pos : -1.0);
                    } else {
                        m_errors.push_back(make_error(n4,
                                                      (const char *) lk_tr(
                                                              "left hand side to find operator ?@ must be a hash, vector, or string").c_str()));
                        return false;
                    }
                    return ok;
                case expr_t::INDEX: {
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    bool anonymous = (l.type() == vardata_t::VECTOR);

                    vardata_t &arr = l.deref();

                    if (!(flags & ENV_MUTABLE) && arr.type() != vardata_t::VECTOR) {
                        m_errors.push_back(make_error(n4->left,
                                                      (const char *) lk_tr(
                                                              "cannot index non array data in non mutable context").c_str()));
                        return false;
                    }

                    ok = ok && interpret(n4->right, cur_env, r, 0, ctl_id);
                    size_t idx = r.deref().as_unsigned();

                    if ((flags & ENV_MUTABLE)
                        && (arr.type() != vardata_t::VECTOR
                            || arr.length() <= idx))
                        arr.resize(idx + 1);

                    vardata_t *item = arr.index(idx);
                    if (anonymous)
                        result.copy(*item);
                    else
                        result.assign(item);

                    return ok;
                }
                case expr_t::HASH: {
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    bool anonymous = (l.type() == vardata_t::HASH);

                    vardata_t &hash = l.deref();

                    if ((flags & ENV_MUTABLE)
                        && (hash.type() != vardata_t::HASH))
                        hash.empty_hash();

                    ok = ok && interpret(n4->right, cur_env, r, 0, ctl_id);
                    vardata_t &val = r.deref();

                    vardata_t *x = hash.lookup(val.as_string());
                    if (x) {
                        if (anonymous)
                            result.copy(*x);
                        else
                            result.assign(x);
                    } else if ((flags & ENV_MUTABLE)) {
                        hash.assign(val.as_string(), x = new vardata_t);
                        result.assign(x);
                    } else
                        result.nullify();

                    return ok;
                }
                case expr_t::CALL:
                case expr_t::THISCALL: {
                    expr_t *cur_expr = n4;

                    if (iden_t *iden = dynamic_cast<iden_t *>(n4->left)) {
                        // query function table for identifier
                        if (lk::fcallinfo_t *fi = cur_env->lookup_func(iden->name)) {
                            lk::invoke_t cxt(cur_env, result, fi->user_data);
                            list_t *argvals = dynamic_cast<list_t *>(n4->right);

                            // first determine number of arguments
                            size_t nargs = 0;
                            if (argvals) nargs = argvals->items.size();

                            if (nargs > 0) {
                                // allocate argument vector and evaluate each argument
                                cxt.arg_list().resize(nargs, vardata_t());
                                for (size_t iarg = 0; iarg < nargs; iarg++) {
                                    unsigned int c = CTL_NONE;

                                    lk::vardata_t &argval = cxt.arg_list()[iarg];
                                    if (!interpret(argvals->items[iarg], cur_env, argval, flags, c)) {
                                        lk_string err;
                                        err << "[" << argvals->line() << "]:failed to evaluate function call argument number " << iarg  << ", value = " << dynamic_cast<iden_t*>(argvals->items[iarg])->name;
                                        m_errors.push_back(err +"\n");
//                                        make_error(argvals, (const char*)lk_string(lk_tr("failed to evaluate function call argument %d to '%s()'\n")).c_str(), (int)iarg));
                                        return false;
                                    }
                                }
                            }

                            try {
                                if (fi->f) (*(fi->f))(cxt);
                                else if (fi->f_ext) lk::external_call(fi->f_ext, cxt);
                                else cxt.error(lk_tr("invalid internal reference to function callback") + " " +  iden->name);
                            }
                            catch (std::exception &e) {
                                cxt.error(e.what());
                            }

                            if (cxt.has_error())
                                m_errors.push_back(make_error(iden, (const char *) lk_string(
                                        lk_tr("error in call to") + " '" + iden->name + "()': " +
                                        cxt.error()).c_str()));

                            // do a deep copy of internalized references
                            result.deep_localize();

                            return !cxt.has_error();
                        }
                    }

                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    expr_t *define = dynamic_cast<expr_t *>(l.deref().func());
                    if (!define) {
                        m_errors.push_back(make_error(n4, (const char *) lk_string(
                                lk_tr("error in function call: malformed 'define'") + "\n").c_str()));
                        return false;
                    }

                    node_t *block = define->right;

                    // create new environment frame
                    env_t frame(cur_env);

                    // number of expected arguments
                    list_t *argnames = dynamic_cast<list_t *>(define->left);
                    size_t nargs_expected = argnames ? argnames->items.size() : 0;

                    // number of provided arguments
                    list_t *argvals = dynamic_cast<list_t *>(n4->right);
                    size_t nargs_given = argvals ? argvals->items.size() : 0;

                    if (n4->oper == expr_t::THISCALL)
                        nargs_given++;

                    if (nargs_given < nargs_expected) {
                        m_errors.push_back(make_error(n4,
                                                      (const char *) lk_string(
                                                              lk_tr("too few arguments provided to function call: %d expected, %d given") +
                                                              "\n").c_str(),
                                                      nargs_expected, nargs_given));
                        return false;
                    }

                    // evaluate each argument and assign it into the new environment
                    expr_t *thisexpr = dynamic_cast<expr_t *>(cur_expr->left);
                    if (cur_expr->oper == expr_t::THISCALL
                        && thisexpr != 0
                        && thisexpr->left != 0) {
                        vardata_t thisobj;
                        unsigned int c = CTL_NONE;
                        if (!interpret(thisexpr->left, cur_env, thisobj, flags, c)) {
                            m_errors.push_back(make_error(cur_expr,
                                                          lk_tr("failed to evaluate 'this' parameter 0 for THISCALL -> method").c_str()));
                            return false;
                        }

                        if (thisobj.type() != vardata_t::REFERENCE) {
                            m_errors.push_back(make_error(cur_expr,
                                                          (const char *) lk_string(
                                                                  lk_tr("'this' parameter did not evaluate to a reference, rather:") +
                                                                  thisobj.typestr()).c_str()));
                            return false;
                        }

                        frame.assign("this", new vardata_t(thisobj));
                    }

                    vardata_t *__args = new vardata_t;
                    __args->empty_vector();

                    if (argvals) {
                        for (size_t argindex = 0;
                             argindex < argvals->items.size();
                             argindex++) {
                            vardata_t v;
                            iden_t *id = 0;

                            unsigned int c = CTL_NONE;
                            if (!interpret(argvals->items[argindex], cur_env, v, flags, c)) {
                                m_errors.push_back(make_error(argvals->items[argindex],
                                                              lk_tr("failed to initialize function call argument\n").c_str()));
                                return false;
                            }

                            if (argindex < argnames->items.size() &&
                                ((id = dynamic_cast<iden_t *>(argnames->items[argindex])) != 0))
                                frame.assign(id->name, new vardata_t(v));

                            __args->vec()->push_back(vardata_t(v));
                        }
                    }

                    frame.assign("__args", __args);

                    // now evaluate the function block in the new environment
                    if (!interpret(block, &frame, result, flags, ctl_id)) {
                        m_errors.push_back(make_error(block, lk_tr("error inside function call\n").c_str()));
                        return false;
                    }

                    // do a deep copy of internalized references
                    result.deep_localize();

                    // reset the sequence control
                    if (ctl_id != CTL_EXIT) ctl_id = CTL_NONE;

                    // environment frame will automatically be destroyed here
                    return true;
                }
                    break;
                case expr_t::SIZEOF:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    if (l.deref().type() == vardata_t::VECTOR) {
                        result.assign((int) l.deref().length());
                        return ok;
                    } else if (l.deref().type() == vardata_t::STRING) {
                        result.assign((int) l.deref().str().length());
                        return ok;
                    } else if (l.deref().type() == vardata_t::HASH) {
                        int count = 0;

                        varhash_t *h = l.deref().hash();
                        for (varhash_t::iterator it = h->begin();
                             it != h->end();
                             ++it) {
                            if ((*it).second->deref().type() != vardata_t::NULLVAL)
                                count++;
                        }
                        result.assign(count);
                        return ok;
                    } else {
                        m_errors.push_back(make_error(n4,
                                                      lk_tr("operand to # ('sizeof') must be a array, string, or table type\n").c_str()));
                        return false;
                    }
                    break;
                case expr_t::KEYSOF:
                    ok = ok && interpret(n4->left, cur_env, l, flags, ctl_id);
                    if (l.deref().type() == vardata_t::HASH) {
                        varhash_t *h = l.deref().hash();
                        result.empty_vector();
                        result.vec()->reserve(h->size());
                        for (varhash_t::iterator it = h->begin();
                             it != h->end();
                             ++it) {
                            if ((*it).second->deref().type() != vardata_t::NULLVAL)
                                result.vec_append((*it).first);
                        }
                        return true;
                    } else {
                        m_errors.push_back(make_error(n4, lk_tr("operand to @ (keysof) must be a table").c_str()));
                        return false;
                    }
                    break;
                case expr_t::TYPEOF:
                    if (lk::iden_t *iden = dynamic_cast<lk::iden_t *>(n4->left)) {
                        if (lk::vardata_t *vv = cur_env->lookup(iden->name, true)) result.assign(vv->typestr());
                        else result.assign("unknown");
                        return true;
                    } else {
                        m_errors.push_back(
                                make_error(n4, lk_tr("argument to typeof(...) must be an identifier").c_str()));
                        return false;
                    }
                    break;
                case expr_t::INITVEC: {
                    result.empty_vector();
                    list_t *p = dynamic_cast<list_t *>(n4->left);
                    if (p) {
                        for (size_t i = 0; i < p->items.size(); i++) {
                            vardata_t v;
                            ok = ok && interpret(p->items[i], cur_env, v, flags, ctl_id);
                            result.vec()->push_back(v.deref());
                        }
                    }
                }
                    return ok && ctl_id == CTL_NONE;
                case expr_t::INITHASH: {
                    result.empty_hash();
                    list_t *p = dynamic_cast<list_t *>(n4->left);
                    if (p) {
                        for (size_t i = 0; i < p->items.size(); i++) {
                            expr_t *assign = dynamic_cast<expr_t *>(p->items[i]);
                            if (assign && assign->oper == expr_t::ASSIGN) {
                                vardata_t vkey, vval;
                                ok = ok && interpret(assign->left, cur_env, vkey, flags, ctl_id)
                                     && interpret(assign->right, cur_env, vval, flags, ctl_id);

                                if (ok) {
                                    lk_string key = vkey.as_string();
                                    varhash_t *h = result.hash();
                                    varhash_t::iterator it = h->find(key);
                                    if (it != h->end())
                                        (*it).second->copy(vval.deref());
                                    else
                                        (*h)[key] = new vardata_t(vval.deref());
                                }
                            }
                        }
                    }
                }
                    return ok && ctl_id == CTL_NONE;
                case expr_t::SWITCH: {
                    vardata_t switchval;
                    switchval.assign(-1.0);
                    if (!interpret(n4->left, cur_env, switchval, flags, ctl_id)) {
                        return false;
                    }
                    list_t *p = dynamic_cast<list_t *>(n4->right);
                    size_t index = switchval.as_unsigned();
                    if (!p || index >= p->items.size()) {
                        m_errors.push_back(
                                make_error(n4, lk_tr("invalid switch statement index of %d").c_str(), index));
                        return false;
                    }

                    if (!interpret(p->items[index], cur_env, result, flags, ctl_id)) {
                        return false;
                    }

                    return ok;
                }
                default:
                    break;
            }
        }
        catch (lk::error_t &e) {
            m_errors.push_back(make_error(n4, (const char *) lk_string(lk_tr("error") +
                                                                       ": %s\n").c_str(),
                                          (const char *) e.text.c_str()));
            return false;
        }
    } else if (ctlstmt_t *n5 = dynamic_cast<ctlstmt_t *>(root)) {
        try {
            vardata_t l;
            bool ok = true;
            switch (n5->ictl) {
                case ctlstmt_t::RETURN:
                    if (n5->rexpr != 0) {
                        ok = ok && interpret(n5->rexpr, cur_env, l, flags, ctl_id);
                        result.copy(l.deref());
                    }
                    ctl_id = CTL_RETURN;
                    return ok;
                case ctlstmt_t::EXIT:
                    ctl_id = CTL_EXIT;
                    return true;
                    break;
                case ctlstmt_t::BREAK:
                    ctl_id = CTL_BREAK;
                    return true;
                    break;
                case ctlstmt_t::CONTINUE:
                    ctl_id = CTL_CONTINUE;
                    return true;
                    break;
            }
        }
        catch (lk::error_t &e) {
            m_errors.push_back(make_error(n5, (const char *) lk_string(lk_tr("error") + ": %s\n").c_str(),
                                          (const char *) e.text.c_str()));
            return false;
        }
    } else if (iden_t *n6 = dynamic_cast<iden_t *>(root)) {
        if (n6->special && !(flags & ENV_MUTABLE)) {
            return special_get(n6->name, result);
        }

        vardata_t *x = cur_env->lookup(n6->name, !(flags & ENV_MUTABLE));

        if (x) {
            if (n6->constval) {
                m_errors.push_back(make_error(n6,
                                              (const char *) lk_string(
                                                      lk_tr("overriding previous non-const identifier with const-ness not allowed:") +
                                                      n6->name + "\n").c_str()));
                result.nullify();
                return false;
            }

            if (n6->globalval) {
                m_errors.push_back(make_error(n6,
                                              (const char *) lk_string(
                                                      lk_tr("overriding previous non-global identifier with global-ness not allowed:") +
                                                      n6->name + "\n").c_str()));
                result.nullify();
                return false;
            }

            result.assign(x);
            return true;
        } else if (!x && (flags & ENV_MUTABLE)) {
            // check if the variable exists in the global frame
            // and it was originally created as a global variable
            x = cur_env->global()->lookup(n6->name, false);
            if (x && x->flagval(vardata_t::GLOBALVAL)) {
                result.assign(x);
                return true;
            }

            x = new vardata_t;

            if (n6->constval) {
                x->set_flag(vardata_t::CONSTVAL);
                x->clear_flag(vardata_t::ASSIGNED);
            }

            if (n6->globalval) {
                x->set_flag(vardata_t::GLOBALVAL);
                cur_env->global()->assign(n6->name, x);
            } else
                cur_env->assign(n6->name, x);

            result.assign(x);
            return true;
        } else {
            m_errors.push_back(make_error(n6,
                                          (const char *) lk_string(
                                                  lk_tr("reference to unassigned variable:") + n6->name +
                                                  "\n").c_str()));
            result.nullify();
            return false;
        }
    } else if (constant_t *n7 = dynamic_cast<constant_t *>(root)) {
        result.assign(n7->value);
        return true;
    } else if (literal_t *n8 = dynamic_cast<literal_t *>(root)) {
        result.assign(n8->value);
        return true;
    } else if (0 != dynamic_cast<null_t *>(root)) {
        result.nullify();
        return true;
    }

    return false;
}

bool lk::eval::do_op_eq(void(*oper)(lk::vardata_t &, lk::vardata_t &),
                        lk::expr_t *n, lk::env_t *cur_env, unsigned int &flags, unsigned int &ctl_id,
                        lk::vardata_t &result, lk::vardata_t &l, lk::vardata_t &r) {
    // evaluate expression before lhs value
    bool ok = interpret(n->right, cur_env, r, flags, ctl_id);

    // if on the LHS of the assignment we have a special variable i.e. ${xy}, use a
    // hack to assign the value to the storage location
    if (lk::iden_t *iden = dynamic_cast<lk::iden_t *>(n->left)) {
        if (iden->special) {
            lk::vardata_t value;
            special_get(iden->name, value);
            (*oper)(value, r.deref());
            return ok && special_set(iden->name, value); // don't bother to copy rhs to result either.
        }
    }

    // otherwise evaluate the LHS in a mutable context, as normal.
    ok = ok && interpret(n->left, cur_env, l, flags | ENV_MUTABLE, ctl_id);
    (*oper)(l.deref(), r.deref());
    result.copy(l.deref());
    return ok;
}
