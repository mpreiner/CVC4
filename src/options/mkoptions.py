#!/usr/bin/env python3

import copy
import re
import sys
import textwrap
import toml

# TODO: more checks for options, e.g., long options start with -- ...
# TODO: documentation generation
# TODO: long-options have --no- alternate if type is bool
# TODO: add error checking as done in mkoptions
# TODO: bool options don't have handlers
# TODO: check short options only one character
# TODO: links and --no- options

MODULE_ATTR_REQ = ['id', 'name', 'header']
MODULE_ATTR_ALL = MODULE_ATTR_REQ + ['options', 'aliases']

OPTION_ATTR_REQ = ['category', 'type']
OPTION_ATTR_ALL = OPTION_ATTR_REQ + [
    'name', 'help', 'smt_name', 'short', 'long', 'default', 'includes',
    'handler', 'predicates', 'notifies', 'links', 'read_only'
]

ALIAS_ATTR_REQ = ['category', 'long', 'links']
ALIAS_ATTR_ALL = ALIAS_ATTR_REQ + ['help']

CATEGORY_VALUES = ['common', 'expert', 'regular', 'undocumented']


class Module(object):
    def __init__(self, d):
        self.__dict__ = dict((k, None) for k in MODULE_ATTR_ALL)
        self.options = []
        self.aliases = []
        for (k, v) in d.items():
            assert(k in d)
            if len(v) > 0:
                self.__dict__[k] = v

class Option(object):
    def __init__(self, d):
        self.__dict__ = dict((k, None) for k in OPTION_ATTR_ALL)
        self.includes = []
        self.predicates = []
        self.notifies = []
        self.links = []
        self.read_only = False
        for (k, v) in d.items():
            assert(k in self.__dict__)
            if k == 'read_only' or len(v) > 0:
                self.__dict__[k] = v

class Alias(object):
    def __init__(self, d):
        self.__dict__ = dict((k, None) for k in ALIAS_ATTR_ALL)
        self.links = []
        for (k, v) in d.items():
            assert(k in self.__dict__)
            if len(v) > 0:
                self.__dict__[k] = v


def die(msg):
    sys.exit(msg)


def check_attribs(req_attribs, valid_attribs, d, type):
    msg_for = ""
    if 'name' in d:
        msg_for = " for '{}'".format(d['name'])
    for k in req_attribs:
        if k not in d:
            die("required {} attribute '{}' not specified{}".format(
                type, k, msg_for))
    for k in d:
        if k not in valid_attribs:
            die("invalid {} attribute '{}' specified{}".format(
                type, k, msg_for))

def write_file(name, s):
    try:
        f = open(name, 'w')
    except IOError:
        die("Could not write '{}'".format(name))
    else:
        with f:
            f.write(s)


def read_tpl(name):
    try:
        f = open(name, 'r')
    except IOError:
        die("Could not find '{}'. Aborting.".format(name))
    else:
        # Escape { and } since we later use .format to add the generated code.
        # Further, strip ${ and }$ from placeholder variables in the template
        # file.
        with f:
            return f.read().replace('{', '{{').replace('}', '}}').\
                            replace('${', '').replace('}$', '')


def codegen_module(module):
    # <module.name>_options.h
    #
    # - #define __CVC4__OPTIONS__{module.id}_H
    # - <module.include> set of all options includes
    # - option holder spec #define macro for option holder
    #   #define CVC4_OPTIONS__{module.id}__FOR_OPTION_HOLDER
    #   {option.name}__option_t::type {option.name};
    #   bool {option.name}__setByUser__;
    # - module declarations in CVC4::options (for each option)
    #   extern struct CVC4_PUBLIC {option.name}__option_t { typedef {option.type} type; type operator()() const; bool wasSetByUser() const; void set(const type& v); } {option.name} CVC4_PUBLIC;
    # - module specializations in CVC4::
    #   template <> void Options::set(options::{option.name}__option_t, const options::{option.name}__option_t::type& x);
    #   template <> bool Options::wasSetByUser(options::{option.name}__option_t) const;
    #   template <> void Options::assign(options::{option.name}__option_t, std::string option, std::string value);
    # - module inlines in CVC4::options
    #   inline {option.name}__option_t::type {option.name}__option_t::operator()() const { return (*Options::current())[*this]; }
    #   inline bool {option.name}__option_t::wasSetByUser() const { return Options::current()->wasSetByUser(*this); }
    #   inline void {option.name}__option_t::set(const {option.name}__option_t::type& v) { Options::current()->set(*this, v); }
    #
    # <module.name>_options.cpp
    #
    # - module accesors in namespace CVC4
    #   template <> void Options::set(options::{option.name}__option_t, const options::{option.name}__option_t::type& x) { d_holder->{option.name} = x; }
    #   template <> bool Options::wasSetByUser(options::{option.name}__option_t) const { return d_holder->{option.name}__setByUser__; }
    # - module global def in namespace CVC4::options
    #   struct {option.name}__option_t {option.name};

    tpl_module_h = read_tpl('module_template.h')
    tpl_module_cpp = read_tpl('module_template.cpp')

    # {name} ... option.name
    tpl_h_holder_macro = "#define CVC4_OPTIONS__{id}__FOR_OPTION_HOLDER"
    tpl_h_holder = \
        "  {name}__option_t::type {name}; \\\n  bool {name}__setByUser__;"

    # {name} ... option.name, {type} ... option.type
    tpl_h_decl_rw  = "extern struct CVC4_PUBLIC {name}__option_t {{ "
    tpl_h_decl_rw += "typedef {type} type; type operator()() const; "
    tpl_h_decl_rw += "bool wasSetByUser() const; "
    tpl_h_decl_rw += "void set(const type& v); }} {name} CVC4_PUBLIC;"

    tpl_h_decl_ro  = "extern struct CVC4_PUBLIC {name}__option_t {{ "
    tpl_h_decl_ro += "typedef {type} type; type operator()() const; "
    tpl_h_decl_ro += "bool wasSetByUser() const; }} {name} CVC4_PUBLIC;"

    # {name} ... option.name
    tpl_h_spec_s    = "template <> void Options::set(options::{name}__option_t, const options::{name}__option_t::type& x);"
    tpl_h_spec_o    = "template <> const options::{name}__option_t::type& Options::operator[](options::{name}__option_t) const;"
    tpl_h_spec_wsbu = "template <> bool Options::wasSetByUser(options::{name}__option_t) const;"
    tpl_h_spec_a    = "template <> void Options::assign(options::{name}__option_t, std::string option, std::string value);"
    tpl_h_spec_ab   = "template <> void Options::assignBool(options::{name}__option_t, std::string option, bool value);"

    # {name} ... option.name
    tpl_h_inl_s    = "inline void {name}__option_t::set(const {name}__option_t::type& v) {{ Options::current()->set(*this, v); }}"
    tpl_h_inl_wsbu = "inline bool {name}__option_t::wasSetByUser() const {{ return Options::current()->wasSetByUser(*this); }}"
    tpl_h_inl_op   = "inline {name}__option_t::type {name}__option_t::operator()() const {{ return (*Options::current())[*this]; }}"

    # {name} ... option.name
    tpl_cpp_acc_s    = "template <> void Options::set(options::{name}__option_t, const options::{name}__option_t::type& x) {{ d_holder->{name} = x; }}"
    tpl_cpp_acc_o    = "template <> const options::{name}__option_t::type& Options::operator[](options::{name}__option_t) const {{ return d_holder->{name}; }}"
    tpl_cpp_acc_wsbu = "template <> bool Options::wasSetByUser(options::{name}__option_t) const {{ return d_holder->{name}__setByUser__; }}"

    tpl_cpp_def = "struct {name}__option_t {name};"


    # .h file
    includes = set()
    holder_specs = []
    decls = []
    specs = []
    inls = []

    # .cpp file
    accs = []
    defs = []

    holder_specs.append(tpl_h_holder_macro.format(id=module.id))

    for option in module.options:
        if option.name is None:
            continue

        ### generate code for {module.name}_options.h
        for include in option.includes:
            includes.add(format_include(include))

        # generate option holder
        holder_specs.append(tpl_h_holder.format(name=option.name))

        # generate module declaration
        tpl_decl = tpl_h_decl_ro if option.read_only else tpl_h_decl_rw
        decls.append(tpl_decl.format(name=option.name, type=option.type))

        # generate module specialization
        if not option.read_only:
            specs.append(tpl_h_spec_s.format(name=option.name))
        specs.append(tpl_h_spec_o.format(name=option.name))
        specs.append(tpl_h_spec_wsbu.format(name=option.name))

        if option.type == 'bool':
            specs.append(tpl_h_spec_ab.format(name=option.name))
        else:
            specs.append(tpl_h_spec_a.format(name=option.name))

        # generate module inlines
        inls.append(tpl_h_inl_op.format(name=option.name))
        inls.append(tpl_h_inl_wsbu.format(name=option.name))
        if not option.read_only:
            inls.append(tpl_h_inl_s.format(name=option.name))


        ### generate code for {module.name}_options.cpp

        # accessors
        if not option.read_only:
            accs.append(tpl_cpp_acc_s.format(name=option.name))
        accs.append(tpl_cpp_acc_o.format(name=option.name))
        accs.append(tpl_cpp_acc_wsbu.format(name=option.name))

        # global defintions
        defs.append(tpl_cpp_def.format(name=option.name))


    filename=module.header[:-2]
    # TODO: template name
    write_file('{}.h'.format(filename),
        tpl_module_h.format(filename=filename,
                            header=module.header,
                            id=module.id,
                            includes='\n'.join(sorted(list(includes))),
                            holder_spec=' \\\n'.join(holder_specs),
                            decls='\n'.join(decls),
                            specs='\n'.join(specs),
                            inls='\n'.join(inls)))

    write_file('{}.cpp'.format(filename),
        tpl_module_cpp.format(filename=filename,
                              accs='\n'.join(accs),
                              defs='\n'.join(defs)))


def long_get_arg(name):
    l = name.split('=')
    assert(len(l) <= 2)
    return l[1] if len(l) == 2 else None

def long_get_option(name):
    return name.split('=')[0]

def is_numeric_type(t):
    if t in ['int', 'unsigned', 'unsigned long', 'long', 'float', 'double']:
        return True
    elif re.match('u?int[0-9]+_t', t):
        return True
    return False

def format_include(include):
    if '<' in include:
        return '#include {}'.format(include)
    return '#include "{}"'.format(include)

def help_format_options(short, long):
    opts = []
    arg = None
    if long:
        opts.append('--{}'.format(long))
        l = long.split('=')
        if len(l) > 1:
            arg = l[1]

    if short:
        if arg:
            opts.append('-{} {}'.format(short, arg))
        else:
            opts.append('-{}'.format(short))

    return ' | '.join(opts)

def help_format(help, short, long):
    width = 80
    width_opt = 25
    text = \
        textwrap.wrap(help.replace('"', '\\"'), width=width - width_opt)
    text_opt = help_format_options(short, long)
    if len(text_opt) > width_opt - 3:
        lines = ['  {}'.format(text_opt)]
        lines.append(' ' * width_opt + text[0])
    else:
        lines = ['  {}{}'.format(text_opt.ljust(width_opt - 2), text[0])]
    lines.extend([' ' * width_opt + l for l in text[1:]])
    return ['"{}\\n"'.format(x) for x in lines]

def codegen_all_modules(modules):

    option_value_start = 256

    tpl_run_handler_bool = \
"""template <> void runBoolPredicates(options::{name}__option_t, std::string option, bool b, options::OptionsHandler* handler) {{
  {predicates}
}}"""
# TODO: remove newline before {handler}
    tpl_run_handler = \
"""template <> options::{name}__option_t::type runHandlerAndPredicates(options::{name}__option_t, std::string option, std::string optionarg, options::OptionsHandler* handler) {{
  options::{name}__option_t::type retval = 
  {handler};
  {predicates}
  return retval;
}}"""
    tpl_assign = \
"""template <> void Options::assign(options::{name}__option_t, std::string option, std::string value) {{
  d_holder->{name} = runHandlerAndPredicates(options::{name}, option, value, d_handler);
  d_holder->{name}__setByUser__ = true;
  Trace(\"options\") << \"user assigned option {name}\" << std::endl;
  {notifications}
}}"""
    tpl_assign_bool = \
"""template <> void Options::assignBool(options::{name}__option_t, std::string option, bool value) {{
  runBoolPredicates(options::{name}, option, value, d_handler);
  d_holder->{name} = value;
  d_holder->{name}__setByUser__ = true;
  Trace(\"options\") << \"user assigned option {name}\" << std::endl;
  {notifications}
}}"""

    tpl_options = read_tpl('options_template_new.cpp')

    headers_module = []
    headers_handler = set()
    notifiers = []
    options_short = []
    cmdline_options = []
    options_smt = []
    options_getopts = []
    options_handler = []
    defaults = []
    custom_handlers = []
    help_common = []
    help_others = []

    options_smtget = [] # TODO
    options_smtset = [] # TODO

    option_value_cur = option_value_start
    for module in modules:
        headers_module.append(format_include(module.header))
        # TODO: sort by name
#        for option in sorted(module.options, key=lambda x: x.long if x.long else x.name):
        for option in module.options:
            assert(option.type != 'void' or option.name is None)
            assert(option.name or option.smt_name or option.short or option.long)
            argument_req = option.type not in ['bool', 'void']

            # TODO: we can probably remove these includes since they are
            #       already included in 'headers_module' header files
            for include in option.includes:
                headers_handler.add(format_include(include))

            # generate help text
            if (option.short or option.long) \
               and option.category != 'undocumented':
                l = help_common if option.category == 'common' else help_others
                if not option.help:
                    print("no help for {} option {}".format(
                           option.category, option.name))
                l.extend(help_format(option.help, option.short, option.long))

            ### generate handler call
            handler = None
            if option.handler:
                handler = \
                    'handler->{}(option, optionarg)'.format(option.handler)
            elif option.type != 'bool':
                handler = \
                    'handleOption<{}>(option, optionarg)'.format(option.type)

            ### generate predicate calls
            predicates = []
            if option.predicates:
                if option.type == 'bool':
                    predicates = \
                        ['handler->{}(option, b);'.format(x) \
                            for x in option.predicates]
                elif option.type != 'void':
                    predicates = \
                        ['handler->{}(option, retval);'.format(x) \
                            for x in option.predicates]
                else:
                    assert(False)

            ### generate notification calls
            notifications = \
                ['d_handler->{}(option);'.format(x) for x in option.notifies]

            ### options_handler and cmdline_options
            cases = []
            if option.short:
                cases.append("case '{}':".format(option.short))

                options_short.append(option.short)
                if argument_req:
                    options_short.append(':')

            if option.long:
                cases.append(
                    'case {}:// --{}'.format(option_value_cur, option.long))
                cmdline_options.append(
                    '{{ "{}", {}_argument, nullptr, {} }},'.format(
                        long_get_option(option.long),
                        'required' if argument_req else 'no',
                        option_value_cur))
                option_value_cur += 1

            if len(cases) > 0:
                if option.type == 'bool' and option.name:
                    cases.append(
                        '  options->assignBool(options::{}, option, true);'.format(option.name))
                elif option.type != 'void' and option.name:
                    cases.append(
                        '  options->assign(options::{}, option, optionarg);'.format(option.name))
                elif handler:
                    cases.append('{};'.format(handler))

                cases.extend(
                    ['  extender->pushBackPreemption("{}");'.format(x) \
                        for x in option.links])
                cases.append('  break;\n')

                options_handler.extend(cases)

            # handle --no- case for boolean options
            if option.long and option.type == 'bool':
                cases = []
                cases.append(
                    'case {}:// --no-{}'.format(
                        option_value_cur, option.long))
                option_value_cur += 1
                cases.append(
                    '  options->assignBool(options::{}, option, false);'.format(option.name))
                cases.append('  break;\n')

                options_handler.extend(cases)

                cmdline_options.append(
                    '{{ "no-{}", {}_argument, nullptr, {} }},'.format(
                        long_get_option(option.long),
                        'required' if argument_req else 'no',
                        option_value_cur))

            # collect SMT option names
            if option.smt_name:
                options_smt.append('"{}",'.format(option.smt_name))

            ### build opts for options::getOptions()
            if option.name:
                optname = None
                if option.smt_name:
                    optname = option.smt_name
                else:
                    optname = option.long

                if optname:
                    if option.type == 'bool':
                        s  = '{ std::vector<std::string> v; '
                        s += 'v.push_back("{}"); '.format(optname)
                        s += 'v.push_back(std::string(d_holder->{} ? "true" : "false")); '.format(option.name)
                        s += 'opts.push_back(v); }'
                    else:
                        s  = '{ std::stringstream ss; '
                        if is_numeric_type(option.type):
                            s += 'ss << std::fixed << std::setprecision(8); '
                        s += 'ss << d_holder->{}; '.format(option.name)
                        s += 'std::vector<std::string> v; '
                        s += 'v.push_back("{}"); '.format(optname)
                        s += 'v.push_back(ss.str()); '
                        s += 'opts.push_back(v); }'
                    options_getopts.append(s)


            ### define runBoolPredicates/runHandlerAndPredicates
            if option.name:
                if option.type == 'bool':
                    if len(predicates) > 0:
                        assert(handler is None)
                        custom_handlers.append(
                            tpl_run_handler_bool.format(
                                name=option.name,
                                predicates='\n'.join(predicates)
                            ))
                elif option.short or option.long:
                    assert(option.type != 'void')
                    assert(handler)
                    custom_handlers.append(
                        tpl_run_handler.format(
                            name=option.name,
                            handler=handler,
                            predicates='\n'.join(predicates)
                        ))

            ### define handler assign/assignBool
            if option.name:
                tpl = tpl_assign_bool if option.type == 'bool' else tpl_assign
                if option.type == 'bool' \
                   or option.short or option.long or option.smt_name:
                    custom_handlers.append(
                            tpl.format(
                                name=option.name,
                                notifications='\n'.join(notifications)
                            ))

            # default option values
            if option.name:
                default = option.default if option.default else ''
                defaults.append('{}({})'.format(option.name, default))
                defaults.append('{}__setByUser__(false)'.format(option.name))


#        for alias in sorted(module.aliases, key=lambda x: x.long):
        for alias in module.aliases:
            argument_req = '=' in alias.long

            options_handler.append(
                'case {}:// --{}'.format(option_value_cur, alias.long))

            assert(len(alias.links) > 0)
            arg = long_get_arg(alias.long)
            for link in alias.links:
                arg_link = long_get_arg(link)
                if arg == arg_link:
                    options_handler.append(
                        'extender->pushBackPreemption("{}");'.format(long_get_option(link)))
                    if argument_req:
                        options_handler.append(
                            'extender->pushBackPreemption(optionarg.c_str());')
                else:
                    options_handler.append(
                        'extender->pushBackPreemption("{}");'.format(link))

            options_handler.append('  break;\n')

            cmdline_options.append(
                '{{ "{}", {}_argument, nullptr, {} }},'.format(
                    long_get_option(alias.long),
                    'required' if argument_req else 'no',
                    option_value_cur))
            option_value_cur += 1

            # generate help text
            if alias.category != 'undocumented':
                l = help_common if alias.category == 'common' else help_others
                if not alias.help:
                    print("no help for {} alias {}".format(
                            alias.category, alias.long))
                l.extend(help_format(alias.help, None, alias.long))



    write_file('options.cpp',
        tpl_options.format(
            template='options_template_new.cpp',
            headers_module='\n'.join(headers_module),
            headers_handler='\n'.join(sorted(list(headers_handler))),
            custom_handlers='\n'.join(custom_handlers),
            module_defaults=',\n  '.join(defaults),
            help_common='\n'.join(help_common),
            help_others='\n'.join(help_others),
            cmdline_options='\n  '.join(cmdline_options),
            options_short=''.join(options_short),
            options_handler='\n    '.join(options_handler),
            option_value_begin=option_value_start,
            option_value_end=option_value_cur,
            options_smt='\n  '.join(options_smt),
            options_getopts='\n  '.join(options_getopts)
        ))




def check(data):

    data_options = []
    data_aliases = []

    if 'option' in data:
        data_options = data['option']
        del(data['option'])
    if 'alias' in data:
        data_aliases = data['alias']
        del(data['alias'])

    check_attribs(MODULE_ATTR_REQ, MODULE_ATTR_ALL, data, 'module')

    module = Module(data)
    for attribs in data_options:
        check_attribs(OPTION_ATTR_REQ, OPTION_ATTR_ALL, attribs, 'option')
        module.options.append(Option(attribs))
    for attribs in data_aliases:
        check_attribs(ALIAS_ATTR_REQ, ALIAS_ATTR_ALL, attribs, 'alias')
        module.aliases.append(Alias(attribs))

    return module


if __name__ == "__main__":

    if len(sys.argv) < 2:
        sys.exit("no TOML option file(s) given")

    filenames = sys.argv[1:]

    # parse files, check attributes and create module/option objects
    modules = []
    for filename in filenames:
        with open(filename, 'r') as f:
            modules.append(check(toml.load(f)))

    for module in modules:
        codegen_module(module)

    codegen_all_modules(modules)

    sys.exit(0)
